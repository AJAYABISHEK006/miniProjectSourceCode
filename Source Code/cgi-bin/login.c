/* =========================================================
   login.c - Two-step login flow (HDFC style)
   Step 1: Enter Customer ID + captcha
   Step 2: Enter password
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../common/html_utils.h"
#include "../common/api_client.h"
#include "../common/session.h"

/* The two stages of our login */
#define STAGE_ENTER_CID  1
#define STAGE_ENTER_PWD  2

static void show_cid_form(const char *error_msg);
static void show_pwd_form(const char *cid, const char *first_name,
                          const char *acc_masked, const char *error_msg);
static int  captcha_ok(int num1, int num2, int user_answer);

int cgiMain() {
    char req_method[8];
    cgiStringValue("REQUEST_METHOD", req_method, 8);

    /* --------------------------------------------------
       GET request: just show the Customer ID form
       -------------------------------------------------- */
    if (strcmp(req_method, "GET") != 0) {
        /* It's a POST - figure out which stage we're in */
        char stage_str[4];
        cgiFormString("stage", stage_str, 4);
        int stage = atoi(stage_str);

        /* ---- STAGE 1: Validate Customer ID ---- */
        if (stage == STAGE_ENTER_CID) {
            char cid[32], captcha_ans[8], num1_str[8], num2_str[8];
            cgiFormString("customer_id",  cid,         32);
            cgiFormString("captcha_ans",  captcha_ans, 8);
            cgiFormString("captcha_num1", num1_str,    8);
            cgiFormString("captcha_num2", num2_str,    8);

            int n1     = atoi(num1_str);
            int n2     = atoi(num2_str);
            int answer = atoi(captcha_ans);

            /* First check the captcha */
            if (!captcha_ok(n1, n2, answer)) {
                show_cid_form("Incorrect security answer. Please try again.");
                return 0;
            }

            if (strlen(cid) < 5) {
                show_cid_form("Please enter a valid Customer ID.");
                return 0;
            }

            /* Ask Rust if this Customer ID exists */
            char json_body[256];
            snprintf(json_body, sizeof(json_body),
                     "{\"customer_id\":\"%s\"}", cid);

            char response[2048];
            int ok = rust_post("/api/auth/validate-customer-id",
                               json_body, response, sizeof(response));

            if (!ok || !resp_is_ok(response)) {
                char err_msg[256];
                resp_get_field(response, "msg", err_msg, sizeof(err_msg));
                if (strlen(err_msg) == 0) {
                    strcpy(err_msg, "Customer ID not found. Please check and try again.");
                }
                show_cid_form(err_msg);
                return 0;
            }

            /* Customer ID is good - get their first name and masked account */
            char first_name[64], acc_masked[32];
            resp_get_field(response, "first_name", first_name, sizeof(first_name));
            resp_get_field(response, "acc_masked",  acc_masked, sizeof(acc_masked));

            /* Show password form */
            show_pwd_form(cid, first_name, acc_masked, NULL);
            return 0;
        }

        /* ---- STAGE 2: Verify Password ---- */
        if (stage == STAGE_ENTER_PWD) {
            char cid[32], password[128];
            cgiFormString("customer_id", cid,      32);
            cgiFormString("password",    password, 128);

            if (strlen(cid) == 0 || strlen(password) == 0) {
                show_cid_form("Something went wrong. Please start again.");
                return 0;
            }

            /* Build login request */
            char json_body[512];
            snprintf(json_body, sizeof(json_body),
                     "{\"customer_id\":\"%s\",\"password\":\"%s\"}", cid, password);

            char response[2048];
            int ok = rust_post("/api/auth/login", json_body, response, sizeof(response));

            if (!ok || !resp_is_ok(response)) {
                /* Get error message from Rust */
                char err_msg[256];
                resp_get_field(response, "msg", err_msg, sizeof(err_msg));
                if (strlen(err_msg) == 0) {
                    strcpy(err_msg, "Login failed. Please try again.");
                }

                /* Show the password form again with the error */
                /* We need the display info again */
                char info_body[128];
                snprintf(info_body, sizeof(info_body),
                         "{\"customer_id\":\"%s\"}", cid);
                char info_resp[2048];
                rust_post("/api/auth/validate-customer-id", info_body,
                          info_resp, sizeof(info_resp));

                char first_name[64], acc_masked[32];
                resp_get_field(info_resp, "first_name", first_name, sizeof(first_name));
                resp_get_field(info_resp, "acc_masked",  acc_masked, sizeof(acc_masked));

                show_pwd_form(cid, first_name, acc_masked, err_msg);
                return 0;
            }

            /* Login success! Get the session token */
            char token[MAX_TOKEN_LEN + 1];
            resp_get_field(response, "token", token, sizeof(token));

            if (strlen(token) == 0) {
                show_cid_form("Login failed. Please try again.");
                return 0;
            }

            /* Set the session cookie and redirect to dashboard */
            cgiHeaderContentType("text/html");
            set_session_cookie(token);
            cgiHeaderLocation("/cgi-bin/dashboard.cgi");
            return 0;
        }
    }

    /* Default: show the Customer ID entry form */
    show_cid_form(NULL);
    return 0;
}

