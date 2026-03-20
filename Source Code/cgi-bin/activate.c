/* =========================================================
   activate.c - NetBanking Activation (4 Steps)
   For Group B users who have an account but no NetBanking
   Step 1: Enter Customer ID
   Step 2: Verify Debit Card
   Step 3: OTP Verification
   Step 4: Set Password
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/html_utils.h"
#include "../common/api_client.h"
#include "../common/session.h"

#define STEP_ENTER_CID   1
#define STEP_VERIFY_CARD 2
#define STEP_OTP_CHECK   3
#define STEP_SET_PASSWORD 4
#define STEP_SUCCESS     5

static const char *step_names[] = {"Customer ID", "Verify Card", "OTP", "Set Password"};

static void show_step1(const char *err);
static void show_step2(const char *cid, const char *err);
static void show_step3(const char *cid, const char *otp_demo, const char *err);
static void show_step4(const char *cid, const char *err);
static void show_success(const char *cid);

int cgiMain() {
    char req_method[8];
    cgiStringValue("REQUEST_METHOD", req_method, 8);

    if (strcmp(req_method, "GET") != 0) {
        char step_str[4];
        cgiFormString("step", step_str, 4);
        int cur_step = atoi(step_str);

        /* ---- STEP 1: Check if Customer ID is valid and inactive ---- */
        if (cur_step == STEP_ENTER_CID) {
            char cid[32];
            cgiFormString("customer_id", cid, 32);

            if (strlen(cid) < 5) {
                show_step1("Please enter a valid Customer ID.");
                return 0;
            }

            char json_body[128];
            snprintf(json_body, sizeof(json_body), "{\"customer_id\":\"%s\"}", cid);

            char resp[2048];
            int ok = rust_post("/api/auth/check-customer", json_body, resp, sizeof(resp));

            if (!ok || !resp_is_ok(resp)) {
                char err[256];
                resp_get_field(resp, "msg", err, sizeof(err));
                if (strlen(err) == 0) strcpy(err, "Customer ID not found.");
                show_step1(err);
                return 0;
            }

            show_step2(cid, NULL);
            return 0;
        }

        /* ---- STEP 2: Verify debit card details ---- */
        if (cur_step == STEP_VERIFY_CARD) {
            char cid[32], card_num[20], card_exp[8], cvv[8];
            cgiFormString("customer_id", cid,      32);
            cgiFormString("card_number", card_num, 20);
            cgiFormString("card_expiry", card_exp, 8);
            cgiFormString("cvv",         cvv,      8);

            if (strlen(cid) == 0 || strlen(card_num) < 12) {
                show_step2(cid, "Please fill all card details correctly.");
                return 0;
            }

            char json_body[512];
            snprintf(json_body, sizeof(json_body),
                     "{\"customer_id\":\"%s\",\"card_number\":\"%s\","
                     "\"card_expiry\":\"%s\",\"cvv\":\"%s\"}",
                     cid, card_num, card_exp, cvv);

            char resp[2048];
            int ok = rust_post("/api/auth/verify-card", json_body, resp, sizeof(resp));

            if (!ok || !resp_is_ok(resp)) {
                char err[256];
                resp_get_field(resp, "msg", err, sizeof(err));
                if (strlen(err) == 0)
                    strcpy(err, "Card details do not match our records.");
                show_step2(cid, err);
                return 0;
            }

            /* Card verified - now generate and send OTP */
            char otp_body[128];
            snprintf(otp_body, sizeof(otp_body),
                     "{\"customer_id\":\"%s\",\"purpose\":\"activation\"}", cid);

            char otp_resp[512];
            rust_post("/api/auth/generate-otp", otp_body, otp_resp, sizeof(otp_resp));

            /* Get the OTP for demo display */
            char demo_otp[16];
            resp_get_field(otp_resp, "otp_for_demo", demo_otp, sizeof(demo_otp));

            show_step3(cid, demo_otp, NULL);
            return 0;
        }

        /* ---- STEP 3: Verify the OTP ---- */
        if (cur_step == STEP_OTP_CHECK) {
            char cid[32], otp_code[16];
            cgiFormString("customer_id", cid,      32);
            cgiFormString("otp_code",    otp_code, 16);

            if (strlen(otp_code) != 6) {
                show_step3(cid, "", "Please enter the 6-digit OTP.");
                return 0;
            }

            char json_body[256];
            snprintf(json_body, sizeof(json_body),
                     "{\"customer_id\":\"%s\",\"otp_entered\":\"%s\","
                     "\"purpose\":\"activation\"}",
                     cid, otp_code);

            char resp[2048];
            int ok = rust_post("/api/auth/verify-otp", json_body, resp, sizeof(resp));

            if (!ok || !resp_is_ok(resp)) {
                char err[256];
                resp_get_field(resp, "msg", err, sizeof(err));
                if (strlen(err) == 0)
                    strcpy(err, "Invalid OTP. Please check and try again.");
                show_step3(cid, "", err);
                return 0;
            }

            /* OTP is good - show set password form */
            show_step4(cid, NULL);
            return 0;
        }

        /* ---- STEP 4: Set the password ---- */
        if (cur_step == STEP_SET_PASSWORD) {
            char cid[32], new_pwd[128], confirm_pwd[128];
            cgiFormString("customer_id",  cid,         32);
            cgiFormString("new_password", new_pwd,     128);
            cgiFormString("confirm_pwd",  confirm_pwd, 128);

            /* Basic password validation in C before calling Rust */
            if (strlen(new_pwd) < 8) {
                show_step4(cid, "Password must be at least 8 characters.");
                return 0;
            }

            if (strcmp(new_pwd, confirm_pwd) != 0) {
                show_step4(cid, "Passwords do not match. Please re-enter.");
                return 0;
            }

            /* Call Rust to hash and save the password */
            char json_body[512];
            snprintf(json_body, sizeof(json_body),
                     "{\"customer_id\":\"%s\",\"new_password\":\"%s\"}",
                     cid, new_pwd);

            char resp[1024];
            int ok = rust_post("/api/auth/set-password", json_body, resp, sizeof(resp));

            if (!ok || !resp_is_ok(resp)) {
                show_step4(cid, "Could not save password. Please try again.");
                return 0;
            }

            /* All done! Show success screen */
            show_success(cid);
            return 0;
        }
    }

    /* Default: show Step 1 */
    show_step1(NULL);
    return 0;
}

