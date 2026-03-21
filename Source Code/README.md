# NexusBank — Online Banking Platform

A complete, professional, fully functional Internet Banking web application modelled on the experience of major Indian bank NetBanking portals. Every form submits, every action reflects in the database, and every page feels and behaves like a live bank website.

| Layer | Technology |
|---|---|
| Frontend | C + CGI (cgic library) — zero JavaScript |
| Styling | Pure CSS (CSS variables, radio-button tabs) |
| Backend API | Rust + Actix Web |
| Database | MySQL 8.0 |
| Server | Apache 2.4 |

---

## Product Goals

- Deliver a fully working Internet Banking experience from login to fund transfer to loan application.
- Provide clean, beginner-friendly navigation with no banking jargon.
- All account data, transaction history, and customer information is stored persistently.
- No broken flows — every user action has a defined outcome, error state, and success state.
- Secure by design — sessions, password hashing, OTP verification on all sensitive actions.

---

## Design Philosophy

The UI/UX follows a clean, minimal, professional aesthetic inspired by the best modern web applications. The design prioritises whitespace, clarity, and ease of use over visual complexity.

| Problem Avoided | Solution Implemented |
|---|---|
| Complex navigation | Simple flat sidebar with clear icons and plain English labels |
| No unified view | Single dashboard showing all account information at a glance |
| Weak personalisation | Greet user by name, show their specific account data everywhere |
| Confusing banking terms | Plain language — 'Send Money' not 'NEFT Remittance' |
| Cluttered interface | Generous whitespace, grouped items, clean card-based UI |
| Hard to find features | Smart search bar at the top of the Service Requests page |

---

## Demo Accounts

### Group A — NetBanking already active (can log in directly)

| Customer ID | Password | Name | Balance |
|---|---|---|---|
| NXB100001 | arjun@123 | Arjun Sharma | ₹4,28,750 |
| NXB100002 | priya@123 | Priya Nair | ₹1,85,500 |
| NXB100003 | ravi@123 | Ravi Kumar | ₹92,300 |

### Group B — Must activate NetBanking first

| Customer ID | Name | Card Number | Expiry | CVV |
|---|---|---|---|---|
| NXB100004 | Karthik Raja | 4111 2233 4455 6604 | 04/26 | 987 |
| NXB100005 | Deepa Menon | 4111 2233 4455 6605 | 07/25 | 246 |
| NXB100006 | Suresh Babu | 4111 2233 4455 6606 | 12/26 | 135 |

> **OTP note:** All OTPs are shown on-screen (demo mode). In production they would be sent via SMS.

---

## Project Structure

```
nexusbank/
├── backend/                  # Rust API source
│   ├── Cargo.toml
│   └── src/
│       ├── main.rs
│       ├── models.rs
│       ├── db.rs
│       ├── auth.rs
│       ├── transfer.rs
│       ├── loans.rs
│       ├── cards.rs
│       ├── services.rs
│       ├── calculator.rs
│       └── profile.rs
│
├── cgi-bin/                  # C CGI source
│   ├── Makefile
│   ├── common/               # Shared headers + helpers
│   │   ├── session.h / .c
│   │   ├── api_client.h / .c
│   │   └── html_utils.h / .c
│   ├── home.c
│   ├── login.c
│   ├── activate.c
│   ├── forgot.c
│   ├── dashboard.c
│   ├── transfer.c
│   ├── transactions.c
│   ├── cards.c
│   ├── loans.c
│   ├── services.c
│   ├── calculator.c
│   ├── profile.c
│   └── logout.c
│
├── database/
│   ├── schema.sql            # All table definitions
│   └── seed_data.sql         # Demo users + transactions
│
└── frontend/
    └── css/
        ├── main.css          # Variables, reset, buttons, forms
        ├── layout.css        # Nav, sidebar, auth layouts
        ├── components.css    # Cards, tables, tabs, calculator
        └── pages.css         # Home page sections
```

---

## Pages & Features

### Public Pages (No Login Required)

**Home Page**
- Top navigation bar with bank logo, Login and Open Account buttons.
- Hero section with headline "Banking Made Simple", tagline, and two call-to-action buttons.
- Services grid with 6 tiles: Savings, Cards, Loans, Investments, Insurance, Transfers.
- Stats section: 50L+ Customers, ₹10,000 Cr+ Deposits, 99.9% Uptime.

**Login — 2-Step Flow**
- Step 1: Customer ID input with math CAPTCHA. Links to Activate and Forgot Password.
- Step 2: Displays "Welcome back, [First Name]" and masked account number. Password with show/hide toggle (CSS only). Locks after 3 failed attempts.

