// =========================================================
// cards.rs - Card management: block, unblock, set PIN etc
// =========================================================

use actix_web::{web, HttpResponse};
use sqlx::MySqlPool;
use uuid::Uuid;

use crate::models::*;
use crate::transfer::get_customer_from_session;

pub async fn get_my_cards(
    db:   web::Data<MySqlPool>,
    path: web::Path<String>,
) -> HttpResponse {
    let cid = path.into_inner();

    let cards = sqlx::query_as::<_, CardRow>(
        "SELECT * FROM debit_cards WHERE customer_id = ?"
    )
    .bind(&cid)
    .fetch_all(db.get_ref())
    .await
    .unwrap_or_default();

    let result: Vec<serde_json::Value> = cards.iter().map(|c| serde_json::json!({
        "card_number_masked": mask_card(&c.card_number),
        "card_expiry":        c.card_expiry,
        "card_type":          c.card_type,
        "card_status":        c.card_status,
    })).collect();

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "cards": result
    })))
}

pub async fn card_action(
    db:   web::Data<MySqlPool>,
    body: web::Json<CardActionReq>,
) -> HttpResponse {
    let cid = match get_customer_from_session(db.get_ref(), &body.session_token).await {
        Some(id) => id,
        None => return HttpResponse::Ok().json(ApiResp::fail("Session expired. Please login again.")),
    };

    match body.action.as_str() {
        "block" => {
            let _ = sqlx::query(
                "UPDATE debit_cards SET card_status = 'Blocked' WHERE customer_id = ?"
            )
            .bind(&cid)
            .execute(db.get_ref())
            .await;

            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "done":    true,
                "message": "Your card has been blocked successfully."
            })))
        }
        "unblock" => {
            let _ = sqlx::query(
                "UPDATE debit_cards SET card_status = 'Active' WHERE customer_id = ?"
            )
            .bind(&cid)
            .execute(db.get_ref())
            .await;

            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "done":    true,
                "message": "Your card has been unblocked."
            })))
        }
        "apply-credit-card" => {
            let card_type = body.extra_data.as_deref().unwrap_or("Millennia Credit Card");
            let rand_part: u32 = rand::random::<u32>() % 9000 + 1000;
            let app_id = format!("NXB-CC-2024-{}", rand_part);

            let _ = sqlx::query(
                "INSERT INTO card_apps (app_id, customer_id, card_type) VALUES (?, ?, ?)"
            )
            .bind(&app_id)
            .bind(&cid)
            .bind(card_type)
            .execute(db.get_ref())
            .await;

            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "app_id":  app_id,
                "status":  "Under Processing",
                "message": "Card will be delivered in 7-10 working days."
            })))
        }
        _ => {
            HttpResponse::Ok().json(ApiResp::fail("Unknown action"))
        }
    }
}

fn mask_card(num: &str) -> String {
    if num.len() >= 4 {
        format!("XXXX XXXX XXXX {}", &num[num.len()-4..])
    } else {
        "XXXX XXXX XXXX XXXX".to_string()
    }
}