static void show_step1(const char *err) {
    page_start("Activate NetBanking", "");
    render_public_nav();

    fprintf(cgiOut,
        "<div class=\"activate-page\">\n"
        "  <div class=\"activate-card\">\n"
        "    <h1>Activate NetBanking</h1>\n"
        "    <p class=\"page-subtitle\">\n"
        "      Already a NexusBank customer? Activate your NetBanking access here.\n"
        "    </p>\n"
    );

    render_steps(4, 1, step_names);

    if (err && strlen(err) > 0) show_error(err);

    fprintf(cgiOut,
        "    <form method=\"POST\" action=\"/cgi-bin/activate.cgi\">\n"
        "      <input type=\"hidden\" name=\"step\" value=\"1\">\n"
        "      <div class=\"field-group\">\n"
        "        <label class=\"field-label\">Customer ID <span class=\"required\">*</span></label>\n"
        "        <input class=\"field-input\" type=\"text\" name=\"customer_id\"\n"
        "               placeholder=\"e.g. NXB100006\" maxlength=\"20\" required>\n"
        "        <div class=\"field-hint\">\n"
        "          Your Customer ID is on your passbook or the letter you received when you opened your account.\n"
        "        </div>\n"
        "      </div>\n"
        "      <button class=\"btn btn-primary btn-full\" type=\"submit\">Continue &rarr;</button>\n"
        "    </form>\n"
        "    <div class=\"auth-links\">\n"
        "      <a href=\"/cgi-bin/login.cgi\">&larr; Already activated? Login</a>\n"
        "    </div>\n"
        "  </div>\n"
        "</div>\n"
    );

    page_end();
}