/* ---- Show Stage 1: Customer ID + Captcha ---- */
static void show_cid_form(const char *error_msg) {
    page_start("Login", "");

    fprintf(cgiOut,
        "<div class=\"auth-page\">\n"
        "  <div class=\"auth-left\">\n"
        "    <div class=\"auth-brand\">\n"
        "      <div class=\"auth-logo\">\n"
        "        <span class=\"logo-nx\">Nexus</span><span class=\"logo-bk\">Bank</span>\n"
        "      </div>\n"
        "      <h2>NetBanking Portal</h2>\n"
        "      <p>Secure. Simple. Always available.</p>\n"
        "    </div>\n"
        "    <div class=\"auth-features\">\n"
        "      <div class=\"auth-feat\">&#10003; Two-factor authentication</div>\n"
        "      <div class=\"auth-feat\">&#10003; End-to-end encryption</div>\n"
        "      <div class=\"auth-feat\">&#10003; Real-time fraud protection</div>\n"
        "    </div>\n"
        "  </div>\n"
        "  <div class=\"auth-right\">\n"
        "    <div class=\"auth-card\">\n"
        "      <div class=\"auth-step-label\">Step 1 of 2</div>\n"
        "      <h1 class=\"auth-title\">Login to NetBanking</h1>\n"
        "      <p class=\"auth-subtitle\">Enter your Customer ID to continue</p>\n"
    );

    if (error_msg && strlen(error_msg) > 0) {
        show_error(error_msg);
    }

    /* Generate a random math captcha */
    srand((unsigned int)time(NULL));
    int num1 = rand() % 9 + 1;
    int num2 = rand() % 9 + 1;

    fprintf(cgiOut,
        "      <form class=\"auth-form\" method=\"POST\" action=\"/cgi-bin/login.cgi\">\n"
        "        <input type=\"hidden\" name=\"stage\" value=\"1\">\n"
        "        <input type=\"hidden\" name=\"captcha_num1\" value=\"%d\">\n"
        "        <input type=\"hidden\" name=\"captcha_num2\" value=\"%d\">\n"
        "        <div class=\"field-group\">\n"
        "          <label class=\"field-label\">Customer ID <span class=\"required\">*</span></label>\n"
        "          <input class=\"field-input\" type=\"text\" name=\"customer_id\"\n"
        "                 placeholder=\"e.g. NXB100001\" maxlength=\"20\" required>\n"
        "          <div class=\"field-hint\">Your Customer ID was provided when you opened your account.</div>\n"
        "        </div>\n"
        "        <div class=\"field-group captcha-field\">\n"
        "          <label class=\"field-label\">Security Check <span class=\"required\">*</span></label>\n"
        "          <div class=\"captcha-row\">\n"
        "            <span class=\"captcha-question\">%d + %d = ?</span>\n"
        "            <input class=\"field-input captcha-input\" type=\"number\"\n"
        "                   name=\"captcha_ans\" placeholder=\"Answer\" required>\n"
        "          </div>\n"
        "        </div>\n"
        "        <button class=\"btn btn-primary btn-full\" type=\"submit\">Continue &rarr;</button>\n"
        "      </form>\n"
        "      <div class=\"auth-links\">\n"
        "        <a href=\"/cgi-bin/activate.cgi\">First Time User? Activate NetBanking</a>\n"
        "        <a href=\"/cgi-bin/forgot.cgi\">Forgot Password?</a>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"
        "</div>\n",
        num1, num2, num1, num2
    );

    page_end();
}

