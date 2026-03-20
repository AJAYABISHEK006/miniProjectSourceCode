/* =========================================================
   loans.c - Loan applications (Personal, Home, Education)
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/html_utils.h"
#include "../common/api_client.h"
#include "../common/session.h"

int cgiMain() {
    char *token = read_session_cookie();
    SessionInfo sess = verify_session(token ? token : "");
    if (!sess.is_valid) { cgiHeaderLocation("/cgi-bin/login.cgi"); return 0; }

    const char *cid = sess.customer_id;

    char loan_type[32], stage_str[4];
    cgiFormString("type",  loan_type, 32);
    cgiFormString("stage", stage_str, 4);
    int stage = atoi(stage_str);

    /* If no type selected, show the loan landing page */
    if (strlen(loan_type) == 0) {
        page_start("Loans", "");
        render_sidebar("loans", "", cid);
        fprintf(cgiOut,
            "<div class=\"page-header\"><h1 class=\"page-title\">Loan Products</h1></div>\n"
            "<p class=\"page-sub\">Choose the loan that's right for you.</p>\n"
            "<div class=\"loan-cards-grid\">\n"

            "  <a href=\"/cgi-bin/loans.cgi?type=Personal+Loan\" class=\"loan-card\">\n"
            "    <div class=\"loan-icon\">&#9679;</div>\n"
            "    <h3>Personal Loan</h3>\n"
            "    <p>Up to &#8377;40 Lakhs. No collateral needed.</p>\n"
            "    <div class=\"loan-rate\">From 10.75%% p.a.</div>\n"
            "    <div class=\"loan-cta\">Apply Now &rarr;</div>\n"
            "  </a>\n"

            "  <a href=\"/cgi-bin/loans.cgi?type=Home+Loan\" class=\"loan-card\">\n"
            "    <div class=\"loan-icon\">&#127968;</div>\n"
            "    <h3>Home Loan</h3>\n"
            "    <p>Finance your dream home. Tenure up to 30 years.</p>\n"
            "    <div class=\"loan-rate\">From 8.5%% p.a.</div>\n"
            "    <div class=\"loan-cta\">Apply Now &rarr;</div>\n"
            "  </a>\n"

            "  <a href=\"/cgi-bin/loans.cgi?type=Education+Loan\" class=\"loan-card\">\n"
            "    <div class=\"loan-icon\">&#127891;</div>\n"
            "    <h3>Education Loan</h3>\n"
            "    <p>Invest in your future. Covers tuition and living expenses.</p>\n"
            "    <div class=\"loan-rate\">From 9%% p.a.</div>\n"
            "    <div class=\"loan-cta\">Apply Now &rarr;</div>\n"
            "  </a>\n"

            "</div>\n"
        );
        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    /* Handle form submission */
    char req_method[8];
    cgiStringValue("REQUEST_METHOD", req_method, 8);

    if (strcmp(req_method, "POST") == 0 && stage == 1) {
        /* Collect all form fields and submit to Rust */
        char amount_str[32], tenure_str[8], purpose[64];
        char emp_type[32], company[128], designation[64], income[32];
        char course_name[128], institution[128];

        cgiFormString("loan_amount",    amount_str,  32);
        cgiFormString("tenure",         tenure_str,  8);
        cgiFormString("loan_purpose",   purpose,     64);
        cgiFormString("employment_type",emp_type,    32);
        cgiFormString("company_name",   company,     128);
        cgiFormString("designation",    designation, 64);
        cgiFormString("monthly_income", income,      32);
        cgiFormString("course_name",    course_name, 128);
        cgiFormString("institution",    institution, 128);

        /* Build form data JSON */
        char form_json[1024];
        snprintf(form_json, sizeof(form_json),
                 "{\"purpose\":\"%s\",\"employment_type\":\"%s\","
                 "\"company\":\"%s\",\"income\":\"%s\","
                 "\"course\":\"%s\",\"institution\":\"%s\"}",
                 purpose, emp_type, company, income, course_name, institution);

        char json_body[2048];
        snprintf(json_body, sizeof(json_body),
                 "{\"session_token\":\"%s\",\"loan_type\":\"%s\","
                 "\"loan_amount\":%s,\"tenure\":%s,\"form_data\":\"%s\"}",
                 token, loan_type, amount_str, tenure_str, form_json);

        char resp[2048];
        rust_post("/api/loans/apply", json_body, resp, sizeof(resp));

        page_start("Loan Application Submitted", "");
        render_sidebar("loans", "", cid);

        if (resp_is_ok(resp)) {
            char app_id[64];
            resp_get_field(resp, "app_id", app_id, sizeof(app_id));

            fprintf(cgiOut,
                "<div class=\"page-header\"><h1>Application Submitted!</h1></div>\n"
                "<div class=\"result-card result-success\">\n"
                "  <div class=\"result-icon\">&#10003;</div>\n"
                "  <h2>Application Received</h2>\n"
                "  <p>Your %s application has been submitted successfully.</p>\n"
                "  <div class=\"result-details\">\n"
                "    <div class=\"review-row\">\n"
                "      <span>Application ID</span>\n"
                "      <strong class=\"mono\">%s</strong>\n"
                "    </div>\n"
                "    <div class=\"review-row\">\n"
                "      <span>Amount Requested</span>\n"
                "      <strong>&#8377; %s</strong>\n"
                "    </div>\n"
                "    <div class=\"review-row\"><span>Status</span><strong>Under Review</strong></div>\n"
                "  </div>\n"
                "  <div class=\"status-tracker\">\n"
                "    <div class=\"track-step track-done\">Applied &#10003;</div>\n"
                "    <div class=\"track-line\"></div>\n"
                "    <div class=\"track-step track-active\">Under Review</div>\n"
                "    <div class=\"track-line\"></div>\n"
                "    <div class=\"track-step\">Approved</div>\n"
                "    <div class=\"track-line\"></div>\n"
                "    <div class=\"track-step\">Disbursed</div>\n"
                "  </div>\n"
                "  <a href=\"/cgi-bin/loans.cgi\" class=\"btn btn-outline\">Apply Another Loan</a>\n"
                "</div>\n",
                loan_type, app_id, amount_str
            );
        } else {
            show_error("Application failed. Please try again.");
        }

        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    /* Show the loan application form */
    page_start(loan_type, "");
    render_sidebar("loans", "", cid);

    fprintf(cgiOut,
        "<div class=\"page-header\">\n"
        "  <a href=\"/cgi-bin/loans.cgi\" class=\"back-link\">&larr; All Loans</a>\n"
        "  <h1 class=\"page-title\">%s Application</h1>\n"
        "</div>\n"
        "<div class=\"form-card\">\n"
        "<form method=\"POST\" action=\"/cgi-bin/loans.cgi\">\n"
        "<input type=\"hidden\" name=\"type\" value=\"%s\">\n"
        "<input type=\"hidden\" name=\"stage\" value=\"1\">\n",
        loan_type, loan_type
    );

    /* Common fields for all loans */
    fprintf(cgiOut,
        "<h3 class=\"form-section-title\">Loan Details</h3>\n"
        "<div class=\"field-row\">\n"
        "  <div class=\"field-group\">\n"
        "    <label class=\"field-label\">Loan Amount (&#8377;) *</label>\n"
        "    <input class=\"field-input\" type=\"number\" name=\"loan_amount\"\n"
        "           placeholder=\"e.g. 500000\" min=\"10000\" required>\n"
        "  </div>\n"
        "  <div class=\"field-group\">\n"
        "    <label class=\"field-label\">Tenure (months) *</label>\n"
        "    <select class=\"field-input\" name=\"tenure\" required>\n"
        "      <option value=\"12\">12 months</option>\n"
        "      <option value=\"24\">24 months</option>\n"
        "      <option value=\"36\">36 months</option>\n"
        "      <option value=\"48\">48 months</option>\n"
        "      <option value=\"60\">60 months</option>\n"
        "    </select>\n"
        "  </div>\n"
        "</div>\n"
    );

    /* Loan-type specific fields */
    if (strcmp(loan_type, "Personal Loan") == 0) {
        fprintf(cgiOut,
            "<div class=\"field-group\">\n"
            "  <label class=\"field-label\">Loan Purpose *</label>\n"
            "  <select class=\"field-input\" name=\"loan_purpose\" required>\n"
            "    <option value=\"\">Select purpose</option>\n"
            "    <option value=\"Medical Emergency\">Medical Emergency</option>\n"
            "    <option value=\"Wedding\">Wedding</option>\n"
            "    <option value=\"Travel\">Travel</option>\n"
            "    <option value=\"Home Renovation\">Home Renovation</option>\n"
            "    <option value=\"Other\">Other</option>\n"
            "  </select>\n"
            "</div>\n"
        );
    } else if (strcmp(loan_type, "Home Loan") == 0) {
        fprintf(cgiOut,
            "<div class=\"field-row\">\n"
            "  <div class=\"field-group\">\n"
            "    <label class=\"field-label\">Property Type *</label>\n"
            "    <select class=\"field-input\" name=\"loan_purpose\" required>\n"
            "      <option value=\"Apartment\">Apartment</option>\n"
            "      <option value=\"House\">Independent House</option>\n"
            "      <option value=\"Plot\">Plot</option>\n"
            "    </select>\n"
            "  </div>\n"
            "  <div class=\"field-group\">\n"
            "    <label class=\"field-label\">Property Value (&#8377;) *</label>\n"
            "    <input class=\"field-input\" type=\"number\" name=\"property_value\"\n"
            "           placeholder=\"Approximate value\" required>\n"
            "  </div>\n"
            "</div>\n"
        );
    } else if (strcmp(loan_type, "Education Loan") == 0) {
        fprintf(cgiOut,
            "<h3 class=\"form-section-title\">Course Details</h3>\n"
            "<div class=\"field-group\">\n"
            "  <label class=\"field-label\">Course Name *</label>\n"
            "  <input class=\"field-input\" type=\"text\" name=\"course_name\"\n"
            "         placeholder=\"e.g. B.Tech Computer Science\" required>\n"
            "</div>\n"
            "<div class=\"field-group\">\n"
            "  <label class=\"field-label\">Institution Name *</label>\n"
            "  <input class=\"field-input\" type=\"text\" name=\"institution\"\n"
            "         placeholder=\"College or university name\" required>\n"
            "</div>\n"
            "<div class=\"field-row\">\n"
            "  <div class=\"field-group\">\n"
            "    <label class=\"field-label\">Course Type *</label>\n"
            "    <select class=\"field-input\" name=\"course_type\">\n"
            "      <option value=\"UG\">Undergraduate (UG)</option>\n"
            "      <option value=\"PG\">Postgraduate (PG)</option>\n"
            "      <option value=\"Diploma\">Diploma</option>\n"
            "      <option value=\"PhD\">PhD</option>\n"
            "    </select>\n"
            "  </div>\n"
            "  <div class=\"field-group\">\n"
            "    <label class=\"field-label\">Course Duration (years)</label>\n"
            "    <input class=\"field-input\" type=\"number\" name=\"duration\" min=\"1\" max=\"6\">\n"
            "  </div>\n"
            "</div>\n"
        );
    }

    /* Employment fields common to most loans */
    fprintf(cgiOut,
        "<h3 class=\"form-section-title\">Employment Details</h3>\n"
        "<div class=\"field-row\">\n"
        "  <div class=\"field-group\">\n"
        "    <label class=\"field-label\">Employment Type *</label>\n"
        "    <select class=\"field-input\" name=\"employment_type\" required>\n"
        "      <option value=\"Salaried\">Salaried</option>\n"
        "      <option value=\"Self-Employed\">Self-Employed</option>\n"
        "      <option value=\"Business\">Business Owner</option>\n"
        "    </select>\n"
        "  </div>\n"
        "  <div class=\"field-group\">\n"
        "    <label class=\"field-label\">Monthly Income (&#8377;) *</label>\n"
        "    <input class=\"field-input\" type=\"number\" name=\"monthly_income\"\n"
        "           placeholder=\"Net monthly income\" required>\n"
        "  </div>\n"
        "</div>\n"
        "<div class=\"field-group\">\n"
        "  <label class=\"field-label\">Company/Business Name *</label>\n"
        "  <input class=\"field-input\" type=\"text\" name=\"company_name\"\n"
        "         placeholder=\"Employer or business name\" required>\n"
        "</div>\n"
        "<div class=\"declaration-box\">\n"
        "  <label class=\"checkbox-label\">\n"
        "    <input type=\"checkbox\" name=\"declaration1\" required>\n"
        "    I confirm that all the information provided is accurate and correct.\n"
        "  </label>\n"
        "  <label class=\"checkbox-label\">\n"
        "    <input type=\"checkbox\" name=\"declaration2\" required>\n"
        "    I agree to the NexusBank Terms &amp; Conditions for loan products.\n"
        "  </label>\n"
        "</div>\n"
        "<button class=\"btn btn-primary\" type=\"submit\">Submit Application</button>\n"
        "</form>\n"
        "</div>\n"
    );

    fprintf(cgiOut, "</main></div>");
    page_end();
    return 0;
}
