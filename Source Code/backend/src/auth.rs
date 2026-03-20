

use actix_web::{web, HttpResponse};
use sqlx::MySqlPool;
use bcrypt::{hash, verify, DEFAULT_COST};
use uuid::Uuid;
use rand::Rng;
use chrono::{Local, Duration};

use crate::models::*;


pub async fn check_customer_id(
    db:   web::Data<MySqlPool>,
    body: web::Json<CheckCustomerReq>,
) -> HttpResponse {
    let cid = &body.customer_id;

    let found = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ?"
    )
    .bind(cid)
    .fetch_optional(db.get_ref())
    .await;

    match found {
        Ok(Some(user)) => {
            
            let first_name = user.full_name.split_whitespace().next().unwrap_or("").to_string();
            let acc = get_masked_acc(db.get_ref(), cid).await;

            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "exists":     true,
                "first_name": first_name,
                "acc_masked": acc,
                "net_active": user.net_active,
            })))
        }
        Ok(None) => {
            HttpResponse::Ok().json(ApiResp::fail("Customer ID not found. Please check and try again."))
        }
        Err(e) => {
            log::error!("DB error in check_customer_id: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"))
        }
    }
}


pub async fn do_login(
    db:   web::Data<MySqlPool>,
    body: web::Json<LoginReq>,
) -> HttpResponse {
    let cid = &body.customer_id;
    let pwd = &body.password;

    let lock_check = sqlx::query_as::<_, (i8,)>(
        "SELECT locked_out FROM login_attempts WHERE customer_id = ?"
    )
    .bind(cid)
    .fetch_optional(db.get_ref())
    .await;

    if let Ok(Some((1,))) = lock_check {
        return HttpResponse::Ok().json(ApiResp::fail(
            "Account locked. Please reset your password to continue."
        ));
    }

    
    let user_result = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ? AND net_active = 1"
    )
    .bind(cid)
    .fetch_optional(db.get_ref())
    .await;

    match user_result {
        Ok(Some(user)) => {
            let stored_hash = match &user.pwd_hash {
                Some(h) => h.clone(),
                None => return HttpResponse::Ok().json(ApiResp::fail("NetBanking not activated")),
            };

            
            let pwd_ok = verify(pwd, &stored_hash).unwrap_or(false);

            if pwd_ok {
                
                let _ = sqlx::query(
                    "DELETE FROM login_attempts WHERE customer_id = ?"
                )
                .bind(cid)
                .execute(db.get_ref())
                .await;

                
                let token = Uuid::new_v4().to_string();
                let expires = Local::now() + Duration::minutes(30);

                let _ = sqlx::query(
                    "INSERT INTO sessions (token, customer_id, ends_at) VALUES (?, ?, ?)"
                )
                .bind(&token)
                .bind(cid)
                .bind(expires.naive_local())
                .execute(db.get_ref())
                .await;

                HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                    "token": token,
                    "customer_id": cid,
                })))
            } else {
                // Track wrong attempts
                track_bad_attempt(db.get_ref(), cid).await;

                let attempts_left = get_attempts_left(db.get_ref(), cid).await;

                if attempts_left == 0 {
                    HttpResponse::Ok().json(ApiResp::fail(
                        "Account locked. Please reset your password to continue."
                    ))
                } else {
                    HttpResponse::Ok().json(ApiResp::fail(&format!(
                        "Incorrect password. {} attempts remaining.", attempts_left
                    )))
                }
            }
        }
        Ok(None) => {
            HttpResponse::Ok().json(ApiResp::fail("Customer ID not found or NetBanking not active."))
        }
        Err(e) => {
            log::error!("Login DB error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"))
        }
    }
}


pub async fn check_for_activation(
    db:   web::Data<MySqlPool>,
    body: web::Json<CheckCustomerReq>,
) -> HttpResponse {
    let cid = &body.customer_id;

    let found = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ?"
    )
    .bind(cid)
    .fetch_optional(db.get_ref())
    .await;

    match found {
        Ok(Some(user)) => {
            if user.net_active == 1 {
                HttpResponse::Ok().json(ApiResp::fail(
                    "NetBanking already activated. Please login."
                ))
            } else {
                HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                    "can_activate": true,
                    "full_name": user.full_name,
                })))
            }
        }
        Ok(None) => {
            HttpResponse::Ok().json(ApiResp::fail("Customer ID not found."))
        }
        Err(e) => {
            log::error!("Activation check error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"))
        }
    }
}


