// =========================================================
// profile.rs - View and update user profile
// =========================================================

use actix_web::{web, HttpResponse};
use sqlx::MySqlPool;
use bcrypt::{hash, verify, DEFAULT_COST};

use crate::models::*;
use crate::transfer::get_customer_from_session;

pub async fn get_profile(
    db:   web::Data<MySqlPool>,
    path: web::Path<String>,
) -> HttpResponse {
    let cid = path.into_inner();

    let user = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ?"
    )
    .bind(&cid)
    .fetch_optional(db.get_ref())
    .await
    .unwrap_or(None);

    let acc = sqlx::query_as::<_, AccountRow>(
        "SELECT * FROM accounts WHERE customer_id = ?"
    )
    .bind(&cid)
    .fetch_optional(db.get_ref())
    .await
    .unwrap_or(None);

    match user {
        Some(u) => {
            // Mask sensitive fields before sending
            let pan_masked    = u.pan_number.as_deref().map(|p| mask_pan(p));
            let aadhaar_masked = u.aadhaar.as_deref().map(|a| mask_aadhaar(a));

            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "customer_id":   u.customer_id,
                "full_name":     u.full_name,
                "dob":           u.dob,
                "gender":        u.gender,
                "pan_masked":    pan_masked,
                "aadhaar_masked": aadhaar_masked,
                "mobile":        u.mobile,
                "email":         u.email,
                "address":       u.house_addr,
                "city":          u.city,
                "state":         u.state_name,
                "pin_code":      u.pin_code,
                "account": acc.map(|a| serde_json::json!({
                    "acc_number": a.acc_number,
                    "acc_type":   a.acc_type,
                    "ifsc":       a.ifsc_code,
                    "branch":     a.branch_name,
                    "opened_on":  a.opened_on,
                    "balance":    a.balance,
                })),
            })))
        }
        None => HttpResponse::Ok().json(ApiResp::fail("User not found")),
    }
}

pub async fn update_profile(
    db:   web::Data<MySqlPool>,
    body: web::Json<ProfileUpdateReq>,
) -> HttpResponse {
    let cid = match get_customer_from_session(db.get_ref(), &body.session_token).await {
        Some(id) => id,
        None => return HttpResponse::Ok().json(ApiResp::fail("Session expired. Please login again.")),
    };

    // Only allow updating certain safe fields
    let allowed = ["mobile", "email", "house_addr", "city", "state_name", "pin_code"];

    if !allowed.contains(&body.field_name.as_str()) {
        return HttpResponse::Ok().json(ApiResp::fail("That field cannot be updated here."));
    }

    let sql = format!("UPDATE users SET {} = ? WHERE customer_id = ?", body.field_name);

    let result = sqlx::query(&sql)
        .bind(&body.new_value)
        .bind(&cid)
        .execute(db.get_ref())
        .await;

    match result {
        Ok(_) => HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
            "updated": true,
            "field":   body.field_name,
        }))),
        Err(e) => {
            log::error!("Profile update error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Update failed"))
        }
    }
}

pub async fn change_password(
    db:   web::Data<MySqlPool>,
    body: web::Json<ChangePwdReq>,
) -> HttpResponse {
    let cid = match get_customer_from_session(db.get_ref(), &body.session_token).await {
        Some(id) => id,
        None => return HttpResponse::Ok().json(ApiResp::fail("Session expired. Please login again.")),
    };

    // Get current hash to verify current password
    let user = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ?"
    )
    .bind(&cid)
    .fetch_optional(db.get_ref())
    .await
    .unwrap_or(None);

    let user = match user {
        Some(u) => u,
        None => return HttpResponse::Ok().json(ApiResp::fail("User not found")),
    };

    let stored_hash = match user.pwd_hash {
        Some(h) => h,
        None => return HttpResponse::Ok().json(ApiResp::fail("No password set")),
    };

    // Check current password is correct
    if !verify(&body.current_pwd, &stored_hash).unwrap_or(false) {
        return HttpResponse::Ok().json(ApiResp::fail("Current password is incorrect."));
    }

    // Hash and save the new password
    let new_hash = match hash(&body.new_pwd, DEFAULT_COST) {
        Ok(h) => h,
        Err(_) => return HttpResponse::InternalServerError().json(ApiResp::fail("Error processing password")),
    };

    let _ = sqlx::query(
        "UPDATE users SET pwd_hash = ? WHERE customer_id = ?"
    )
    .bind(&new_hash)
    .bind(&cid)
    .execute(db.get_ref())
    .await;

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "changed": true,
        "message": "Password changed successfully."
    })))
}

fn mask_pan(pan: &str) -> String {
    if pan.len() >= 4 {
        format!("ABCXX{}", &pan[5..])
    } else {
        "XXXXX".to_string()
    }
}

fn mask_aadhaar(a: &str) -> String {
    if a.len() >= 4 {
        format!("XXXX XXXX {}", &a[a.len()-4..])
    } else {
        "XXXX XXXX XXXX".to_string()
    }
}
