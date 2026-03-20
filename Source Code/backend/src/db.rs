// =========================================================
// db.rs - Database connection pool setup
// Called once at startup, shared across all handlers
// =========================================================

use sqlx::mysql::MySqlPool;
use std::env;

pub async fn connect() -> Result<MySqlPool, sqlx::Error> {
    // Read the DB connection string from environment
    // Format: mysql://user:password@host/dbname
    let db_url = env::var("DATABASE_URL")
        .unwrap_or_else(|_| "mysql://root:nexusbank@mysql:3306/nexusbank".to_string());

    // Create a pool with up to 10 connections
    // This lets multiple requests run at the same time
    let pool = MySqlPool::connect(&db_url).await?;

    log::info!("Database connected successfully");

    Ok(pool)
}

// Run the schema and seed files if tables are empty
// This makes the Docker setup self-contained on first boot
pub async fn seed_if_needed(pool: &MySqlPool) {
    // Check if we already have users loaded
    let row: Option<(i64,)> = sqlx::query_as("SELECT COUNT(*) FROM users")
        .fetch_optional(pool)
        .await
        .unwrap_or(None);

    if let Some((count,)) = row {
        if count > 0 {
            log::info!("Database already has data, skipping seed");
            return;
        }
    }

    // If we got here, tables are empty - need to seed
    // In Docker setup, schema.sql and seed_data.sql are mounted
    // and auto-run by MySQL entrypoint, so this is just a safety check
    log::info!("Database seeding handled by Docker MySQL entrypoint");
}