pub async fn verify_card(
    db:   web::Data<MySqlPool>,
    body: web::Json<VerifyCardReq>,
) -> HttpResponse {
    let cid = &body.customer_id;

    let card = sqlx::query_as::<_, CardRow>(
        "SELECT * FROM debit_cards WHERE customer_id = ?"
    )
    .bind(cid)
    .fetch_optional(db.get_ref())
    .await;

    match card {
        Ok(Some(c)) => {
            
            let num_ok    = c.card_number == body.card_number.replace(" ", "");
            let expiry_ok = c.card_expiry == body.card_expiry;
            let cvv_ok    = verify(&body.cvv, &c.cvv_hash).unwrap_or(false);

            if num_ok && expiry_ok && cvv_ok {
                HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                    "card_ok": true
                })))
            } else {
                HttpResponse::Ok().json(ApiResp::fail(
                    "Card details do not match our records. Please check and try again."
                ))
            }
        }
        Ok(None) => {
            HttpResponse::Ok().json(ApiResp::fail("No card found for this account."))
        }
        Err(e) => {
            log::error!("Card verify error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"))
        }
    }
}

-
pub async fn make_otp(
    db:   web::Data<MySqlPool>,
    body: web::Json<OtpReq>,
) -> HttpResponse {
    let cid     = &body.customer_id;
    let purpose = &body.purpose;

    
    let otp_code: u32 = rand::thread_rng().gen_range(100000..999999);
    let otp_str  = otp_code.to_string();

    
    let good_till = Local::now() + Duration::minutes(5);

    
    let _ = sqlx::query(
        "DELETE FROM otp_codes WHERE customer_id = ? AND for_what = ?"
    )
    .bind(cid)
    .bind(purpose)
    .execute(db.get_ref())
    .await;

    
    let saved = sqlx::query(
        "INSERT INTO otp_codes (customer_id, the_otp, for_what, good_till) VALUES (?, ?, ?, ?)"
    )
    .bind(cid)
    .bind(&otp_str)
    .bind(purpose)
    .bind(good_till.naive_local())
    .execute(db.get_ref())
    .await;

    match saved {
        Ok(_) => {
           
            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "otp_for_demo": otp_str,
                "valid_mins":   5,
            })))
        }
        Err(e) => {
            log::error!("OTP save error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Could not generate OTP"))
        }
    }
}


pub async fn verify_otp(
    db:   web::Data<MySqlPool>,
    body: web::Json<OtpVerifyReq>,
) -> HttpResponse {
    let now = Local::now().naive_local();

    let otp_row = sqlx::query_as::<_, OtpRow>(
        "SELECT * FROM otp_codes
         WHERE customer_id = ? AND for_what = ? AND already_used = 0 AND good_till > ?
         ORDER BY id DESC LIMIT 1"
    )
    .bind(&body.customer_id)
    .bind(&body.purpose)
    .bind(now)
    .fetch_optional(db.get_ref())
    .await;

    match otp_row {
        Ok(Some(row)) => {
            if row.the_otp == body.otp_entered {
                // Mark it used so it can't be reused
                let _ = sqlx::query(
                    "UPDATE otp_codes SET already_used = 1 WHERE id = ?"
                )
                .bind(row.id)
                .execute(db.get_ref())
                .await;

                HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                    "verified": true
                })))
            } else {
                HttpResponse::Ok().json(ApiResp::fail("Invalid OTP. Please check and try again."))
            }
        }
        Ok(None) => {
            HttpResponse::Ok().json(ApiResp::fail("OTP expired. Please click Resend to get a new one."))
        }
        Err(e) => {
            log::error!("OTP verify error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"))
        }
    }
}


pub async fn set_password(
    db:   web::Data<MySqlPool>,
    body: web::Json<SetPwdReq>,
) -> HttpResponse {
    let hashed = match hash(&body.new_password, DEFAULT_COST) {
        Ok(h) => h,
        Err(_) => return HttpResponse::InternalServerError().json(ApiResp::fail("Hashing failed")),
    };

    let result = sqlx::query(
        "UPDATE users SET pwd_hash = ?, net_active = 1 WHERE customer_id = ?"
    )
    .bind(&hashed)
    .bind(&body.customer_id)
    .execute(db.get_ref())
    .await;

    match result {
        Ok(_) => HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
            "activated": true
        }))),
        Err(e) => {
            log::error!("Set password error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Could not save password"))
        }
    }
}


