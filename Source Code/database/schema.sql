-- ============================================================
-- NexusBank Database Schema
-- Author: Ajay
-- Description: All tables for the banking portal
-- ============================================================

CREATE DATABASE IF NOT EXISTS nexusbank;
USE nexusbank;

-- Users table - stores all customer personal info
CREATE TABLE IF NOT EXISTS users (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    customer_id     VARCHAR(20) UNIQUE NOT NULL,
    full_name       VARCHAR(100) NOT NULL,
    dob             DATE NOT NULL,
    gender          VARCHAR(10),
    pan_number      VARCHAR(10),
    aadhaar         VARCHAR(12),
    mobile          VARCHAR(10) NOT NULL,
    email           VARCHAR(100),
    house_addr      VARCHAR(200),
    city            VARCHAR(50),
    state_name      VARCHAR(50),
    pin_code        VARCHAR(6),
    pwd_hash        VARCHAR(255),
    net_active      TINYINT(1) DEFAULT 0,
    created_on      TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Account details - balance, account number etc
CREATE TABLE IF NOT EXISTS accounts (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    customer_id     VARCHAR(20) NOT NULL,
    acc_number      VARCHAR(20) UNIQUE NOT NULL,
    acc_type        ENUM('Savings','Salary','Current') NOT NULL,
    ifsc_code       VARCHAR(11) NOT NULL,
    branch_name     VARCHAR(100),
    balance         DECIMAL(15,2) DEFAULT 0.00,
    opened_on       DATE,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Debit card info
CREATE TABLE IF NOT EXISTS debit_cards (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    customer_id     VARCHAR(20) NOT NULL,
    card_number     VARCHAR(16) NOT NULL,
    card_expiry     VARCHAR(5) NOT NULL,
    cvv_hash        VARCHAR(255) NOT NULL,
    card_type       VARCHAR(30),
    card_status     ENUM('Active','Blocked') DEFAULT 'Active',
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- All transactions - credits and debits
CREATE TABLE IF NOT EXISTS transactions (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    customer_id     VARCHAR(20) NOT NULL,
    txn_time        DATETIME NOT NULL,
    description     VARCHAR(255),
    txn_type        ENUM('Credit','Debit') NOT NULL,
    amount          DECIMAL(15,2) NOT NULL,
    bal_after       DECIMAL(15,2) NOT NULL,
    ref_id          VARCHAR(50),
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Loan applications
CREATE TABLE IF NOT EXISTS loan_apps (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    app_id          VARCHAR(30) UNIQUE NOT NULL,
    customer_id     VARCHAR(20) NOT NULL,
    loan_type       VARCHAR(30) NOT NULL,
    loan_amount     DECIMAL(15,2) NOT NULL,
    tenure_months   INT NOT NULL,
    app_status      ENUM('Under Review','Approved','Rejected','Disbursed') DEFAULT 'Under Review',
    form_data       TEXT,
    applied_on      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Service requests raised by customers
CREATE TABLE IF NOT EXISTS service_reqs (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    sr_number       VARCHAR(30) UNIQUE NOT NULL,
    customer_id     VARCHAR(20) NOT NULL,
    sr_type         VARCHAR(50) NOT NULL,
    sr_data         TEXT,
    sr_status       ENUM('Pending','Processing','Completed') DEFAULT 'Pending',
    raised_on       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Login sessions
CREATE TABLE IF NOT EXISTS sessions (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    token           VARCHAR(100) UNIQUE NOT NULL,
    customer_id     VARCHAR(20) NOT NULL,
    started_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ends_at         TIMESTAMP NOT NULL,
    is_live         TINYINT(1) DEFAULT 1,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Temporary OTP store
CREATE TABLE IF NOT EXISTS otp_codes (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    customer_id     VARCHAR(20) NOT NULL,
    the_otp         VARCHAR(6) NOT NULL,
    for_what        VARCHAR(30) NOT NULL,
    made_at         TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    good_till       TIMESTAMP NOT NULL,
    already_used    TINYINT(1) DEFAULT 0,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Credit card applications
CREATE TABLE IF NOT EXISTS card_apps (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    app_id          VARCHAR(30) UNIQUE NOT NULL,
    customer_id     VARCHAR(20) NOT NULL,
    card_type       VARCHAR(50) NOT NULL,
    app_status      ENUM('Under Processing','Approved','Dispatched') DEFAULT 'Under Processing',
    applied_on      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);

-- Wrong password attempt tracker
CREATE TABLE IF NOT EXISTS login_attempts (
    id              INT AUTO_INCREMENT PRIMARY KEY,
    customer_id     VARCHAR(20) NOT NULL,
    bad_attempts    INT DEFAULT 0,
    locked_out      TINYINT(1) DEFAULT 0,
    last_try        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (customer_id) REFERENCES users(customer_id)
);
