// =========================================================
// calculator.rs - EMI calculation logic
// Formula: EMI = P * r * (1+r)^n / ((1+r)^n - 1)
// =========================================================

use actix_web::{web, HttpResponse};
use crate::models::*;

pub async fn calc_emi(
    body: web::Json<EmiCalcReq>,
) -> HttpResponse {
    let principal  = body.principal;
    let ann_rate   = body.annual_rate;
    let months     = body.tenure_months as f64;

    if principal <= 0.0 || ann_rate <= 0.0 || months <= 0.0 {
        return HttpResponse::Ok().json(ApiResp::fail("Please enter valid values"));
    }

    // Convert annual rate to monthly rate
    let monthly_rate = ann_rate / (12.0 * 100.0);

    // EMI formula
    let factor = (1.0 + monthly_rate).powf(months);
    let emi    = principal * monthly_rate * factor / (factor - 1.0);

    let total_amount   = emi * months;
    let total_interest = total_amount - principal;

    HttpResponse::Ok().json(ApiResp::success(serde_json::json!({
        "monthly_emi":     format!("{:.2}", emi),
        "total_interest":  format!("{:.2}", total_interest),
        "total_payable":   format!("{:.2}", total_amount),
        "principal":       format!("{:.2}", principal),
    })))
}
