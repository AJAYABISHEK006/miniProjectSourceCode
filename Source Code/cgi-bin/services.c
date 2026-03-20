/* =========================================================
   services.c - Service Requests (Address, KYC, Email etc)
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/html_utils.h"
#include "../common/api_client.h"
#include "../common/session.h"

/* All 8 service types we support */
static const char *all_services[] = {
    "Address Change",
    "Update KYC",
    "Update Email ID",
    "Aadhaar Seeding",
    "Change Account Type",
    "Customer Profile Update",
    "Email Statements",
    "Debit Card Services",
    NULL
};

static const char *service_descs[] = {
    "Update your registered address",
    "Update Aadhaar and PAN details",
    "Change your email address",
    "Link your Aadhaar to account",
    "Switch between account types",
    "Update personal details",
    "Register for monthly statements",
    "Block, unblock, or request new card",
    NULL
};

int cgiMain() {
    char *token = read_session_cookie();
    SessionInfo sess = verify_session(token ? token : "");
    if (!sess.is_valid) { cgiHeaderLocation("/cgi-bin/login.cgi"); return 0; }

    const char *cid = sess.customer_id;

    char sr_type[64], req_method[8], search_str[64];
    cgiFormString("sr_type", sr_type,    64);
    cgiFormString("search",  search_str, 64);
    cgiStringValue("REQUEST_METHOD", req_method, 8);

    /* Handle SR form submission */
    if (strcmp(req_method, "POST") == 0 && strlen(sr_type) > 0) {
        char otp_code[16];
        cgiFormString("otp_code", otp_code, 16);

        if (strlen(otp_code) == 0) {
            /* Generate OTP first */
            char otp_body[128];
            snprintf(otp_body, sizeof(otp_body),
                     "{\"customer_id\":\"%s\",\"purpose\":\"sr\"}", cid);
            char otp_resp[512];
            rust_post("/api/auth/generate-otp", otp_body, otp_resp, sizeof(otp_resp));
            char demo_otp[16];
            resp_get_field(otp_resp, "otp_for_demo", demo_otp, sizeof(demo_otp));

            /* Show OTP form - collect all previous values in hidden inputs */
            page_start("Confirm Service Request", "");
            render_sidebar("services", "", cid);
            fprintf(cgiOut,
                "<div class=\"page-header\"><h1>%s</h1></div>"
                "<div class=\"form-card\">",
                sr_type
            );
            if (strlen(demo_otp) > 0) {
                fprintf(cgiOut,
                    "<div class=\"demo-otp-box\">"
                    "<span class=\"demo-label\">OTP:</span>"
                    "<strong class=\"demo-otp-val\">%s</strong>"
                    "</div>", demo_otp
                );
            }
            fprintf(cgiOut,
                "<p>An OTP has been sent to your registered mobile number. Enter it below to confirm.</p>"
                "<form method=\"POST\" action=\"/cgi-bin/services.cgi\">"
                "<input type=\"hidden\" name=\"sr_type\" value=\"%s\">"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Enter OTP *</label>"
                "<input class=\"field-input otp-big\" type=\"text\" name=\"otp_code\""
                "       maxlength=\"6\" required>"
                "</div>"
                "<button class=\"btn btn-primary\" type=\"submit\">Confirm &amp; Submit</button>"
                "</form></div></main></div>",
                sr_type
            );
            page_end();
            return 0;
        }

        /* OTP provided - verify and submit SR */
        char otp_jbody[256];
        snprintf(otp_jbody, sizeof(otp_jbody),
                 "{\"customer_id\":\"%s\",\"otp_entered\":\"%s\",\"purpose\":\"sr\"}",
                 cid, otp_code);
        char otp_resp[1024];
        rust_post("/api/auth/verify-otp", otp_jbody, otp_resp, sizeof(otp_resp));

        if (!resp_is_ok(otp_resp)) {
            page_start("Services", "");
            render_sidebar("services", "", cid);
            show_error("Invalid OTP. Please try again.");
            fprintf(cgiOut,
                "<a href=\"/cgi-bin/services.cgi\" class=\"btn btn-outline\">Back to Services</a>"
                "</main></div>"
            );
            page_end();
            return 0;
        }

        /* Submit the SR to Rust */
        char sr_jbody[1024];
        snprintf(sr_jbody, sizeof(sr_jbody),
                 "{\"session_token\":\"%s\",\"sr_type\":\"%s\",\"sr_data\":\"{}\"}",
                 token, sr_type);

        char resp[2048];
        rust_post("/api/services/submit", sr_jbody, resp, sizeof(resp));

        page_start("Service Request Submitted", "");
        render_sidebar("services", "", cid);

        if (resp_is_ok(resp)) {
            char sr_num[32];
            resp_get_field(resp, "sr_number", sr_num, sizeof(sr_num));
            fprintf(cgiOut,
                "<div class=\"page-header\"><h1>Request Submitted</h1></div>"
                "<div class=\"result-card result-success\">"
                "<div class=\"result-icon\">&#10003;</div>"
                "<h2>%s</h2>"
                "<p>Your service request has been received and is being processed.</p>"
                "<div class=\"result-details\">"
                "<div class=\"review-row\"><span>SR Number</span><strong class=\"mono\">%s</strong></div>"
                "<div class=\"review-row\"><span>Status</span><strong>Pending</strong></div>"
                "</div>"
                "<a href=\"/cgi-bin/services.cgi\" class=\"btn btn-outline\">Back to Services</a>"
                "</div>",
                sr_type, sr_num
            );
        } else {
            show_error("Submission failed. Please try again.");
        }

        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    /* Show a specific service form */
    if (strlen(sr_type) > 0 && strcmp(req_method, "GET") == 0) {
        page_start(sr_type, "");
        render_sidebar("services", "", cid);
        fprintf(cgiOut,
            "<div class=\"page-header\">"
            "<a href=\"/cgi-bin/services.cgi\" class=\"back-link\">&larr; All Services</a>"
            "<h1 class=\"page-title\">%s</h1>"
            "</div>"
            "<div class=\"form-card\">"
            "<form method=\"POST\" action=\"/cgi-bin/services.cgi\">"
            "<input type=\"hidden\" name=\"sr_type\" value=\"%s\">",
            sr_type, sr_type
        );

        /* Render appropriate fields per service type */
        if (strcmp(sr_type, "Address Change") == 0) {
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">New House No / Flat No *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"house_no\" required>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Street / Area *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"street\" required>"
                "</div>"
                "<div class=\"field-row\">"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">City *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"city\" required>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">PIN Code *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"pin_code\" maxlength=\"6\" required>"
                "</div>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">State *</label>"
                "<select class=\"field-input\" name=\"state\" required>"
                "<option value=\"\">Select State</option>"
                "<option>Tamil Nadu</option><option>Maharashtra</option>"
                "<option>Delhi</option><option>Karnataka</option>"
                "<option>Kerala</option><option>Gujarat</option>"
                "<option>Telangana</option><option>Other</option>"
                "</select>"
                "</div>"
            );
        } else if (strcmp(sr_type, "Update Email ID") == 0) {
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">New Email Address *</label>"
                "<input class=\"field-input\" type=\"email\" name=\"new_email\" required>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Confirm New Email *</label>"
                "<input class=\"field-input\" type=\"email\" name=\"confirm_email\" required>"
                "</div>"
            );
        } else if (strcmp(sr_type, "Aadhaar Seeding") == 0) {
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Aadhaar Number (12 digits) *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"aadhaar\" maxlength=\"12\" required>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Confirm Aadhaar *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"confirm_aadhaar\" maxlength=\"12\" required>"
                "</div>"
            );
        } else if (strcmp(sr_type, "Email Statements") == 0) {
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Statement Frequency *</label>"
                "<select class=\"field-input\" name=\"frequency\">"
                "<option value=\"Monthly\">Monthly</option>"
                "<option value=\"Quarterly\">Quarterly</option>"
                "</select>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"checkbox-label\">"
                "<input type=\"checkbox\" name=\"stop_physical\">"
                "Stop physical statements (go paperless)"
                "</label>"
                "</div>"
            );
        } else {
            /* Generic form for other service types */
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Details / Reason *</label>"
                "<textarea class=\"field-input\" name=\"details\" rows=\"4\""
                "          placeholder=\"Please provide relevant details...\" required></textarea>"
                "</div>"
            );
        }

        fprintf(cgiOut,
            "<button class=\"btn btn-primary\" type=\"submit\">Submit Request</button>"
            "</form></div></main></div>"
        );
        page_end();
        return 0;
    }

    /* Show all services landing page */
    page_start("Service Requests", "");
    render_sidebar("services", "", cid);
    fprintf(cgiOut,
        "<div class=\"page-header\"><h1 class=\"page-title\">Service Requests</h1></div>\n"
        "<form class=\"search-bar\" method=\"GET\" action=\"/cgi-bin/services.cgi\">\n"
        "  <input class=\"field-input search-input\" type=\"text\" name=\"search\"\n"
        "         placeholder=\"&#128269; Search services (e.g. address, KYC, email)...\"\n"
        "         value=\"%s\">\n"
        "</form>\n"
        "<div class=\"services-grid\">\n",
        search_str
    );

    for (int i = 0; all_services[i] != NULL; i++) {
        /* If searching, filter results */
        if (strlen(search_str) > 0) {
            if (strstr(all_services[i], search_str) == NULL &&
                strstr(service_descs[i], search_str) == NULL) {
                continue;
            }
        }

        fprintf(cgiOut,
            "  <a href=\"/cgi-bin/services.cgi?sr_type=%s\" class=\"sr-card\">\n"
            "    <h3>%s</h3>\n"
            "    <p>%s</p>\n"
            "    <div class=\"sr-cta\">Request &rarr;</div>\n"
            "  </a>\n",
            all_services[i],
            all_services[i],
            service_descs[i]
        );
    }

    fprintf(cgiOut, "</div>\n</main></div>");
    page_end();
    return 0;
}
