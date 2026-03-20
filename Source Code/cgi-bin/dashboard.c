/* =========================================================
   dashboard.c - Main dashboard after login
   Shows balance, quick actions, recent transactions
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../common/html_utils.h"
#include "../common/api_client.h"
#include "../common/session.h"

int cgiMain() {
    /* Must be logged in to see this page */
    char *token = read_session_cookie();
    SessionInfo sess = verify_session(token ? token : "");

    if (!sess.is_valid) {
        cgiHeaderLocation("/cgi-bin/login.cgi");
        return 0;
    }

    const char *cid = sess.customer_id;

    /* Get dashboard data from Rust */
    char url[256];
    snprintf(url, sizeof(url),
             "http://rust-backend:8080/api/dashboard/%s", cid);

    char resp[8192];
    int ok = rust_get(url, resp, sizeof(resp));

    /* Parse what we need */
    char full_name[128], first_name[64], balance[32];
    char acc_masked[32], acc_type[32], ifsc[20], branch[100];

    resp_get_field(resp, "full_name",  full_name,  sizeof(full_name));
    resp_get_field(resp, "first_name", first_name, sizeof(first_name));

    /* These are nested in "account" object - extract manually */
    char *acc_block = strstr(resp, "\"account\":");
    if (acc_block) {
        char tmp[4096];
        strncpy(tmp, acc_block, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        resp_get_field(tmp, "balance",    balance,    sizeof(balance));
        resp_get_field(tmp, "acc_masked", acc_masked, sizeof(acc_masked));
        resp_get_field(tmp, "acc_type",   acc_type,   sizeof(acc_type));
        resp_get_field(tmp, "ifsc",       ifsc,       sizeof(ifsc));
        resp_get_field(tmp, "branch",     branch,     sizeof(branch));
    }

    /* Greeting based on time of day */
    time_t now   = time(NULL);
    struct tm *t  = localtime(&now);
    const char *greeting;
    if (t->tm_hour < 12)       greeting = "Good Morning";
    else if (t->tm_hour < 17)  greeting = "Good Afternoon";
    else                       greeting = "Good Evening";

    page_start("Dashboard", "");
    render_sidebar("dashboard", full_name, cid);

    /* ---- Page Header ---- */
    fprintf(cgiOut,
        "<div class=\"page-header\">\n"
        "  <div>\n"
        "    <h1 class=\"page-title\">%s, %s &#128075;</h1>\n"
        "    <p class=\"page-sub\">Here's what's happening with your account today.</p>\n"
        "  </div>\n"
        "</div>\n",
        greeting,
        strlen(first_name) > 0 ? first_name : "there"
    );

    /* ---- Account Summary Card ---- */
    fprintf(cgiOut,
        "<div class=\"account-card\">\n"
        "  <div class=\"account-left\">\n"
        "    <div class=\"account-label\">Available Balance</div>\n"
        /* Show/hide balance using CSS checkbox trick */
        "    <input type=\"checkbox\" id=\"bal-toggle\" class=\"bal-toggle-check\">\n"
        "    <div class=\"balance-wrap\">\n"
        "      <div class=\"balance-shown\">&#8377; %s</div>\n"
        "      <div class=\"balance-hidden\">&#8377; &#9679;&#9679;&#9679;&#9679;&#9679;</div>\n"
        "    </div>\n"
        "    <label for=\"bal-toggle\" class=\"bal-toggle-btn\">Show / Hide Balance</label>\n"
        "  </div>\n"
        "  <div class=\"account-right\">\n"
        "    <div class=\"account-detail\">\n"
        "      <span class=\"detail-label\">Account Number</span>\n"
        "      <span class=\"detail-val\">%s</span>\n"
        "    </div>\n"
        "    <div class=\"account-detail\">\n"
        "      <span class=\"detail-label\">Account Type</span>\n"
        "      <span class=\"detail-val\"><span class=\"acc-type-badge\">%s</span></span>\n"
        "    </div>\n"
        "    <div class=\"account-detail\">\n"
        "      <span class=\"detail-label\">IFSC Code</span>\n"
        "      <span class=\"detail-val mono\">%s</span>\n"
        "    </div>\n"
        "    <div class=\"account-detail\">\n"
        "      <span class=\"detail-label\">Branch</span>\n"
        "      <span class=\"detail-val\">%s</span>\n"
        "    </div>\n"
        "  </div>\n"
        "</div>\n",
        strlen(balance) > 0 ? balance : "0.00",
        strlen(acc_masked) > 0 ? acc_masked : "XXXXXXXXXXXX",
        strlen(acc_type) > 0 ? acc_type : "Savings",
        strlen(ifsc) > 0 ? ifsc : "--",
        strlen(branch) > 0 ? branch : "--"
    );

    /* ---- Quick Action Tiles ---- */
    fprintf(cgiOut,
        "<div class=\"section-title\">Quick Actions</div>\n"
        "<div class=\"quick-actions\">\n"
        "  <a href=\"/cgi-bin/transfer.cgi\" class=\"quick-tile\">\n"
        "    <div class=\"tile-icon\">&#8652;</div>\n"
        "    <div class=\"tile-label\">Send Money</div>\n"
        "    <div class=\"tile-sub\">NEFT · IMPS · UPI</div>\n"
        "  </a>\n"
        "  <a href=\"/cgi-bin/transactions.cgi\" class=\"quick-tile\">\n"
        "    <div class=\"tile-icon\">&#9783;</div>\n"
        "    <div class=\"tile-label\">Transactions</div>\n"
        "    <div class=\"tile-sub\">View history</div>\n"
        "  </a>\n"
        "  <a href=\"/cgi-bin/cards.cgi\" class=\"quick-tile\">\n"
        "    <div class=\"tile-icon\">&#9646;</div>\n"
        "    <div class=\"tile-label\">My Cards</div>\n"
        "    <div class=\"tile-sub\">Manage cards</div>\n"
        "  </a>\n"
        "  <a href=\"/cgi-bin/loans.cgi\" class=\"quick-tile\">\n"
        "    <div class=\"tile-icon\">&#9679;</div>\n"
        "    <div class=\"tile-label\">Apply Loan</div>\n"
        "    <div class=\"tile-sub\">Personal · Home · Education</div>\n"
        "  </a>\n"
        "  <a href=\"/cgi-bin/services.cgi\" class=\"quick-tile\">\n"
        "    <div class=\"tile-icon\">&#9741;</div>\n"
        "    <div class=\"tile-label\">Services</div>\n"
        "    <div class=\"tile-sub\">Address · KYC · More</div>\n"
        "  </a>\n"
        "  <a href=\"/cgi-bin/calculator.cgi\" class=\"quick-tile\">\n"
        "    <div class=\"tile-icon\">&#9636;</div>\n"
        "    <div class=\"tile-label\">EMI Calculator</div>\n"
        "    <div class=\"tile-sub\">Plan your loan</div>\n"
        "  </a>\n"
        "</div>\n"
    );

    /* ---- Recent Transactions ---- */
    fprintf(cgiOut,
        "<div class=\"section-header-row\">\n"
        "  <div class=\"section-title\">Recent Transactions</div>\n"
        "  <a href=\"/cgi-bin/transactions.cgi\" class=\"see-all-link\">View All &rarr;</a>\n"
        "</div>\n"
        "<div class=\"txn-list\">\n"
    );

    /* Parse recent transactions from response */
    char *txn_block = strstr(resp, "\"recent_txns\":[");
    if (txn_block && ok) {
        /* Walk through and print each transaction */
        char *cursor = txn_block + 15;
        int txn_count = 0;

        while (*cursor && *cursor != ']' && txn_count < 5) {
            if (*cursor == '{') {
                /* Extract fields for this transaction */
                char txn_date[32], txn_desc[128], txn_type[16], txn_amt[32];
                char small_block[512];
                char *end = strchr(cursor, '}');
                if (!end) break;

                int blen = (int)(end - cursor + 1);
                if (blen > 511) blen = 511;
                strncpy(small_block, cursor, blen);
                small_block[blen] = '\0';

                resp_get_field(small_block, "date",        txn_date, sizeof(txn_date));
                resp_get_field(small_block, "description", txn_desc, sizeof(txn_desc));
                resp_get_field(small_block, "type",        txn_type, sizeof(txn_type));
                resp_get_field(small_block, "amount",      txn_amt,  sizeof(txn_amt));

                const char *credit_class = strcmp(txn_type, "Credit") == 0
                                           ? "txn-credit" : "txn-debit";
                const char *sign = strcmp(txn_type, "Credit") == 0 ? "+" : "-";

                fprintf(cgiOut,
                    "  <div class=\"txn-row\">\n"
                    "    <div class=\"txn-left\">\n"
                    "      <div class=\"txn-icon %s\">%s</div>\n"
                    "      <div>\n"
                    "        <div class=\"txn-desc\">%s</div>\n"
                    "        <div class=\"txn-date\">%s</div>\n"
                    "      </div>\n"
                    "    </div>\n"
                    "    <div class=\"txn-amount %s\">%s&#8377; %s</div>\n"
                    "  </div>\n",
                    credit_class,
                    strcmp(txn_type, "Credit") == 0 ? "&#8593;" : "&#8595;",
                    strlen(txn_desc) > 0 ? txn_desc : "Transaction",
                    txn_date,
                    credit_class,
                    sign,
                    txn_amt
                );

                cursor = end + 1;
                txn_count++;
            } else {
                cursor++;
            }
        }
    }

    if (strstr(resp, "\"recent_txns\":[]") || !ok) {
        fprintf(cgiOut,
            "  <div class=\"empty-state\">\n"
            "    <p>No transactions yet.</p>\n"
            "  </div>\n"
        );
    }

    fprintf(cgiOut, "</div>\n");

    /* Close main-content and app-layout */
    fprintf(cgiOut, "  </main>\n</div>\n");
    page_end();
    return 0;
}