static void show_step2(const char *cid, const char *err) {
    page_start("Activate - Verify Card", "");
    render_public_nav();

    fprintf(cgiOut,
        "<div class=\"activate-page\">\n"
        "  <div class=\"activate-card\">\n"
        "    <h1>Verify Your Debit Card</h1>\n"
        "    <p class=\"page-subtitle\">Enter your NexusBank debit card details to verify your identity.</p>\n"
    );

    render_steps(4, 2, step_names);

    if (err && strlen(err) > 0) show_error(err);

    fprintf(cgiOut,
        "    <form method=\"POST\" action=\"/cgi-bin/activate.cgi\">\n"
        "      <input type=\"hidden\" name=\"step\" value=\"2\">\n"
        "      <input type=\"hidden\" name=\"customer_id\" value=\"%s\">\n"
        "      <div class=\"field-group\">\n"
        "        <label class=\"field-label\">16-digit Card Number <span class=\"required\">*</span></label>\n"
        "        <input class=\"field-input\" type=\"text\" name=\"card_number\"\n"
        "               placeholder=\"XXXX XXXX XXXX XXXX\" maxlength=\"19\" required>\n"
        "      </div>\n"
        "      <div class=\"field-row\">\n"
        "        <div class=\"field-group\">\n"
        "          <label class=\"field-label\">Expiry (MM/YY) <span class=\"required\">*</span></label>\n"
        "          <input class=\"field-input\" type=\"text\" name=\"card_expiry\"\n"
        "                 placeholder=\"MM/YY\" maxlength=\"5\" required>\n"
        "        </div>\n"
        "        <div class=\"field-group\">\n"
        "          <label class=\"field-label\">CVV <span class=\"required\">*</span></label>\n"
        "          <input class=\"field-input\" type=\"password\" name=\"cvv\"\n"
        "                 placeholder=\"3 digits\" maxlength=\"3\" required>\n"
        "        </div>\n"
        "      </div>\n"
        "      <button class=\"btn btn-primary btn-full\" type=\"submit\">Verify Card &rarr;</button>\n"
        "    </form>\n"
        "  </div>\n"
        "</div>\n",
        cid
    );

    page_end();
}

static void show_step3(const char *cid, const char *otp_demo, const char *err) {
    page_start("Activate - OTP Verification", "");
    render_public_nav();

    fprintf(cgiOut,
        "<div class=\"activate-page\">\n"
        "  <div class=\"activate-card\">\n"
        "    <h1>OTP Verification</h1>\n"
        "    <p class=\"page-subtitle\">An OTP has been sent to your registered mobile number.</p>\n"
    );

    render_steps(4, 3, step_names);

    if (err && strlen(err) > 0) show_error(err);

    /* Show the OTP for demo purposes */
    if (otp_demo && strlen(otp_demo) > 0) {
        fprintf(cgiOut,
            "    <div class=\"demo-otp-box\">\n"
            "      <span class=\"demo-label\">Demo OTP (For testing only):</span>\n"
            "      <strong class=\"demo-otp-val\">%s</strong>\n"
            "    </div>\n",
            otp_demo
        );
    }

    fprintf(cgiOut,
        "    <form method=\"POST\" action=\"/cgi-bin/activate.cgi\">\n"
        "      <input type=\"hidden\" name=\"step\" value=\"3\">\n"
        "      <input type=\"hidden\" name=\"customer_id\" value=\"%s\">\n"
        "      <div class=\"field-group\">\n"
        "        <label class=\"field-label\">Enter OTP <span class=\"required\">*</span></label>\n"
        "        <input class=\"field-input otp-big\" type=\"text\" name=\"otp_code\"\n"
        "               placeholder=\"6-digit OTP\" maxlength=\"6\" required>\n"
        "      </div>\n"
        /* CSS countdown ring - no JavaScript */
        "      <div class=\"otp-countdown\">\n"
        "        <div class=\"countdown-ring\"></div>\n"
        "        <span class=\"countdown-text\">Valid for 5 minutes</span>\n"
        "      </div>\n"
        "      <button class=\"btn btn-primary btn-full\" type=\"submit\">Verify OTP &rarr;</button>\n"
        "    </form>\n"
        "    <div class=\"auth-links\">\n"
        "      <a href=\"/cgi-bin/activate.cgi\">Resend OTP (restart)</a>\n"
        "    </div>\n"
        "  </div>\n"
        "</div>\n",
        cid
    );

    page_end();
}

