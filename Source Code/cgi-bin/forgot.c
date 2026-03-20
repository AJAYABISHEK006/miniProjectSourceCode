/* =========================================================
   forgot.c - Forgot Password (4-step flow)
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/html_utils.h"
#include "../common/api_client.h"

static const char *steps[] = {"Customer ID", "Mobile", "OTP", "New Password"};

int cgiMain() {
    char req_method[8];
    cgiStringValue("REQUEST_METHOD", req_method, 8);

    if (strcmp(req_method, "GET") != 0) {
        char step_str[4];
        cgiFormString("step", step_str, 4);
        int cur = atoi(step_str);

        if (cur == 1) {
            /* Validate Customer ID */
            char cid[32];
            cgiFormString("customer_id", cid, 32);

            char jbody[128];
            snprintf(jbody, sizeof(jbody), "{\"customer_id\":\"%s\"}", cid);

            char resp[1024];
            int ok = rust_post("/api/auth/validate-customer-id", jbody, resp, sizeof(resp));

            if (!ok || !resp_is_ok(resp)) {
                goto show_step1;
            }

            /* Show step 2: enter mobile */
            page_start("Forgot Password - Step 2", "");
            render_public_nav();
            fprintf(cgiOut,
                "<div class=\"activate-page\"><div class=\"activate-card\">"
                "<h1>Reset Password</h1><p class=\"page-subtitle\">Verify your identity</p>"
            );
            render_steps(4, 2, steps);
            fprintf(cgiOut,
                "<form method=\"POST\" action=\"/cgi-bin/forgot.cgi\">"
                "<input type=\"hidden\" name=\"step\" value=\"2\">"
                "<input type=\"hidden\" name=\"customer_id\" value=\"%s\">"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Registered Mobile Number</label>"
                "<input class=\"field-input\" type=\"tel\" name=\"mobile\""
                "       placeholder=\"10-digit mobile number\" maxlength=\"10\" required>"
                "</div>"
                "<button class=\"btn btn-primary btn-full\" type=\"submit\">Send OTP</button>"
                "</form></div></div>",
                cid
            );
            page_end();
            return 0;
        }

        if (cur == 2) {
            /* Verify mobile and send OTP */
            char cid[32], mobile[16];
            cgiFormString("customer_id", cid,    32);
            cgiFormString("mobile",      mobile, 16);

            char otp_body[128];
            snprintf(otp_body, sizeof(otp_body),
                     "{\"customer_id\":\"%s\",\"purpose\":\"reset\"}", cid);

            char otp_resp[512];
            rust_post("/api/auth/generate-otp", otp_body, otp_resp, sizeof(otp_resp));

            char demo_otp[16];
            resp_get_field(otp_resp, "otp_for_demo", demo_otp, sizeof(demo_otp));

            /* Show step 3: enter OTP */
            page_start("Forgot Password - OTP", "");
            render_public_nav();
            fprintf(cgiOut,
                "<div class=\"activate-page\"><div class=\"activate-card\">"
                "<h1>Reset Password</h1><p class=\"page-subtitle\">Enter OTP</p>"
            );
            render_steps(4, 3, steps);
            if (strlen(demo_otp) > 0) {
                fprintf(cgiOut,
                    "<div class=\"demo-otp-box\">"
                    "<span class=\"demo-label\">Demo OTP:</span>"
                    "<strong class=\"demo-otp-val\">%s</strong>"
                    "</div>", demo_otp
                );
            }
            fprintf(cgiOut,
                "<form method=\"POST\" action=\"/cgi-bin/forgot.cgi\">"
                "<input type=\"hidden\" name=\"step\" value=\"3\">"
                "<input type=\"hidden\" name=\"customer_id\" value=\"%s\">"
                "<input type=\"hidden\" name=\"mobile\" value=\"%s\">"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Enter OTP</label>"
                "<input class=\"field-input otp-big\" type=\"text\" name=\"otp_code\""
                "       placeholder=\"6-digit OTP\" maxlength=\"6\" required>"
                "</div>"
                "<button class=\"btn btn-primary btn-full\" type=\"submit\">Verify OTP</button>"
                "</form></div></div>",
                cid, mobile
            );
            page_end();
            return 0;
        }

        if (cur == 3) {
            /* Verify OTP */
            char cid[32], mobile[16], otp[16];
            cgiFormString("customer_id", cid,    32);
            cgiFormString("mobile",      mobile, 16);
            cgiFormString("otp_code",    otp,    16);

            char jbody[256];
            snprintf(jbody, sizeof(jbody),
                     "{\"customer_id\":\"%s\",\"otp_entered\":\"%s\","
                     "\"purpose\":\"reset\"}", cid, otp);

            char resp[1024];
            int ok = rust_post("/api/auth/verify-otp", jbody, resp, sizeof(resp));

            if (!ok || !resp_is_ok(resp)) {
                /* Show error and come back to OTP step */
                page_start("Forgot Password - OTP Error", "");
                render_public_nav();
                fprintf(cgiOut,
                    "<div class=\"activate-page\"><div class=\"activate-card\">"
                    "<h1>Reset Password</h1>"
                );
                render_steps(4, 3, steps);
                show_error("Invalid OTP. Please check and try again.");
                fprintf(cgiOut,
                    "<form method=\"POST\" action=\"/cgi-bin/forgot.cgi\">"
                    "<input type=\"hidden\" name=\"step\" value=\"3\">"
                    "<input type=\"hidden\" name=\"customer_id\" value=\"%s\">"
                    "<input type=\"hidden\" name=\"mobile\" value=\"%s\">"
                    "<div class=\"field-group\">"
                    "<label class=\"field-label\">Enter OTP</label>"
                    "<input class=\"field-input otp-big\" type=\"text\" name=\"otp_code\""
                    "       placeholder=\"6-digit OTP\" maxlength=\"6\" required>"
                    "</div>"
                    "<button class=\"btn btn-primary btn-full\" type=\"submit\">Verify OTP</button>"
                    "</form></div></div>",
                    cid, mobile
                );
                page_end();
                return 0;
            }

            /* Show step 4: set new password */
            page_start("Forgot Password - Set New Password", "");
            render_public_nav();
            fprintf(cgiOut,
                "<div class=\"activate-page\"><div class=\"activate-card\">"
                "<h1>Reset Password</h1><p class=\"page-subtitle\">Set your new password</p>"
            );
            render_steps(4, 4, steps);
            fprintf(cgiOut,
                "<form method=\"POST\" action=\"/cgi-bin/forgot.cgi\">"
                "<input type=\"hidden\" name=\"step\" value=\"4\">"
                "<input type=\"hidden\" name=\"customer_id\" value=\"%s\">"
                "<input type=\"hidden\" name=\"mobile\" value=\"%s\">"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">New Password</label>"
                "<input class=\"field-input\" type=\"password\" name=\"new_password\""
                "       placeholder=\"Min 8 characters\" minlength=\"8\" required>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Confirm Password</label>"
                "<input class=\"field-input\" type=\"password\" name=\"confirm_pwd\""
                "       placeholder=\"Re-enter password\" required>"
                "</div>"
                "<button class=\"btn btn-primary btn-full\" type=\"submit\">Reset Password</button>"
                "</form></div></div>",
                cid, mobile
            );
            page_end();
            return 0;
        }

        if (cur == 4) {
            /* Save the new password */
            char cid[32], mobile[16], new_pwd[128], confirm[128];
            cgiFormString("customer_id",  cid,     32);
            cgiFormString("mobile",       mobile,  16);
            cgiFormString("new_password", new_pwd, 128);
            cgiFormString("confirm_pwd",  confirm, 128);

            if (strcmp(new_pwd, confirm) != 0) {
                page_start("Forgot Password - Error", "");
                render_public_nav();
                fprintf(cgiOut, "<div class=\"activate-page\"><div class=\"activate-card\"><h1>Reset Password</h1>");
                render_steps(4, 4, steps);
                show_error("Passwords do not match. Please re-enter.");
                fprintf(cgiOut, "<a href=\"/cgi-bin/forgot.cgi\" class=\"btn btn-outline\">Start Over</a></div></div>");
                page_end();
                return 0;
            }

            char jbody[512];
            snprintf(jbody, sizeof(jbody),
                     "{\"customer_id\":\"%s\",\"mobile\":\"%s\",\"new_password\":\"%s\"}",
                     cid, mobile, new_pwd);

            char resp[1024];
            rust_post("/api/auth/reset-password", jbody, resp, sizeof(resp));

            /* Show success */
            page_start("Password Reset Successful", "");
            render_public_nav();
            fprintf(cgiOut,
                "<div class=\"activate-page\"><div class=\"activate-card success-card\">"
                "<div class=\"success-icon\">&#10003;</div>"
                "<h1>Password Reset!</h1>"
                "<p>Your password has been reset successfully. You can now login with your new password.</p>"
                "<a href=\"/cgi-bin/login.cgi\" class=\"btn btn-primary btn-full\">Login Now</a>"
                "</div></div>"
            );
            page_end();
            return 0;
        }
    }

show_step1:
    page_start("Forgot Password", "");
    render_public_nav();
    fprintf(cgiOut,
        "<div class=\"activate-page\"><div class=\"activate-card\">"
        "<h1>Forgot Password?</h1>"
        "<p class=\"page-subtitle\">Don't worry — we'll help you reset it in a few steps.</p>"
    );
    render_steps(4, 1, steps);
    fprintf(cgiOut,
        "<form method=\"POST\" action=\"/cgi-bin/forgot.cgi\">"
        "<input type=\"hidden\" name=\"step\" value=\"1\">"
        "<div class=\"field-group\">"
        "<label class=\"field-label\">Customer ID</label>"
        "<input class=\"field-input\" type=\"text\" name=\"customer_id\""
        "       placeholder=\"e.g. NXB100001\" required>"
        "</div>"
        "<button class=\"btn btn-primary btn-full\" type=\"submit\">Continue</button>"
        "</form>"
        "<div class=\"auth-links\">"
        "<a href=\"/cgi-bin/login.cgi\">&larr; Back to Login</a>"
        "</div>"
        "</div></div>"
    );
    page_end();
    return 0;
}
