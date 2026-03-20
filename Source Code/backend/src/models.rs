// =========================================================
// models.rs - All data structures used across the app
// These map to the MySQL tables in schema.sql
// =========================================================

use serde::{Deserialize, Serialize};
use chrono::NaiveDateTime;

// ---- Standard API response sent back to CGI ----
#[derive(Serialize, Deserialize)]
pub struct ApiResp {
    pub ok:    bool,
    pub data:  serde_json::Value,
    pub msg:   String,
}

impl ApiResp {
    pub fn success(payload: serde_json::Value) -> Self {
        ApiResp { ok: true, data: payload, msg: String::new() }
    }

    pub fn fail(reason: &str) -> Self {
        ApiResp {
            ok:   false,
            data: serde_json::Value::Null,
            msg:  reason.to_string(),
        }
    }
}

// ---- User row from the users table ----
#[derive(Serialize, Deserialize, sqlx::FromRow, Clone)]
pub struct UserRow {
    pub id:          i32,
    pub customer_id: String,
    pub full_name:   String,
    pub dob:         String,
    pub gender:      Option<String>,
    pub pan_number:  Option<String>,
    pub aadhaar:     Option<String>,
    pub mobile:      String,
    pub email:       Option<String>,
    pub house_addr:  Option<String>,
    pub city:        Option<String>,
    pub state_name:  Option<String>,
    pub pin_code:    Option<String>,
    pub pwd_hash:    Option<String>,
    pub net_active:  i8,
}

// ---- Account row ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct AccountRow {
    pub id:          i32,
    pub customer_id: String,
    pub acc_number:  String,
    pub acc_type:    String,
    pub ifsc_code:   String,
    pub branch_name: Option<String>,
    pub balance:     f64,
    pub opened_on:   Option<String>,
}

// ---- Debit card row ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct CardRow {
    pub id:          i32,
    pub customer_id: String,
    pub card_number: String,
    pub card_expiry: String,
    pub cvv_hash:    String,
    pub card_type:   Option<String>,
    pub card_status: String,
}

// ---- Single transaction entry ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct TxnRow {
    pub id:          i32,
    pub customer_id: String,
    pub txn_time:    NaiveDateTime,
    pub description: Option<String>,
    pub txn_type:    String,
    pub amount:      f64,
    pub bal_after:   f64,
    pub ref_id:      Option<String>,
}

// ---- Loan application row ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct LoanRow {
    pub id:            i32,
    pub app_id:        String,
    pub customer_id:   String,
    pub loan_type:     String,
    pub loan_amount:   f64,
    pub tenure_months: i32,
    pub app_status:    String,
    pub form_data:     Option<String>,
    pub applied_on:    Option<NaiveDateTime>,
}

// ---- Service request row ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct SrRow {
    pub id:          i32,
    pub sr_number:   String,
    pub customer_id: String,
    pub sr_type:     String,
    pub sr_data:     Option<String>,
    pub sr_status:   String,
    pub raised_on:   Option<NaiveDateTime>,
}

// ---- Session row ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct SessionRow {
    pub id:          i32,
    pub token:       String,
    pub customer_id: String,
    pub is_live:     i8,
}

// ---- OTP row ----
#[derive(Serialize, Deserialize, sqlx::FromRow)]
pub struct OtpRow {
    pub id:           i32,
    pub customer_id:  String,
    pub the_otp:      String,
    pub for_what:     String,
    pub already_used: i8,
}

// =========================================================
// Request body structures - what CGI sends to Rust
// =========================================================

#[derive(Deserialize)]
pub struct LoginReq {
    pub customer_id: String,
    pub password:    String,
}

#[derive(Deserialize)]
pub struct CheckCustomerReq {
    pub customer_id: String,
}

#[derive(Deserialize)]
pub struct VerifyCardReq {
    pub customer_id:  String,
    pub card_number:  String,
    pub card_expiry:  String,
    pub cvv:          String,
}

#[derive(Deserialize)]
pub struct OtpReq {
    pub customer_id: String,
    pub purpose:     String,
}

#[derive(Deserialize)]
pub struct OtpVerifyReq {
    pub customer_id: String,
    pub otp_entered: String,
    pub purpose:     String,
}

#[derive(Deserialize)]
pub struct SetPwdReq {
    pub customer_id:  String,
    pub new_password: String,
}

#[derive(Deserialize)]
pub struct ResetPwdReq {
    pub customer_id:  String,
    pub mobile:       String,
    pub new_password: String,
}

#[derive(Deserialize)]
pub struct TransferReq {
    pub session_token:  String,
    pub transfer_type:  String,   // NEFT, IMPS, UPI
    pub to_account:     Option<String>,
    pub to_ifsc:        Option<String>,
    pub to_upi:         Option<String>,
    pub ben_name:       String,
    pub the_amount:     f64,
    pub remarks:        Option<String>,
}

#[derive(Deserialize)]
pub struct LoanApplyReq {
    pub session_token: String,
    pub loan_type:     String,
    pub loan_amount:   f64,
    pub tenure:        i32,
    pub form_data:     String,   // JSON string of the full form
}

#[derive(Deserialize)]
pub struct CardActionReq {
    pub session_token: String,
    pub action:        String,   // block, unblock, set-pin, etc
    pub extra_data:    Option<String>,
}

#[derive(Deserialize)]
pub struct ServiceReqBody {
    pub session_token: String,
    pub sr_type:       String,
    pub sr_data:       String,   // JSON of the form fields
}

#[derive(Deserialize)]
pub struct EmiCalcReq {
    pub principal:     f64,
    pub annual_rate:   f64,
    pub tenure_months: i32,
}

#[derive(Deserialize)]
pub struct ProfileUpdateReq {
    pub session_token: String,
    pub field_name:    String,
    pub new_value:     String,
}

#[derive(Deserialize)]
pub struct ChangePwdReq {
    pub session_token:  String,
    pub current_pwd:    String,
    pub new_pwd:        String,
}
