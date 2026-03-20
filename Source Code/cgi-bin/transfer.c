/* =========================================================
   transfer.c - Fund Transfer (NEFT / IMPS / UPI)
   5-step flow: Select type > Details > Review > OTP > Result
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
    char req_method[8];
    cgiStringValue("REQUEST_METHOD", req_method, 8);

    char stage_str[4];
    cgiFormString("stage", stage_str, 4);
    int stage = atoi(stage_str);

    /* ---- STAGE 0 / GET: Select transfer type ---- */
    if (strcmp(req_method, "GET") != 0 && stage == 0) stage = 0;

    if (stage == 0 || strcmp(req_method, "GET") == 0) {
        page_start("Send Money", "");
        render_sidebar("transfer", "", cid);
        fprintf(cgiOut,
            "<div class=\"page-header\"><h1 class=\"page-title\">Send Money</h1></div>\n"
            "<p class=\"page-sub\">Choose how you want to transfer funds.</p>\n"
            "<form method=\"POST\" action=\"/cgi-bin/transfer.cgi\">\n"
            "<input type=\"hidden\" name=\"stage\" value=\"1\">\n"
            "<div class=\"transfer-type-grid\">\n"
            "  <label class=\"transfer-type-card\">\n"
            "    <input type=\"radio\" name=\"transfer_type\" value=\"NEFT\" required>\n"
            "    <div class=\"type-inner\">\n"
            "      <div class=\"type-name\">NEFT</div>\n"
            "      <div class=\"type-time\">2-4 hours</div>\n"
            "      <div class=\"type-desc\">Bank to bank transfer. Works all days.</div>\n"
            "    </div>\n"
            "  </label>\n"
            "  <label class=\"transfer-type-card\">\n"
            "    <input type=\"radio\" name=\"transfer_type\" value=\"IMPS\">\n"
            "    <div class=\"type-inner\">\n"
            "      <div class=\"type-name\">IMPS</div>\n"
            "      <div class=\"type-time\">Instant 24x7</div>\n"
            "      <div class=\"type-desc\">Immediate transfer at any time of day.</div>\n"
            "    </div>\n"
            "  </label>\n"
            "  <label class=\"transfer-type-card\">\n"
            "    <input type=\"radio\" name=\"transfer_type\" value=\"UPI\">\n"
            "    <div class=\"type-inner\">\n"
            "      <div class=\"type-name\">UPI</div>\n"
            "      <div class=\"type-time\">Instant</div>\n"
            "      <div class=\"type-desc\">Transfer using a UPI ID (e.g. name@upi).</div>\n"
            "    </div>\n"
            "  </label>\n"
            "</div>\n"
            "<button class=\"btn btn-primary\" type=\"submit\">Continue &rarr;</button>\n"
            "</form>\n"
        );
        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    if (stage == 1) {
        /* Show beneficiary details form */
        char xfer_type[16];
        cgiFormString("transfer_type", xfer_type, 16);

        page_start("Send Money - Details", "");
        render_sidebar("transfer", "", cid);
        fprintf(cgiOut,
            "<div class=\"page-header\"><h1 class=\"page-title\">%s Transfer</h1></div>\n"
            "<div class=\"form-card\">\n"
            "<form method=\"POST\" action=\"/cgi-bin/transfer.cgi\">\n"
            "<input type=\"hidden\" name=\"stage\" value=\"2\">\n"
            "<input type=\"hidden\" name=\"transfer_type\" value=\"%s\">\n",
            xfer_type, xfer_type
        );

        if (strcmp(xfer_type, "UPI") == 0) {
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">UPI ID *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"to_upi\""
                "       placeholder=\"e.g. name@upi\" required>"
                "</div>"
            );
        } else {
            fprintf(cgiOut,
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Beneficiary Name *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"ben_name\""
                "       placeholder=\"Full name\" required>"
                "</div>"
                "<div class=\"field-row\">"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Account Number *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"to_account\""
                "       placeholder=\"Account number\" required>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">Confirm Account *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"confirm_acc\""
                "       placeholder=\"Re-enter account number\" required>"
                "</div>"
                "</div>"
                "<div class=\"field-group\">"
                "<label class=\"field-label\">IFSC Code *</label>"
                "<input class=\"field-input\" type=\"text\" name=\"to_ifsc\""
                "       placeholder=\"e.g. NXB0001001\" required>"
                "</div>"
            );
        }

        fprintf(cgiOut,
            "<div class=\"field-row\">"
            "<div class=\"field-group\">"
            "<label class=\"field-label\">Amount (&#8377;) *</label>"
            "<input class=\"field-input\" type=\"number\" name=\"the_amount\""
            "       placeholder=\"Enter amount\" min=\"1\" required>"
            "</div>"
            "<div class=\"field-group\">"
            "<label class=\"field-label\">Remarks</label>"
            "<input class=\"field-input\" type=\"text\" name=\"remarks\""
            "       placeholder=\"Optional note\">"
            "</div>"
            "</div>"
            "<button class=\"btn btn-primary\" type=\"submit\">Review Transfer</button>"
            "</form></div>"
        );
        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    if (stage == 2) {
        /* Review stage - show summary and OTP */
        char xfer_type[16], ben_name[128], to_acc[32], to_upi[64];
        char to_ifsc[16], amount_str[32], remarks[128];

        cgiFormString("transfer_type", xfer_type,   16);
        cgiFormString("ben_name",      ben_name,    128);
        cgiFormString("to_account",    to_acc,      32);
        cgiFormString("to_upi",        to_upi,      64);
        cgiFormString("to_ifsc",       to_ifsc,     16);
        cgiFormString("the_amount",    amount_str,  32);
        cgiFormString("remarks",       remarks,     128);

        /* Generate OTP for this transfer */
        char otp_body[128];
        snprintf(otp_body, sizeof(otp_body),
                 "{\"customer_id\":\"%s\",\"purpose\":\"transfer\"}", cid);
        char otp_resp[512];
        rust_post("/api/auth/generate-otp", otp_body, otp_resp, sizeof(otp_resp));
        char demo_otp[16];
        resp_get_field(otp_resp, "otp_for_demo", demo_otp, sizeof(demo_otp));

        page_start("Send Money - Review", "");
        render_sidebar("transfer", "", cid);
        fprintf(cgiOut,
            "<div class=\"page-header\"><h1 class=\"page-title\">Review Transfer</h1></div>\n"
            "<div class=\"review-card\">\n"
            "  <div class=\"review-row\"><span>Transfer Type</span><strong>%s</strong></div>\n"
            "  <div class=\"review-row\"><span>To</span><strong>%s</strong></div>\n"
            "  <div class=\"review-row\"><span>Amount</span><strong class=\"amount-big\">&#8377; %s</strong></div>\n"
            "  <div class=\"review-row\"><span>Remarks</span><strong>%s</strong></div>\n"
            "</div>\n",
            xfer_type,
            strcmp(xfer_type, "UPI") == 0 ? to_upi : to_acc,
            amount_str,
            strlen(remarks) > 0 ? remarks : "—"
        );

        if (strlen(demo_otp) > 0) {
            fprintf(cgiOut,
                "<div class=\"demo-otp-box\">"
                "<span class=\"demo-label\">Transfer OTP:</span>"
                "<strong class=\"demo-otp-val\">%s</strong>"
                "</div>", demo_otp
            );
        }

        fprintf(cgiOut,
            "<form method=\"POST\" action=\"/cgi-bin/transfer.cgi\">"
            "<input type=\"hidden\" name=\"stage\" value=\"3\">"
            "<input type=\"hidden\" name=\"transfer_type\" value=\"%s\">"
            "<input type=\"hidden\" name=\"ben_name\" value=\"%s\">"
            "<input type=\"hidden\" name=\"to_account\" value=\"%s\">"
            "<input type=\"hidden\" name=\"to_upi\" value=\"%s\">"
            "<input type=\"hidden\" name=\"to_ifsc\" value=\"%s\">"
            "<input type=\"hidden\" name=\"the_amount\" value=\"%s\">"
            "<input type=\"hidden\" name=\"remarks\" value=\"%s\">"
            "<div class=\"field-group\">"
            "<label class=\"field-label\">Enter OTP to Confirm *</label>"
            "<input class=\"field-input otp-big\" type=\"text\" name=\"otp_code\""
            "       placeholder=\"6-digit OTP\" maxlength=\"6\" required>"
            "</div>"
            "<div class=\"btn-row\">"
            "<a href=\"/cgi-bin/transfer.cgi\" class=\"btn btn-outline\">Cancel</a>"
            "<button class=\"btn btn-primary\" type=\"submit\">Confirm Transfer</button>"
            "</div>"
            "</form>",
            xfer_type, ben_name, to_acc, to_upi, to_ifsc, amount_str, remarks
        );
        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    if (stage == 3) {
        /* Do the actual transfer */
        char xfer_type[16], ben_name[128], to_acc[32], to_upi[64];
        char to_ifsc[16], amount_str[32], remarks[128], otp_code[16];

        cgiFormString("transfer_type", xfer_type,  16);
        cgiFormString("ben_name",      ben_name,   128);
        cgiFormString("to_account",    to_acc,     32);
        cgiFormString("to_upi",        to_upi,     64);
        cgiFormString("to_ifsc",       to_ifsc,    16);
        cgiFormString("the_amount",    amount_str, 32);
        cgiFormString("remarks",       remarks,    128);
        cgiFormString("otp_code",      otp_code,   16);

        /* Verify OTP first */
        char otp_body[256];
        snprintf(otp_body, sizeof(otp_body),
                 "{\"customer_id\":\"%s\",\"otp_entered\":\"%s\",\"purpose\":\"transfer\"}",
                 cid, otp_code);
        char otp_resp[1024];
        int otp_ok = rust_post("/api/auth/verify-otp", otp_body, otp_resp, sizeof(otp_resp));

        if (!otp_ok || !resp_is_ok(otp_resp)) {
            page_start("Transfer Failed", "");
            render_sidebar("transfer", "", cid);
            fprintf(cgiOut, "<div class=\"page-header\"><h1>Transfer Failed</h1></div>");
            show_error("Invalid OTP. Transfer cancelled for security.");
            fprintf(cgiOut,
                "<a href=\"/cgi-bin/transfer.cgi\" class=\"btn btn-primary\">Try Again</a>"
            );
            fprintf(cgiOut, "</main></div>");
            page_end();
            return 0;
        }

        /* Build transfer request */
        char json_body[1024];
        snprintf(json_body, sizeof(json_body),
                 "{\"session_token\":\"%s\",\"transfer_type\":\"%s\","
                 "\"to_account\":\"%s\",\"to_ifsc\":\"%s\","
                 "\"to_upi\":\"%s\",\"ben_name\":\"%s\","
                 "\"the_amount\":%s,\"remarks\":\"%s\"}",
                 token, xfer_type, to_acc, to_ifsc, to_upi, ben_name, amount_str, remarks);

        char resp[2048];
        int ok = rust_post("/api/transfer", json_body, resp, sizeof(resp));

        page_start("Transfer Result", "");
        render_sidebar("transfer", "", cid);

        if (ok && resp_is_ok(resp)) {
            char txn_id[64], new_bal[32];
            resp_get_field(resp, "txn_id",      txn_id,  sizeof(txn_id));
            resp_get_field(resp, "new_balance",  new_bal, sizeof(new_bal));

            fprintf(cgiOut,
                "<div class=\"result-card result-success\">"
                "<div class=\"result-icon\">&#10003;</div>"
                "<h2>Transfer Successful!</h2>"
                "<div class=\"result-details\">"
                "<div class=\"review-row\"><span>Amount Sent</span><strong>&#8377; %s</strong></div>"
                "<div class=\"review-row\"><span>Transaction ID</span><strong class=\"mono\">%s</strong></div>"
                "<div class=\"review-row\"><span>New Balance</span><strong>&#8377; %s</strong></div>"
                "</div>"
                "<div class=\"btn-row\">"
                "<a href=\"/cgi-bin/dashboard.cgi\" class=\"btn btn-outline\">Dashboard</a>"
                "<a href=\"/cgi-bin/transfer.cgi\" class=\"btn btn-primary\">Transfer Again</a>"
                "</div>"
                "</div>",
                amount_str, txn_id, new_bal
            );
        } else {
            char err_msg[256];
            resp_get_field(resp, "msg", err_msg, sizeof(err_msg));
            fprintf(cgiOut,
                "<div class=\"result-card result-fail\">"
                "<div class=\"result-icon\">&#10006;</div>"
                "<h2>Transfer Failed</h2>"
                "<p>%s</p>"
                "<a href=\"/cgi-bin/transfer.cgi\" class=\"btn btn-primary\">Try Again</a>"
                "</div>",
                strlen(err_msg) > 0 ? err_msg : "Something went wrong. Please try again."
            );
        }

        fprintf(cgiOut, "</main></div>");
        page_end();
        return 0;
    }

    /* Fallback */
    cgiHeaderLocation("/cgi-bin/transfer.cgi");
    return 0;
}
