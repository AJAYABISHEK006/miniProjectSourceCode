// =========================================================
// main.rs - NexusBank Rust Backend Entry Point
// Starts the Actix-web server and registers all routes
// =========================================================

mod db;
mod models;
mod auth;
mod transfer;
mod loans;
mod cards;
mod services;
mod calculator;
mod profile;

use actix_web::{web, App, HttpServer, middleware};
use dotenv::dotenv;
use std::env;

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    // Load .env file if it exists
    dotenv().ok();

    // Setup basic logging
    env_logger::init_from_env(
        env_logger::Env::default().default_filter_or("info")
    );

    log::info!("Starting NexusBank backend...");

    // Connect to MySQL
    let pool = db::connect().await.expect("Could not connect to database");
    let pool_data = web::Data::new(pool);

    // Get port from env or default to 8080
    let port = env::var("PORT").unwrap_or_else(|_| "8080".to_string());
    let bind_addr = format!("0.0.0.0:{}", port);

    log::info!("Server starting on {}", bind_addr);

    // Start the HTTP server
    HttpServer::new(move || {
        App::new()
            .app_data(pool_data.clone())
            // Log every request
            .wrap(middleware::Logger::default())

            // ---- Auth routes ----
            .route("/api/auth/validate-customer-id", web::post().to(auth::check_customer_id))
            .route("/api/auth/login",                web::post().to(auth::do_login))
            .route("/api/auth/check-customer",       web::post().to(auth::check_for_activation))
            .route("/api/auth/verify-card",          web::post().to(auth::verify_card))
            .route("/api/auth/generate-otp",         web::post().to(auth::make_otp))
            .route("/api/auth/verify-otp",           web::post().to(auth::verify_otp))
            .route("/api/auth/set-password",         web::post().to(auth::set_password))
            .route("/api/auth/reset-password",       web::post().to(auth::reset_password))
            .route("/api/auth/verify-session",       web::get().to(auth::check_session))
            .route("/api/auth/logout",               web::post().to(auth::logout))

            // ---- Dashboard and transfer routes ----
            .route("/api/dashboard/{cid}",           web::get().to(transfer::get_dashboard))
            .route("/api/transfer",                  web::post().to(transfer::do_transfer))
            .route("/api/transactions/{cid}",        web::get().to(transfer::get_transactions))
            .route("/api/transfer/ifsc-lookup",      web::post().to(transfer::ifsc_lookup))

            // ---- Loan routes ----
            .route("/api/loans/apply",               web::post().to(loans::apply_for_loan))
            .route("/api/loans/{cid}",               web::get().to(loans::get_my_loans))

            // ---- Cards routes ----
            .route("/api/cards/{cid}",               web::get().to(cards::get_my_cards))
            .route("/api/cards/action",              web::post().to(cards::card_action))

            // ---- Service request routes ----
            .route("/api/services/submit",           web::post().to(services::raise_sr))

            // ---- EMI Calculator ----
            .route("/api/calculator/emi",            web::post().to(calculator::calc_emi))

            // ---- Profile routes ----
            .route("/api/profile/{cid}",             web::get().to(profile::get_profile))
            .route("/api/profile/update",            web::post().to(profile::update_profile))
            .route("/api/profile/change-password",   web::post().to(profile::change_password))
    })
    .bind(&bind_addr)?
    .run()
    .await
}