static void show_step4(const char *cid, const char *err) {
    page_start("Activate - Set Password", "");
    render_public_nav();

    fprintf(cgiOut,
        "<div class=\"activate-page\">\n"
        "  <div class=\"activate-card\">\n"
        "    <h1>Set Your Password</h1>\n"
        "    <p class=\"page-subtitle\">Choose a strong password for your NetBanking account.</p>\n"
    );

    render_steps(4, 4, step_names);

    if (err && strlen(err) > 0) show_error(err);

    fprintf(cgiOut,
        "    <div class=\"pwd-rules\">\n"
        "      <p class=\"rules-title\">Password must have:</p>\n"
        "      <ul>\n"
        "        <li>Minimum 8 characters</li>\n"
        "        <li>At least 1 uppercase letter</li>\n"
        "        <li>At least 1 number</li>\n"
        "        <li>At least 1 special character (e.g. @, #, $)</li>\n"
        "      </ul>\n"
        "    </div>\n"
        "    <form method=\"POST\" action=\"/cgi-bin/activate.cgi\">\n"
        "      <input type=\"hidden\" name=\"step\" value=\"4\">\n"
        "      <input type=\"hidden\" name=\"customer_id\" value=\"%s\">\n"
        /* CSS password strength indicator using :has() selector */
        "      <div class=\"field-group\">\n"
        "        <label class=\"field-label\">New Password <span class=\"required\">*</span></label>\n"
        "        <div class=\"pwd-wrapper\">\n"
        "          <input type=\"checkbox\" id=\"show-pwd1\" class=\"pwd-toggle-check\">\n"
        "          <input class=\"field-input pwd-field\" type=\"password\"\n"
        "                 id=\"new-pwd\" name=\"new_password\"\n"
        "                 placeholder=\"Enter new password\" minlength=\"8\" required>\n"
        "          <label class=\"pwd-toggle-btn\" for=\"show-pwd1\">&#128065;</label>\n"
        "        </div>\n"
        /* Strength meter powered by pure CSS */
        "        <div class=\"strength-meter\">\n"
        "          <div class=\"strength-bar\"></div>\n"
        "          <div class=\"strength-bar\"></div>\n"
        "          <div class=\"strength-bar\"></div>\n"
        "          <div class=\"strength-bar\"></div>\n"
        "        </div>\n"
        "        <div class=\"strength-label\">Password strength: <span>Weak</span></div>\n"
        "      </div>\n"
        "      <div class=\"field-group\">\n"
        "        <label class=\"field-label\">Confirm Password <span class=\"required\">*</span></label>\n"
        "        <input class=\"field-input\" type=\"password\" name=\"confirm_pwd\"\n"
        "               placeholder=\"Re-enter password\" required>\n"
        "      </div>\n"
        "      <button class=\"btn btn-primary btn-full\" type=\"submit\">Activate NetBanking</button>\n"
        "    </form>\n"
        "  </div>\n"
        "</div>\n",
        cid
    );

    page_end();
}

static void show_success(const char *cid) {
    page_start("NetBanking Activated", "");
    render_public_nav();

    char safe_cid[32];
    html_safe(cid, safe_cid, sizeof(safe_cid));

    fprintf(cgiOut,
        "<div class=\"activate-page\">\n"
        "  <div class=\"activate-card success-card\">\n"
        "    <div class=\"success-icon\">&#10003;</div>\n"
        "    <h1>NetBanking Activated!</h1>\n"
        "    <p>Your NexusBank NetBanking has been successfully activated.</p>\n"
        "    <div class=\"success-details\">\n"
        "      <div class=\"detail-row\">\n"
        "        <span>Customer ID</span>\n"
        "        <strong>%s</strong>\n"
        "      </div>\n"
        "      <div class=\"detail-row\">\n"
        "        <span>Status</span>\n"
        "        <strong class=\"text-success\">Active</strong>\n"
        "      </div>\n"
        "    </div>\n"
        "    <a href=\"/cgi-bin/login.cgi\" class=\"btn btn-primary btn-full\">Login Now</a>\n"
        "  </div>\n"
        "</div>\n",
        safe_cid
    );

    page_end();
}
