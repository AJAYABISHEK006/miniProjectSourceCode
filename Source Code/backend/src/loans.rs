// =========================================================
// loans.rs - Handle all loan applications
// Personal, Home, Education loan flows
// =========================================================

use actix_web::{web, HttpResponse};
use sqlx::MySqlPool;
use uuid::Uuid;

use crate::models::*;
use crate::transfer::get_customer_from_session;

pub async fn apply_for_loan(
    db:   web::Data<MySqlPool>,
    body: web::Json<LoanApplyReq>,
) -> HttpResponse {
    let cid = match get_customer_from_session(db.get_ref(), &body.session_token).await {
        Some(id) => id,
        None => return HttpResponse::Ok().json(ApiResp::fail("Session expired. Please login again.")),
    };

    // Generate a nice application ID like NXB-PL-2024-001
    let short_type = match body.loan_type.as_str() {
        "Personal Loan"  => "PL",
        "Home Loan"      => "HL",
        "Education Loan" => "EL",
        _                => "LN",
    };
    let rand_part: u32 = rand::random::<u32>() % 9000 + 1000;
    let app_id = format!("NXB-{}-2024-{}", short_type, rand_part);

    let saved = sqlx::query(
        "INSERT INTO loan_apps (app_id, customer_id, loan_type, loan_amount, tenure_months, form_data)
         VALUES (?, ?, ?, ?, ?, ?)"
    )
    .bind(&app_id)
    .bind(&cid)
    .bind(&body.loan_type)
    .bind(body.loan_amount)
    .bind(body.tenure)
    .bind(&body.form_data)
    .execute(db.get_ref())
    .await;

    match saved {
        Ok(_) => HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
            "app_id":    app_id,
            "status":    "Under Review",
            "loan_type": body.loan_type,
            "amount":    body.loan_amount,
        }))),
        Err(e) => {
            log::error!("Loan apply error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Could not save application"))
        }
    }
}

pub async fn get_my_loans(
    db:   web::Data<MySqlPool>,
    path: web::Path<String>,
) -> HttpResponse {
    let cid = path.into_inner();

    let loans = sqlx::query_as::<_, LoanRow>(
        "SELECT * FROM loan_apps WHERE customer_id = ? ORDER BY applied_on DESC"
    )
    .bind(&cid)
    .fetch_all(db.get_ref())
    .await
    .unwrap_or_default();

    let result: Vec<serde_json::Value> = loans.iter().map(|l| serde_json::json!({
        "app_id":     l.app_id,
        "loan_type":  l.loan_type,
        "amount":     l.loan_amount,
        "tenure":     l.tenure_months,
        "status":     l.app_status,
        "applied_on": l.applied_on,
    })).collect();

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "loans": result
    })))
}