/* ---- Show Stage 2: Password Entry ---- */
static void show_pwd_form(const char *cid, const char *first_name,
                          const char *acc_masked, const char *error_msg) {
    page_start("Login - Enter Password", "");

    char safe_name[128], safe_acc[64];
    html_safe(first_name, safe_name, sizeof(safe_name));
    html_safe(acc_masked,  safe_acc,  sizeof(safe_acc));

    fprintf(cgiOut,
        "<div class=\"auth-page\">\n"
        "  <div class=\"auth-left\">\n"
        "    <div class=\"auth-brand\">\n"
        "      <div class=\"auth-logo\">\n"
        "        <span class=\"logo-nx\">Nexus</span><span class=\"logo-bk\">Bank</span>\n"
        "      </div>\n"
        "      <h2>NetBanking Portal</h2>\n"
        "      <p>Secure. Simple. Always available.</p>\n"
        "    </div>\n"
        "  </div>\n"
        "  <div class=\"auth-right\">\n"
        "    <div class=\"auth-card\">\n"
        "      <div class=\"auth-step-label\">Step 2 of 2</div>\n"
        "      <div class=\"welcome-banner\">\n"
        "        <div class=\"welcome-avatar\">%c</div>\n"
        "        <div>\n"
        "          <div class=\"welcome-name\">Welcome back, %s</div>\n"
        "          <div class=\"welcome-acc\">Account: %s</div>\n"
        "        </div>\n"
        "      </div>\n",
        (strlen(safe_name) > 0) ? safe_name[0] : 'U',
        safe_name,
        safe_acc
    );

    if (error_msg && strlen(error_msg) > 0) {
        show_error(error_msg);
    }

    fprintf(cgiOut,
        "      <form class=\"auth-form\" method=\"POST\" action=\"/cgi-bin/login.cgi\">\n"
        "        <input type=\"hidden\" name=\"stage\" value=\"2\">\n"
        "        <input type=\"hidden\" name=\"customer_id\" value=\"%s\">\n"
        /* Password field with show/hide toggle (CSS checkbox trick) */
        "        <div class=\"field-group\">\n"
        "          <label class=\"field-label\">Password (IPIN) <span class=\"required\">*</span></label>\n"
        "          <div class=\"pwd-wrapper\">\n"
        "            <input type=\"checkbox\" id=\"show-pwd\" class=\"pwd-toggle-check\">\n"
        "            <input class=\"field-input pwd-field\" type=\"password\"\n"
        "                   name=\"password\" placeholder=\"Enter your password\" required>\n"
        "            <label class=\"pwd-toggle-btn\" for=\"show-pwd\">\n"
        "              <span class=\"eye-open\">&#128065;</span>\n"
        "              <span class=\"eye-closed\">&#128564;</span>\n"
        "            </label>\n"
        "          </div>\n"
        "        </div>\n"
        "        <button class=\"btn btn-primary btn-full\" type=\"submit\">Login Securely</button>\n"
        "      </form>\n"
        "      <div class=\"auth-links\">\n"
        "        <a href=\"/cgi-bin/login.cgi\">&larr; Not you? Go back</a>\n"
        "        <a href=\"/cgi-bin/forgot.cgi\">Forgot Password?</a>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"
        "</div>\n",
        cid
    );

    page_end();
}

/* Simple captcha check: did user answer correctly? */
static int captcha_ok(int num1, int num2, int user_answer) {
    return (num1 + num2 == user_answer) ? 1 : 0;
}
