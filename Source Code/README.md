# NexusBank — College Banking Website

A full-stack internet banking simulation modeled on HDFC NetBanking.

| Layer | Technology |
|---|---|
| Frontend | C + CGI (cgic library) — zero JavaScript |
| Styling | Pure CSS (CSS variables, radio-button tabs) |
| Backend API | Rust + Actix Web |
| Database | MySQL 8.0 |
| Server | Apache 2.4 inside Docker |
| Deployment | AWS EC2 + Docker Compose |

---

## Demo Accounts

### Group A — NetBanking already active (can log in directly)

| Customer ID | Password | Name | Balance |
|---|---|---|---|
| NXB100001 | arjun@123 | Arjun Sharma | ₹4,28,750 |
| NXB100002 | priya@123 | Priya Nair | ₹1,85,500 |
| NXB100003 | ravi@123 | Ravi Kumar | ₹92,300 |

### Group B — Must activate NetBanking first

| Customer ID | Debit Card | Expiry | CVV |
|---|---|---|---|
| NXB100006 | 4111223344556606 | 04/26 | 987 |
| NXB100007 | 4111223344556607 | 07/27 | 456 |
| NXB100008 | 4111223344556608 | 11/25 | 321 |

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
├── frontend/
│   └── css/
│       ├── main.css          # Variables, reset, buttons, forms
│       ├── layout.css        # Nav, sidebar, auth layouts
│       ├── components.css    # Cards, tables, tabs, calculator
│       └── pages.css         # Home page sections
│
├── docker/
│   └── apache-nexusbank.conf # Apache virtual host
│
├── Dockerfile.rust           # Rust build image
├── Dockerfile.cgi            # Apache + cgic image
└── docker-compose.yml        # Orchestration
```

---

## Deploying on AWS EC2 (Step-by-Step)

### Step 1 — Launch an EC2 instance

1. Log in to [AWS Console](https://console.aws.amazon.com)
2. Go to **EC2 → Launch Instance**
3. Choose **Ubuntu Server 24.04 LTS** (free tier eligible)
4. Instance type: **t2.micro** (free tier) or **t3.small** (recommended)
5. Create or select a key pair — download the `.pem` file
6. Under **Network settings → Edit**, add these inbound rules to the security group:

   | Type | Port | Source |
   |---|---|---|
   | SSH | 22 | My IP |
   | HTTP | 80 | Anywhere (0.0.0.0/0) |

7. Click **Launch Instance**

---

### Step 2 — Connect to your instance

```bash
# Replace with your key file and EC2 public IP
chmod 400 your-key.pem
ssh -i your-key.pem ubuntu@YOUR-EC2-PUBLIC-IP
```

---

### Step 3 — Install Docker on the EC2 instance

```bash
# Update packages
sudo apt update && sudo apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com | sudo sh

# Add your user to the docker group (no sudo needed after this)
sudo usermod -aG docker ubuntu

# Apply group change (or log out and back in)
newgrp docker

# Install Docker Compose plugin
sudo apt install -y docker-compose-plugin

# Verify
docker --version
docker compose version
```

---

### Step 4 — Upload the project files

**Option A — Using scp (from your local machine):**
```bash
# Zip the project folder on your local machine first
zip -r nexusbank.zip nexusbank/

# Upload to EC2
scp -i your-key.pem nexusbank.zip ubuntu@YOUR-EC2-PUBLIC-IP:~
```

Then on the EC2 instance:
```bash
unzip nexusbank.zip
cd nexusbank
```

**Option B — Using git:**
```bash
# On EC2
sudo apt install -y git
git clone https://github.com/YOUR-USERNAME/nexusbank.git
cd nexusbank
```

---

### Step 5 — Build and start all containers

```bash
# Inside the nexusbank/ directory on EC2
docker compose up --build -d
```

This command:
- Builds the Rust backend image (compiles Rust code)
- Builds the Apache + CGI image (compiles all C files)
- Pulls MySQL 8.0
- Creates a Docker network so containers talk to each other
- Auto-runs `schema.sql` and `seed_data.sql` on first MySQL boot
- Starts everything in the background (`-d`)

**First build takes 3–5 minutes** (downloading base images, compiling Rust).

---

### Step 6 — Open the website

Visit in your browser:
```
http://YOUR-EC2-PUBLIC-IP
```

That's it! The site is live.

---

## Useful Commands

```bash
# View running containers
docker compose ps

# Watch live logs from all services
docker compose logs -f

# Watch logs from one service
docker compose logs -f rust-backend
docker compose logs -f apache-cgi
docker compose logs -f mysql

# Restart a single service (e.g. after code changes)
docker compose up --build -d apache-cgi

# Stop everything
docker compose down

# Stop and delete the database (full reset)
docker compose down -v

# Open a MySQL shell
docker exec -it nexusbank-mysql mysql -u nexusapp -pnexusapp2024 nexusbank

# Open a shell inside the Apache container
docker exec -it nexusbank-apache bash
```

---

## Making Code Changes

### Changing C CGI files:
```bash
# Edit the file
nano cgi-bin/transfer.c

# Rebuild only the Apache container
docker compose up --build -d apache-cgi
```

### Changing Rust backend:
```bash
# Edit the file
nano backend/src/transfer.rs

# Rebuild only the Rust container
docker compose up --build -d rust-backend
```

### Changing CSS:
CSS files are served directly from the `frontend/` folder which is copied into the container at build time. After editing:
```bash
docker compose up --build -d apache-cgi
```

---

## Features

| Feature | Details |
|---|---|
| Login | 2-step (Customer ID → Password), math CAPTCHA, wrong-attempt lockout after 3 tries |
| Activate NetBanking | 4-step: verify identity → debit card → OTP → set password |
| Forgot Password | 4-step: Customer ID → mobile verify → OTP → new password |
| Dashboard | Account balance (CSS show/hide), quick action tiles, recent transactions |
| Fund Transfer | NEFT / IMPS / UPI, IFSC lookup, OTP confirmation |
| Transactions | Full history with filter by type, date, keyword + pagination |
| Cards | View debit cards, apply for credit card, block/unblock |
| Loans | Apply for Personal / Home / Education loans |
| Service Requests | 8 types: address change, cheque book, statement, etc. |
| EMI Calculator | Sliders + presets, Rust-powered formula, CSS donut chart |
| Profile | View & edit personal/contact details, change password |
| Security | bcrypt passwords, HttpOnly session cookies, OTP on all actions |

---

## API Reference (Rust Backend)

Base URL: `http://rust-backend:8080` (internal Docker network only)

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

## Troubleshooting

**Site not loading:**
```bash
docker compose ps          # Check all containers are "Up"
docker compose logs apache-cgi   # Look for Apache errors
```

**Database not connecting:**
```bash
docker compose logs mysql  # Check MySQL started cleanly
docker compose logs rust-backend  # Check connection errors
```

**"Permission denied" on CGI scripts:**
```bash
docker exec -it nexusbank-apache ls -la /usr/lib/cgi-bin/nexusbank/
# All files should be executable (-rwxr-xr-x)
```

**Full reset (start fresh):**
```bash
docker compose down -v     # Deletes MySQL data volume too
docker compose up --build -d
```
