// =========================================================
// transfer.rs - Handle NEFT, IMPS, UPI money transfers
// Also handles the dashboard data fetch
// =========================================================

use actix_web::{web, HttpResponse};
use sqlx::MySqlPool;
use uuid::Uuid;
use chrono::Local;

use crate::models::*;

// ---- Get dashboard data for the logged-in user ----
pub async fn get_dashboard(
    db:      web::Data<MySqlPool>,
    path:    web::Path<String>,
) -> HttpResponse {
    let cid = path.into_inner();

    // Get user info
    let user = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ?"
    )
    .bind(&cid)
    .fetch_optional(db.get_ref())
    .await;

    let user = match user {
        Ok(Some(u)) => u,
        _ => return HttpResponse::Ok().json(ApiResp::fail("User not found")),
    };

    // Get account info
    let account = sqlx::query_as::<_, AccountRow>(
        "SELECT * FROM accounts WHERE customer_id = ?"
    )
    .bind(&cid)
    .fetch_optional(db.get_ref())
    .await
    .unwrap_or(None);

    // Get last 5 transactions
    let recent_txns = sqlx::query_as::<_, TxnRow>(
        "SELECT * FROM transactions WHERE customer_id = ? ORDER BY txn_time DESC LIMIT 5"
    )
    .bind(&cid)
    .fetch_all(db.get_ref())
    .await
    .unwrap_or_default();

    let acc_data = account.map(|a| serde_json::json!({
        "acc_number":  a.acc_number,
        "acc_masked":  mask_acc(&a.acc_number),
        "acc_type":    a.acc_type,
        "balance":     a.balance,
        "ifsc":        a.ifsc_code,
        "branch":      a.branch_name,
        "opened_on":   a.opened_on,
    })).unwrap_or(serde_json::Value::Null);

    let txn_list: Vec<serde_json::Value> = recent_txns.iter().map(|t| serde_json::json!({
        "date":        t.txn_time.format("%d %b %Y").to_string(),
        "time":        t.txn_time.format("%H:%M").to_string(),
        "description": t.description,
        "type":        t.txn_type,
        "amount":      t.amount,
        "bal_after":   t.bal_after,
        "ref_id":      t.ref_id,
    })).collect();

    let first_name = user.full_name.split_whitespace().next().unwrap_or("").to_string();

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "customer_id": user.customer_id,
        "full_name":   user.full_name,
        "first_name":  first_name,
        "mobile":      user.mobile,
        "email":       user.email,
        "account":     acc_data,
        "recent_txns": txn_list,
    })))
}

// ---- Do the actual fund transfer ----
pub async fn do_transfer(
    db:   web::Data<MySqlPool>,
    body: web::Json<TransferReq>,
) -> HttpResponse {
    // First verify the session to get the sender's customer ID
    let sender_id = match get_customer_from_session(db.get_ref(), &body.session_token).await {
        Some(id) => id,
        None => return HttpResponse::Ok().json(ApiResp::fail("Session expired. Please login again.")),
    };

    // Get sender's balance
    let sender_acc = sqlx::query_as::<_, AccountRow>(
        "SELECT * FROM accounts WHERE customer_id = ?"
    )
    .bind(&sender_id)
    .fetch_optional(db.get_ref())
    .await
    .unwrap_or(None);

    let sender_acc = match sender_acc {
        Some(a) => a,
        None => return HttpResponse::Ok().json(ApiResp::fail("Account not found")),
    };

    // Check if they have enough money
    if sender_acc.balance < body.the_amount {
        return HttpResponse::Ok().json(ApiResp::fail(&format!(
            "Insufficient balance. Your available balance is Rs.{:.2}", sender_acc.balance
        )));
    }

    // Minimum transfer amount check
    if body.the_amount <= 0.0 {
        return HttpResponse::Ok().json(ApiResp::fail("Transfer amount must be greater than zero."));
    }

    let new_bal = sender_acc.balance - body.the_amount;
    let txn_id  = format!("TXN{}", Uuid::new_v4().to_string().replace("-", "").to_uppercase()[..16].to_string());
    let now     = Local::now().naive_local();

    // Description based on transfer type
    let desc = match body.transfer_type.as_str() {
        "UPI"  => format!("{} to {}", body.transfer_type, body.to_upi.as_deref().unwrap_or("UPI")),
        _      => format!("{} to {}", body.transfer_type, body.ben_name),
    };

    // Deduct balance and add transaction - both in one go
    // In a real bank this would be a proper DB transaction
    let update_result = sqlx::query(
        "UPDATE accounts SET balance = ? WHERE customer_id = ?"
    )
    .bind(new_bal)
    .bind(&sender_id)
    .execute(db.get_ref())
    .await;

    if update_result.is_err() {
        return HttpResponse::InternalServerError().json(ApiResp::fail("Transfer failed. Please try again."));
    }

    // Log the transaction
    let _ = sqlx::query(
        "INSERT INTO transactions (customer_id, txn_time, description, txn_type, amount, bal_after, ref_id)
         VALUES (?, ?, ?, 'Debit', ?, ?, ?)"
    )
    .bind(&sender_id)
    .bind(now)
    .bind(&desc)
    .bind(body.the_amount)
    .bind(new_bal)
    .bind(&txn_id)
    .execute(db.get_ref())
    .await;

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "transfer_done":  true,
        "txn_id":         txn_id,
        "amount":         body.the_amount,
        "new_balance":    new_bal,
        "transfer_type":  body.transfer_type,
        "beneficiary":    body.ben_name,
    })))
}