pub async fn reset_password(
    db:   web::Data<MySqlPool>,
    body: web::Json<ResetPwdReq>,
) -> HttpResponse {
  
    let user = sqlx::query_as::<_, UserRow>(
        "SELECT * FROM users WHERE customer_id = ? AND mobile = ?"
    )
    .bind(&body.customer_id)
    .bind(&body.mobile)
    .fetch_optional(db.get_ref())
    .await;

    let _found_user = match user {
        Ok(Some(u)) => u,
        Ok(None) => return HttpResponse::Ok().json(ApiResp::fail(
            "Mobile number does not match our records."
        )),
        Err(e) => {
            log::error!("Reset pwd user check error: {}", e);
            return HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"));
        }
    };

    
    let hashed = match hash(&body.new_password, DEFAULT_COST) {
        Ok(h) => h,
        Err(_) => return HttpResponse::InternalServerError().json(ApiResp::fail("Hashing failed")),
    };

    let _ = sqlx::query(
        "UPDATE users SET pwd_hash = ? WHERE customer_id = ?"
    )
    .bind(&hashed)
    .bind(&body.customer_id)
    .execute(db.get_ref())
    .await;

    
    let _ = sqlx::query(
        "UPDATE login_attempts SET locked_out = 0, bad_attempts = 0 WHERE customer_id = ?"
    )
    .bind(&body.customer_id)
    .execute(db.get_ref())
    .await;

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "reset_done": true
    })))
}


pub async fn check_session(
    db:    web::Data<MySqlPool>,
    query: web::Query<std::collections::HashMap<String, String>>,
) -> HttpResponse {
    let token = match query.get("token") {
        Some(t) => t.clone(),
        None    => return HttpResponse::Ok().json(ApiResp::fail("No token provided")),
    };

    let now = Local::now().naive_local();

    let sess = sqlx::query_as::<_, SessionRow>(
        "SELECT * FROM sessions WHERE token = ? AND is_live = 1 AND ends_at > ?"
    )
    .bind(&token)
    .bind(now)
    .fetch_optional(db.get_ref())
    .await;

    match sess {
        Ok(Some(s)) => {
            HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
                "valid":       true,
                "customer_id": s.customer_id,
            })))
        }
        Ok(None) => {
            HttpResponse::Ok().json(ApiResp::fail("Session expired or invalid."))
        }
        Err(e) => {
            log::error!("Session check error: {}", e);
            HttpResponse::InternalServerError().json(ApiResp::fail("Something went wrong"))
        }
    }
}


pub async fn logout(
    db:   web::Data<MySqlPool>,
    body: web::Json<serde_json::Value>,
) -> HttpResponse {
    let token = body.get("token").and_then(|t| t.as_str()).unwrap_or("");

    let _ = sqlx::query(
        "UPDATE sessions SET is_live = 0 WHERE token = ?"
    )
    .bind(token)
    .execute(db.get_ref())
    .await;

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "logged_out": true
    })))
}



async fn get_masked_acc(db: &MySqlPool, cid: &str) -> String {
    let row = sqlx::query_as::<_, (String,)>(
        "SELECT acc_number FROM accounts WHERE customer_id = ?"
    )
    .bind(cid)
    .fetch_optional(db)
    .await
    .unwrap_or(None);

    if let Some((acc,)) = row {
        if acc.len() >= 4 {
            let last_four = &acc[acc.len()-4..];
            format!("XXXXXXXX{}", last_four)
        } else {
            "XXXXXX".to_string()
        }
    } else {
        "XXXXXX".to_string()
    }
}

async fn track_bad_attempt(db: &MySqlPool, cid: &str) {
    // Try to update existing row
    let updated = sqlx::query(
        "UPDATE login_attempts SET bad_attempts = bad_attempts + 1, last_try = NOW()
         WHERE customer_id = ?"
    )
    .bind(cid)
    .execute(db)
    .await;

    if let Ok(r) = updated {
        if r.rows_affected() == 0 {
            // First bad attempt - insert a new row
            let _ = sqlx::query(
                "INSERT INTO login_attempts (customer_id, bad_attempts) VALUES (?, 1)"
            )
            .bind(cid)
            .execute(db)
            .await;
        } else {
            
            let _ = sqlx::query(
                "UPDATE login_attempts SET locked_out = 1
                 WHERE customer_id = ? AND bad_attempts >= 3"
            )
            .bind(cid)
            .execute(db)
            .await;
        }
    }
}

async fn get_attempts_left(db: &MySqlPool, cid: &str) -> i32 {
    let row = sqlx::query_as::<_, (i32,)>(
        "SELECT bad_attempts FROM login_attempts WHERE customer_id = ?"
    )
    .bind(cid)
    .fetch_optional(db)
    .await
    .unwrap_or(None);

    let attempts = row.map(|(a,)| a).unwrap_or(0);
    std::cmp::max(0, 3 - attempts)
}
