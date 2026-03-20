// =========================================================
// services.rs - All service requests (address, KYC etc)
// =========================================================

use actix_web::{web, HttpResponse};
use sqlx::MySqlPool;

use crate::models::*;
use crate::transfer::get_customer_from_session;

pub async fn raise_sr(
    db:   web::Data<MySqlPool>,
    body: web::Json<ServiceReqBody>,
) -> HttpResponse {
    let cid = match get_customer_from_session(db.get_ref(), &body.session_token).await {
        Some(id) => id,
        None => return HttpResponse::Ok().json(ApiResp::fail("Session expired. Please login again.")),
    };

    // Generate SR number
    let rand_num: u32 = rand::random::<u32>() % 90000 + 10000;
    let sr_number = format!("SR{}", rand_num);

    // For some SRs we also update the DB immediately
    // e.g., address change updates the users table right away
    match body.sr_type.as_str() {
        "Address Change" => {
            // Parse the new address from sr_data JSON
            if let Ok(data) = serde_json::from_str::<serde_json::Value>(&body.sr_data) {
                let new_addr  = data.get("new_address").and_then(|v| v.as_str()).unwrap_or("");
                let new_city  = data.get("city").and_then(|v| v.as_str()).unwrap_or("");
                let new_state = data.get("state").and_then(|v| v.as_str()).unwrap_or("");
                let new_pin   = data.get("pin_code").and_then(|v| v.as_str()).unwrap_or("");

                let _ = sqlx::query(
                    "UPDATE users SET house_addr = ?, city = ?, state_name = ?, pin_code = ?
                     WHERE customer_id = ?"
                )
                .bind(new_addr)
                .bind(new_city)
                .bind(new_state)
                .bind(new_pin)
                .bind(&cid)
                .execute(db.get_ref())
                .await;
            }
        }
        "Update Email ID" => {
            if let Ok(data) = serde_json::from_str::<serde_json::Value>(&body.sr_data) {
                let new_email = data.get("new_email").and_then(|v| v.as_str()).unwrap_or("");
                let _ = sqlx::query(
                    "UPDATE users SET email = ? WHERE customer_id = ?"
                )
                .bind(new_email)
                .bind(&cid)
                .execute(db.get_ref())
                .await;
            }
        }
        _ => {
            // Other SRs just get logged - no immediate DB update
        }
    }

    // Save the SR record
    let saved = sqlx::query(
        "INSERT INTO service_reqs (sr_number, customer_id, sr_type, sr_data) VALUES (?, ?, ?, ?)"
    )
    .bind(&sr_number)
    .bind(&cid)
    .bind(&body.sr_type)
    .bind(&body.sr_data)
    .execute(db.get_ref())
    .await;

    match saved {
        Ok(_) => HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
            "sr_number": sr_number,
            "sr_type":   body.sr_type,
            "status":    "Pending",
        }))),
        Err(e) => {
            log::error!("SR save error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Could not submit service request"))
        }
    }
}
