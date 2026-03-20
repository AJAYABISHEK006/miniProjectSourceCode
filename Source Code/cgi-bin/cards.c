/* =========================================================
   cards.c - Cards Overview and Management
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

    char action[32];
    cgiFormString("action", action, 32);

    /* Handle card actions (block, apply etc) */
    if (strlen(action) > 0) {
        char req_method[8];
        cgiStringValue("REQUEST_METHOD", req_method, 8);

        if (strcmp(req_method, "POST") == 0) {
            /* Get OTP first if needed */
            char otp_body[128];
            snprintf(otp_body, sizeof(otp_body),
                     "{\"customer_id\":\"%s\",\"purpose\":\"card\"}", cid);
            char otp_resp[512];
            rust_post("/api/auth/generate-otp", otp_body, otp_resp, sizeof(otp_resp));
            char demo_otp[16];
            resp_get_field(otp_resp, "otp_for_demo", demo_otp, sizeof(demo_otp));

            char otp_code[16];
            cgiFormString("otp_code", otp_code, 16);

            if (strlen(otp_code) > 0) {
                /* OTP submitted - verify and do the action */
                char jbody[512];
                snprintf(jbody, sizeof(jbody),
                         "{\"session_token\":\"%s\",\"action\":\"%s\"}", token, action);

                char resp[1024];
                rust_post("/api/cards/action", jbody, resp, sizeof(resp));

                page_start("Cards", "");
                render_sidebar("cards", "", cid);
                fprintf(cgiOut, "<div class=\"page-header\"><h1>Card Action</h1></div>");
                if (resp_is_ok(resp)) {
                    show_success("Card action completed successfully.");
                } else {
                    show_error("Action failed. Please try again.");
                }
                fprintf(cgiOut,
                    "<a href=\"/cgi-bin/cards.cgi\" class=\"btn btn-outline\">Back to Cards</a>"
                    "</main></div>"
                );
                page_end();
                return 0;
            } else {
                /* Show OTP form */
                page_start("Confirm Card Action", "");
                render_sidebar("cards", "", cid);
                fprintf(cgiOut,
                    "<div class=\"page-header\"><h1>Confirm Action</h1></div>"
                    "<div class=\"form-card\">"
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
                    "<form method=\"POST\" action=\"/cgi-bin/cards.cgi\">"
                    "<input type=\"hidden\" name=\"action\" value=\"%s\">"
                    "<div class=\"field-group\">"
                    "<label class=\"field-label\">Enter OTP to confirm</label>"
                    "<input class=\"field-input otp-big\" type=\"text\" name=\"otp_code\""
                    "       maxlength=\"6\" required>"
                    "</div>"
                    "<button class=\"btn btn-primary\" type=\"submit\">Confirm</button>"
                    "</form></div></main></div>",
                    action
                );
                page_end();
                return 0;
            }
        }
    }

    page_start("My Cards", "");
    render_sidebar("cards", "", cid);

    fprintf(cgiOut,
        "<div class=\"page-header\"><h1 class=\"page-title\">My Cards</h1></div>\n"

        /* CSS tab system using radio buttons */
        "<div class=\"card-tabs\">\n"
        "  <input type=\"radio\" name=\"ctab\" id=\"ctab1\" class=\"ctab-radio\" checked>\n"
        "  <input type=\"radio\" name=\"ctab\" id=\"ctab2\" class=\"ctab-radio\">\n"
        "  <input type=\"radio\" name=\"ctab\" id=\"ctab3\" class=\"ctab-radio\">\n"
        "  <div class=\"ctab-labels\">\n"
        "    <label for=\"ctab1\" class=\"ctab-label\">My Debit Cards</label>\n"
        "    <label for=\"ctab2\" class=\"ctab-label\">Credit Cards</label>\n"
        "    <label for=\"ctab3\" class=\"ctab-label\">Card Management</label>\n"
        "  </div>\n"

        /* Tab 1: Debit Cards */
        "  <div class=\"ctab-panel\" id=\"cpanel1\">\n"
        "    <div class=\"cards-display\">\n"
        "      <div class=\"bank-card bank-card-blue\">\n"
        "        <div class=\"bcard-top\">\n"
        "          <span class=\"bcard-bank\">NexusBank</span>\n"
        "          <span class=\"bcard-chip\">&#9632;&#9632;</span>\n"
        "        </div>\n"
        "        <div class=\"bcard-number\">XXXX XXXX XXXX 6601</div>\n"
        "        <div class=\"bcard-bottom\">\n"
        "          <div><div class=\"bcard-lbl\">Card Holder</div><div class=\"bcard-val\">Account Holder</div></div>\n"
        "          <div><div class=\"bcard-lbl\">Expires</div><div class=\"bcard-val\">08/27</div></div>\n"
        "        </div>\n"
        "        <div class=\"bcard-type\">Platinum Debit</div>\n"
        "      </div>\n"
        "      <div class=\"bank-card bank-card-green\">\n"
        "        <div class=\"bcard-top\">\n"
        "          <span class=\"bcard-bank\">NexusBank</span>\n"
        "          <span class=\"bcard-type-small\">RuPay</span>\n"
        "        </div>\n"
        "        <div class=\"bcard-number\">XXXX XXXX XXXX 6602</div>\n"
        "        <div class=\"bcard-bottom\">\n"
        "          <div><div class=\"bcard-lbl\">UPI Enabled</div><div class=\"bcard-val\">Active</div></div>\n"
        "          <div><div class=\"bcard-lbl\">Contactless</div><div class=\"bcard-val\">&#10003;</div></div>\n"
        "        </div>\n"
        "        <div class=\"bcard-type\">RuPay Debit</div>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"

        /* Tab 2: Credit Cards */
        "  <div class=\"ctab-panel\" id=\"cpanel2\">\n"
        "    <div class=\"credit-cards-grid\">\n"
        "      <div class=\"credit-card-offer\">\n"
        "        <div class=\"cc-header cc-gold\">\n"
        "          <span>Millennia Credit Card</span>\n"
        "          <span class=\"cc-tag\">Cashback</span>\n"
        "        </div>\n"
        "        <div class=\"cc-features\">\n"
        "          <div class=\"cc-feat\">&#10003; 5%% cashback on online spends</div>\n"
        "          <div class=\"cc-feat\">&#10003; 1%% on all other spends</div>\n"
        "          <div class=\"cc-feat\">&#10003; Zero lost card liability</div>\n"
        "        </div>\n"
        "        <div class=\"cc-fee\">Joining Fee: &#8377;500 + GST</div>\n"
        "        <form method=\"POST\" action=\"/cgi-bin/cards.cgi\">\n"
        "          <input type=\"hidden\" name=\"action\" value=\"apply-credit-card\">\n"
        "          <button class=\"btn btn-primary\" type=\"submit\">Apply Now</button>\n"
        "        </form>\n"
        "      </div>\n"
        "      <div class=\"credit-card-offer\">\n"
        "        <div class=\"cc-header cc-black\">\n"
        "          <span>Platinum Credit Card</span>\n"
        "          <span class=\"cc-tag\">Premium</span>\n"
        "        </div>\n"
        "        <div class=\"cc-features\">\n"
        "          <div class=\"cc-feat\">&#10003; Airport lounge access</div>\n"
        "          <div class=\"cc-feat\">&#10003; Golf privileges</div>\n"
        "          <div class=\"cc-feat\">&#10003; 2%% forex markup</div>\n"
        "        </div>\n"
        "        <div class=\"cc-fee\">Joining Fee: &#8377;2,500 + GST</div>\n"
        "        <form method=\"POST\" action=\"/cgi-bin/cards.cgi\">\n"
        "          <input type=\"hidden\" name=\"action\" value=\"apply-credit-card\">\n"
        "          <button class=\"btn btn-primary\" type=\"submit\">Apply Now</button>\n"
        "        </form>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"

        /* Tab 3: Card Management */
        "  <div class=\"ctab-panel\" id=\"cpanel3\">\n"
        "    <div class=\"mgmt-actions\">\n"
        "      <form method=\"POST\" action=\"/cgi-bin/cards.cgi\">\n"
        "        <input type=\"hidden\" name=\"action\" value=\"block\">\n"
        "        <button class=\"mgmt-btn mgmt-danger\" type=\"submit\">\n"
        "          <span class=\"mgmt-icon\">&#128274;</span>\n"
        "          <span>Block Card</span>\n"
        "          <span class=\"mgmt-sub\">Instantly block if lost or stolen</span>\n"
        "        </button>\n"
        "      </form>\n"
        "      <form method=\"POST\" action=\"/cgi-bin/cards.cgi\">\n"
        "        <input type=\"hidden\" name=\"action\" value=\"unblock\">\n"
        "        <button class=\"mgmt-btn\" type=\"submit\">\n"
        "          <span class=\"mgmt-icon\">&#128275;</span>\n"
        "          <span>Unblock Card</span>\n"
        "          <span class=\"mgmt-sub\">Reactivate your card</span>\n"
        "        </button>\n"
        "      </form>\n"
        "    </div>\n"
        "    <div class=\"fees-table-wrap\">\n"
        "      <h3>Fees &amp; Charges</h3>\n"
        "      <table class=\"fees-table\">\n"
        "        <thead><tr><th>Charge</th><th>Classic Debit</th><th>Millennia Credit</th><th>Platinum Credit</th></tr></thead>\n"
        "        <tbody>\n"
        "          <tr><td>Annual Fee</td><td>Free</td><td>&#8377;500 + GST</td><td>&#8377;2,500 + GST</td></tr>\n"
        "          <tr><td>Fee Waiver</td><td>—</td><td>Spend &#8377;1L/year</td><td>Spend &#8377;3L/year</td></tr>\n"
        "          <tr><td>Cash Withdrawal</td><td>2.5%%</td><td>2.5%%</td><td>2.5%%</td></tr>\n"
        "          <tr><td>Forex Markup</td><td>3.5%%</td><td>3.5%%</td><td>2%%</td></tr>\n"
        "        </tbody>\n"
        "      </table>\n"
        "    </div>\n"
        "  </div>\n"
        "</div>\n"
    );

    fprintf(cgiOut, "</main></div>");
    page_end();
    return 0;
}