**Activate Internet Banking — 4-Step Flow**
- Step 1: Enter Customer ID.
- Step 2: Verify debit card — 16-digit card number, expiry, CVV.
- Step 3: OTP verification with 5-minute CSS countdown timer.
- Step 4: Set password with live strength indicator (CSS only). Green confirmation on success.

**Forgot Password — 4-Step Flow**
- Step 1: Enter Customer ID.
- Step 2: Enter registered mobile number.
- Step 3: OTP verification.
- Step 4: Set new password with live strength indicator.

---

### Protected Pages (Login Required)

Every protected page checks for a valid session. If the session is missing or expired, the user is immediately redirected to the login page.

**Dashboard**
- Personalised greeting (Good Morning / Afternoon / Evening, [Name]), last login timestamp, and Logout button.
- Account summary card with balance hidden by default — 'Show Balance' toggle using CSS checkbox hack (no JavaScript).
- 6 Quick Action tiles (2×3 grid): Send Money, Transactions, My Cards, Apply for Loan, Service Requests, EMI Calculator.
- Recent Transactions panel: last 5 transactions with date, description, colour-coded amount, and 'View All' link.

**Fund Transfer — 5-Step Flow**
- Step 1: Select type — NEFT (2–4 hours), IMPS (Instant, 24×7), UPI (Instant).
- Step 2: Enter beneficiary details and amount.
- Step 3: Full review summary before confirmation.
- Step 4: OTP verification with 60-second CSS timer.
- Step 5: Result screen — transaction ID, new balance, Download Receipt. On failure — error reason and Try Again.

**Transaction History**
- Filter bar: All / Credits Only / Debits Only, date range.
- Keyword search with page re-render.
- Table: Date & Time, Reference ID, Description, Type badge, Amount (colour-coded), Balance After.
- Pagination: 10 transactions per page. Download Statement button.

**Cards**
- 4 card types: Classic Debit (blue), Millennia Credit (gold), Platinum Credit (black), RuPay Debit (green).
- CSS flip animation (no JavaScript) — front shows masked card number, back shows masked CVV.
- 4 tabs (CSS radio-button hack): Overview, Fees & Charges, Security, Card Management.
- Card Management: Set ATM PIN, Enable/Disable International, Enable/Disable Online Transactions, Set Daily Limit, Report Lost Card — each requires OTP.
- Credit card application: Eligibility → Form → Review → Confirmation with Application ID.

**Loans**

| Loan Type | Max Amount | Max Tenure | Key Fields |
|---|---|---|---|
| Personal Loan | ₹40 Lakhs | 60 months | Purpose dropdown, live EMI preview |
| Home Loan | ₹5 Crore | 30 years | Property type, co-applicant, 2 references |
| Education Loan | ₹20 Lakhs | 15 years | Academic marks, moratorium period, mandatory co-applicant |

All applications generate a unique Application ID (e.g. NXB-PL-2024-001) with a 4-stage status tracker: Applied → Under Review → Approved → Disbursed.

**Service Requests**

| Service | What It Does | Processing Time |
|---|---|---|
| Address Change | Updates registered address | 3–4 working days |
| Update KYC | Submits Aadhaar and PAN for re-verification | 5–7 working days |
| Update Email ID | Changes registered email | Immediate |
| Aadhaar Seeding | Links Aadhaar for DBT benefit credits | 1–2 working days |
| Customer Profile Update | Updates mobile, email, DOB, gender, occupation | Immediate |
| Register for Email Statements | Activates monthly or quarterly email statements | Immediate |
| Debit Card Services | New card, Set ATM PIN, Upgrade Card | 7–10 working days |

Each request generates a unique Service Request ID. OTP is required before submitting any request that modifies account data.

**EMI Calculator**
- Three sliders: Loan Amount (₹10,000 – ₹50,00,000), Interest Rate (5%–30% p.a.), Tenure (6–360 months).
- Results update on every slider change: Monthly EMI, Total Interest, Total Payable.
- CSS donut chart showing principal vs. interest (conic-gradient, no JavaScript).
- Quick presets: Personal Loan 12%, Home Loan 8.5%, Education Loan 9%.

**Profile & Settings — 4-Tab Layout**

| Tab | Contents |
|---|---|
| Personal Details | Full name, DOB, gender, masked PAN, masked Aadhaar — editable after OTP |
| Contact Details | Mobile number, email, home address — each has an Edit button requiring OTP |
| Account Details | Account number, type, IFSC, branch, opening date — read-only |
| Security | Change password (current → new → confirm) and last login info |

