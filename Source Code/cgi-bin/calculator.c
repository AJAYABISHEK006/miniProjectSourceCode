/* =========================================================
   calculator.c - EMI Calculator
   Sliders + inputs, calculates on every form submit
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

    /* Read input values */
    char principal_str[32], rate_str[16], tenure_str[8], preset[32];
    cgiFormString("principal",     principal_str, 32);
    cgiFormString("annual_rate",   rate_str,      16);
    cgiFormString("tenure_months", tenure_str,    8);
    cgiFormString("preset",        preset,        32);

    /* Apply presets */
    if (strlen(preset) > 0 && strlen(rate_str) == 0) {
        if (strcmp(preset, "personal")  == 0) strcpy(rate_str, "12.0");
        else if (strcmp(preset, "home") == 0) strcpy(rate_str, "8.5");
        else if (strcmp(preset, "edu")  == 0) strcpy(rate_str, "9.0");
    }

    /* Default values */
    if (strlen(principal_str) == 0) strcpy(principal_str, "500000");
    if (strlen(rate_str) == 0)      strcpy(rate_str, "12.0");
    if (strlen(tenure_str) == 0)    strcpy(tenure_str, "36");

    /* Calculate EMI if we have values */
    char emi_result[2048];
    memset(emi_result, 0, sizeof(emi_result));
    char monthly_emi[32] = "", total_interest[32] = "", total_payable[32] = "";

    if (strlen(principal_str) > 0) {
        char jbody[256];
        snprintf(jbody, sizeof(jbody),
                 "{\"principal\":%s,\"annual_rate\":%s,\"tenure_months\":%s}",
                 principal_str, rate_str, tenure_str);

        rust_post("/api/calculator/emi", jbody, emi_result, sizeof(emi_result));

        resp_get_field(emi_result, "monthly_emi",    monthly_emi,    sizeof(monthly_emi));
        resp_get_field(emi_result, "total_interest",  total_interest, sizeof(total_interest));
        resp_get_field(emi_result, "total_payable",   total_payable,  sizeof(total_payable));
    }

    page_start("EMI Calculator", "");
    render_sidebar("calculator", "", cid);

    fprintf(cgiOut,
        "<div class=\"page-header\"><h1 class=\"page-title\">EMI Calculator</h1></div>\n"
        "<p class=\"page-sub\">Plan your loan repayments before you apply.</p>\n"

        /* Preset buttons */
        "<div class=\"preset-bar\">\n"
        "  <form method=\"GET\" action=\"/cgi-bin/calculator.cgi\">\n"
        "    <input type=\"hidden\" name=\"preset\" value=\"personal\">\n"
        "    <button class=\"preset-btn%s\" type=\"submit\">Personal Loan (12%%)</button>\n"
        "  </form>\n"
        "  <form method=\"GET\" action=\"/cgi-bin/calculator.cgi\">\n"
        "    <input type=\"hidden\" name=\"preset\" value=\"home\">\n"
        "    <button class=\"preset-btn%s\" type=\"submit\">Home Loan (8.5%%)</button>\n"
        "  </form>\n"
        "  <form method=\"GET\" action=\"/cgi-bin/calculator.cgi\">\n"
        "    <input type=\"hidden\" name=\"preset\" value=\"edu\">\n"
        "    <button class=\"preset-btn%s\" type=\"submit\">Education Loan (9%%)</button>\n"
        "  </form>\n"
        "</div>\n"

        "<div class=\"calc-layout\">\n"
        "  <div class=\"calc-inputs\">\n"
        "    <form method=\"GET\" action=\"/cgi-bin/calculator.cgi\">\n"

        "    <div class=\"field-group\">\n"
        "      <label class=\"field-label\">Loan Amount (&#8377;)</label>\n"
        "      <input class=\"field-input\" type=\"number\" name=\"principal\"\n"
        "             value=\"%s\" min=\"10000\" max=\"5000000\" step=\"10000\">\n"
        "      <input class=\"calc-slider\" type=\"range\" name=\"p_slider\"\n"
        "             value=\"%s\" min=\"10000\" max=\"5000000\" step=\"10000\">\n"
        "    </div>\n"

        "    <div class=\"field-group\">\n"
        "      <label class=\"field-label\">Annual Interest Rate (%%)</label>\n"
        "      <input class=\"field-input\" type=\"number\" name=\"annual_rate\"\n"
        "             value=\"%s\" min=\"5\" max=\"30\" step=\"0.1\">\n"
        "      <input class=\"calc-slider\" type=\"range\" name=\"r_slider\"\n"
        "             value=\"%s\" min=\"5\" max=\"30\" step=\"0.1\">\n"
        "    </div>\n"

        "    <div class=\"field-group\">\n"
        "      <label class=\"field-label\">Tenure (months)</label>\n"
        "      <input class=\"field-input\" type=\"number\" name=\"tenure_months\"\n"
        "             value=\"%s\" min=\"6\" max=\"360\" step=\"6\">\n"
        "      <input class=\"calc-slider\" type=\"range\" name=\"t_slider\"\n"
        "             value=\"%s\" min=\"6\" max=\"360\" step=\"6\">\n"
        "    </div>\n"

        "    <button class=\"btn btn-primary\" type=\"submit\">Calculate EMI</button>\n"
        "    </form>\n"
        "  </div>\n",

        strcmp(preset, "personal") == 0 ? " active" : "",
        strcmp(preset, "home")     == 0 ? " active" : "",
        strcmp(preset, "edu")      == 0 ? " active" : "",
        principal_str, principal_str,
        rate_str, rate_str,
        tenure_str, tenure_str
    );

    /* Results panel */
    if (strlen(monthly_emi) > 0) {
        fprintf(cgiOut,
            "  <div class=\"calc-results\">\n"
            "    <div class=\"result-highlight\">\n"
            "      <div class=\"result-lbl\">Monthly EMI</div>\n"
            "      <div class=\"result-emi\">&#8377; %s</div>\n"
            "    </div>\n"
            "    <div class=\"result-breakdown\">\n"
            "      <div class=\"breakdown-row\">\n"
            "        <span>Principal Amount</span>\n"
            "        <strong>&#8377; %s</strong>\n"
            "      </div>\n"
            "      <div class=\"breakdown-row\">\n"
            "        <span>Total Interest</span>\n"
            "        <strong class=\"interest-val\">&#8377; %s</strong>\n"
            "      </div>\n"
            "      <div class=\"breakdown-row total-row\">\n"
            "        <span>Total Payable</span>\n"
            "        <strong>&#8377; %s</strong>\n"
            "      </div>\n"
            "    </div>\n"
            /* CSS conic-gradient donut chart */
            "    <div class=\"donut-chart\">\n"
            "      <div class=\"donut-ring\"></div>\n"
            "      <div class=\"donut-labels\">\n"
            "        <div class=\"donut-lbl principal-lbl\">Principal</div>\n"
            "        <div class=\"donut-lbl interest-lbl\">Interest</div>\n"
            "      </div>\n"
            "    </div>\n"
            "  </div>\n",
            monthly_emi, principal_str, total_interest, total_payable
        );
    } else {
        fprintf(cgiOut,
            "  <div class=\"calc-results calc-placeholder\">\n"
            "    <div class=\"placeholder-text\">Enter values and click Calculate</div>\n"
            "  </div>\n"
        );
    }

    fprintf(cgiOut, "</div>\n</main></div>");
    page_end();
    return 0;
}