// ---- Get all transactions with optional filters ----
pub async fn get_transactions(
    db:    web::Data<MySqlPool>,
    path:  web::Path<String>,
    query: web::Query<std::collections::HashMap<String, String>>,
) -> HttpResponse {
    let cid         = path.into_inner();
    let filter_type = query.get("type").cloned().unwrap_or_default();
    let search_text = query.get("search").cloned().unwrap_or_default();
    let date_from   = query.get("from").cloned().unwrap_or_default();
    let date_to     = query.get("to").cloned().unwrap_or_default();
    let page_num: i32 = query.get("page").and_then(|p| p.parse().ok()).unwrap_or(1);

    let offset = (page_num - 1) * 10;

    // Build the query dynamically based on filters
    let mut sql = String::from(
        "SELECT * FROM transactions WHERE customer_id = ?"
    );

    if filter_type == "Credit" || filter_type == "Debit" {
        sql.push_str(&format!(" AND txn_type = '{}'", filter_type));
    }

    if !search_text.is_empty() {
        sql.push_str(&format!(" AND description LIKE '%{}%'", search_text));
    }

    if !date_from.is_empty() {
        sql.push_str(&format!(" AND DATE(txn_time) >= '{}'", date_from));
    }

    if !date_to.is_empty() {
        sql.push_str(&format!(" AND DATE(txn_time) <= '{}'", date_to));
    }

    sql.push_str(" ORDER BY txn_time DESC LIMIT 10 OFFSET ?");

    let txns = sqlx::query_as::<_, TxnRow>(&sql)
        .bind(&cid)
        .bind(offset)
        .fetch_all(db.get_ref())
        .await
        .unwrap_or_default();

    let result: Vec<serde_json::Value> = txns.iter().map(|t| serde_json::json!({
        "date":        t.txn_time.format("%d %b %Y %H:%M").to_string(),
        "description": t.description,
        "type":        t.txn_type,
        "amount":      t.amount,
        "bal_after":   t.bal_after,
        "ref_id":      t.ref_id,
    })).collect();

    // Get account balance to show at top
    let bal = sqlx::query_as::<_, (f64,)>("SELECT balance FROM accounts WHERE customer_id = ?")
        .bind(&cid)
        .fetch_optional(db.get_ref())
        .await
        .unwrap_or(None)
        .map(|(b,)| b)
        .unwrap_or(0.0);

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "transactions": result,
        "current_page": page_num,
        "balance":      bal,
    })))
}

// ---- Simple IFSC lookup - returns bank name ----
pub async fn ifsc_lookup(
    body: web::Json<serde_json::Value>,
) -> HttpResponse {
    let ifsc = body.get("ifsc").and_then(|i| i.as_str()).unwrap_or("");

    // In a real system this would hit an IFSC API
    // For demo we just return something based on the first few chars
    let bank_name = if ifsc.starts_with("NXB") {
        "NexusBank"
    } else if ifsc.starts_with("HDFC") {
        "HDFC Bank"
    } else if ifsc.starts_with("SBIN") {
        "State Bank of India"
    } else if ifsc.starts_with("ICIC") {
        "ICICI Bank"
    } else if ifsc.starts_with("KKBK") {
        "Kotak Mahindra Bank"
    } else {
        "Other Bank"
    };

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "bank_name": bank_name
    })))
}

// =========================================================
// Helper: get customer ID from session token
// =========================================================

pub async fn get_customer_from_session(db: &MySqlPool, token: &str) -> Option<String> {
    let now = Local::now().naive_local();

    let row = sqlx::query_as::<_, SessionRow>(
        "SELECT * FROM sessions WHERE token = ? AND is_live = 1 AND ends_at > ?"
    )
    .bind(token)
    .bind(now)
    .fetch_optional(db)
    .await
    .unwrap_or(None);

    row.map(|s| s.customer_id)
}

fn mask_acc(acc: &str) -> String {
    if acc.len() >= 4 {
        format!("XXXXXXXX{}", &acc[acc.len()-4..])
    } else {
        "XXXXXX".to_string()
    }
}