---

## Error & Success Messages

### Error Messages

| Trigger | Message |
|---|---|
| Wrong Customer ID | Customer ID not found. Please check and try again. |
| Wrong Password (1st or 2nd attempt) | Incorrect password. [X] attempts remaining. |
| Wrong Password (3rd attempt) | Account locked. Please reset your password to continue. |
| Wrong Card Details | Card details do not match our records. Please check and try again. |
| Wrong OTP | Invalid OTP. Please check and try again. |
| OTP Expired | OTP has expired. Please click Resend to get a new one. |
| Insufficient Balance | Insufficient balance. Your available balance is ₹[X]. |
| Same Account Transfer | You cannot transfer to your own account. |
| Weak Password | Password must have min 8 characters, 1 uppercase, 1 number, 1 special character. |
| Passwords Don't Match | Passwords do not match. Please re-enter. |
| Session Expired | Your session has expired. Please login again. |

### Success Messages

| Action | Message |
|---|---|
| Login | Welcome back, [Name]! Redirecting to dashboard... |
| Internet Banking Activation | Internet Banking activated successfully! You can now login. |
| Password Reset | Password reset successfully! You can now login with your new password. |
| Fund Transfer | ₹[Amount] transferred successfully. Transaction ID: [ID] |
| Loan Application | Application submitted! Your Application ID is [ID]. We will review within 2–4 working days. |
| Service Request | Request submitted. Service Request ID: [SR-ID]. Processing time: [X] working days. |
| Profile Update | Your details have been updated successfully. |
| Card Blocked | Card ending [XXXX] has been blocked. Contact support to unblock. |

---

## API Reference (Rust Backend)

Base URL: `http://rust-backend:8080`

| Method | Path | Purpose |
|---|---|---|
| POST | `/api/auth/validate-customer-id` | Check Customer ID exists |
| POST | `/api/auth/login` | Verify password, create session |
| POST | `/api/auth/check-customer` | Check activation eligibility |
| POST | `/api/auth/verify-card` | Validate debit card for activation |
| POST | `/api/auth/generate-otp` | Generate OTP for any action |
| POST | `/api/auth/verify-otp` | Verify submitted OTP |
| POST | `/api/auth/set-password` | Set password during activation |
| POST | `/api/auth/reset-password` | Reset forgotten password |
| GET  | `/api/auth/verify-session` | Validate session cookie |
| POST | `/api/auth/logout` | Destroy session |
| GET  | `/api/dashboard/{cid}` | Account info + recent transactions |
| POST | `/api/transfer` | Execute fund transfer |
| GET  | `/api/transactions/{cid}` | Full transaction history |
| POST | `/api/transfer/ifsc-lookup` | Bank name from IFSC code |
| POST | `/api/loans/apply` | Submit loan application |
| GET  | `/api/loans/{cid}` | User's loan applications |
| GET  | `/api/cards/{cid}` | User's debit/credit cards |
| POST | `/api/cards/action` | Block / unblock / apply credit |
| POST | `/api/services/submit` | Submit service request |
| POST | `/api/calculator/emi` | Calculate EMI |
| GET  | `/api/profile/{cid}` | Get profile details |
| POST | `/api/profile/update` | Update profile fields |
| POST | `/api/profile/change-password` | Change login password |
| GET  | `/health` | Health check (returns 200 OK) |

---

## Verification Checklist

| Flow to Verify | Expected Result |
|---|---|
| Home page with no session | Public access, no redirect |
| Login with Group A credentials | Dashboard loads with correct data |
| Three consecutive wrong passwords | Account locked message displays |
| Activate Internet Banking (Group B user) | Full 4-step flow completes; login then works |
| Forgot password flow | OTP → reset → login with new password |
| NEFT fund transfer | Balance deducted; transaction in history |
| UPI fund transfer | Balance deducted; transaction in history |
| Transaction filters (credits / debits) | Filtered list renders correctly |
| Personal loan application | Full form, EMI shown, Application ID generated |
| Address change service request | OTP verified; address updated in database |
| EMI calculator | Values update on every slider change |
| Card block action | Card status changes; confirmation shown |
| Profile mobile number update | OTP required; mobile updated |
| Logout | Session cleared; redirect to home |
| Access dashboard URL without login | Redirects to login page |
| All 6 pre-loaded users in database | Present after first application start |
