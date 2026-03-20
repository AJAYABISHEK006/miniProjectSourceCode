/* =========================================================
   transactions.c - Transaction History Page
   Filter by type, date range, search description
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

    /* Read filter values from query string */
    char filter_type[16], search_txt[64], date_from[16], date_to[16], page_str[8];
    cgiFormString("type",   filter_type, 16);
    cgiFormString("search", search_txt,  64);
    cgiFormString("from",   date_from,   16);
    cgiFormString("to",     date_to,     16);
    cgiFormString("page",   page_str,    8);

    int page_num = atoi(page_str);
    if (page_num < 1) page_num = 1;

    /* Build the API URL with query params */
    char api_url[512];
    snprintf(api_url, sizeof(api_url),
             "http://rust-backend:8080/api/transactions/%s?type=%s&search=%s&from=%s&to=%s&page=%d",
             cid, filter_type, search_txt, date_from, date_to, page_num);

    char resp[8192];
    rust_get(api_url, resp, sizeof(resp));

    char cur_bal[32];
    resp_get_field(resp, "balance", cur_bal, sizeof(cur_bal));

    page_start("Transaction History", "");
    render_sidebar("transactions", "", cid);

    fprintf(cgiOut,
        "<div class=\"page-header\">\n"
        "  <h1 class=\"page-title\">Transaction History</h1>\n"
        "  <div class=\"balance-pill\">Balance: &#8377; %s</div>\n"
        "</div>\n",
        strlen(cur_bal) > 0 ? cur_bal : "0.00"
    );

    /* ---- Filter Bar ---- */
    fprintf(cgiOut,
        "<form class=\"filter-bar\" method=\"GET\" action=\"/cgi-bin/transactions.cgi\">\n"
        "  <div class=\"filter-group\">\n"
        "    <label>Type</label>\n"
        "    <select class=\"field-input\" name=\"type\">\n"
        "      <option value=\"\"%s>All</option>\n"
        "      <option value=\"Credit\"%s>Credits</option>\n"
        "      <option value=\"Debit\"%s>Debits</option>\n"
        "    </select>\n"
        "  </div>\n"
        "  <div class=\"filter-group\">\n"
        "    <label>From</label>\n"
        "    <input class=\"field-input\" type=\"date\" name=\"from\" value=\"%s\">\n"
        "  </div>\n"
        "  <div class=\"filter-group\">\n"
        "    <label>To</label>\n"
        "    <input class=\"field-input\" type=\"date\" name=\"to\" value=\"%s\">\n"
        "  </div>\n"
        "  <div class=\"filter-group filter-search\">\n"
        "    <label>Search</label>\n"
        "    <input class=\"field-input\" type=\"text\" name=\"search\"\n"
        "           placeholder=\"Search by description...\" value=\"%s\">\n"
        "  </div>\n"
        "  <button class=\"btn btn-outline\" type=\"submit\">Apply</button>\n"
        "  <a href=\"/cgi-bin/transactions.cgi\" class=\"btn btn-ghost\">Clear</a>\n"
        "</form>\n",
        strlen(filter_type) == 0 ? " selected" : "",
        strcmp(filter_type, "Credit") == 0 ? " selected" : "",
        strcmp(filter_type, "Debit")  == 0 ? " selected" : "",
        date_from, date_to, search_txt
    );

    /* ---- Transaction Table ---- */
    fprintf(cgiOut,
        "<div class=\"txn-table-wrap\">\n"
        "<table class=\"txn-table\">\n"
        "<thead>\n"
        "<tr>\n"
        "  <th>Date &amp; Time</th>\n"
        "  <th>Description</th>\n"
        "  <th>Type</th>\n"
        "  <th>Amount</th>\n"
        "  <th>Balance After</th>\n"
        "  <th>Ref ID</th>\n"
        "</tr>\n"
        "</thead>\n"
        "<tbody>\n"
    );

    /* Parse transactions from the response */
    int found_any = 0;
    char *cursor = strstr(resp, "\"transactions\":[");
    if (cursor) {
        cursor += 16;
        while (*cursor && *cursor != ']') {
            if (*cursor == '{') {
                char *end_obj = strchr(cursor, '}');
                if (!end_obj) break;

                int blen = (int)(end_obj - cursor + 1);
                if (blen > 511) blen = 511;

                char block[512];
                strncpy(block, cursor, blen);
                block[blen] = '\0';

                char t_date[40], t_desc[128], t_type[16], t_amt[32], t_bal[32], t_ref[64];
                resp_get_field(block, "date",        t_date, sizeof(t_date));
                resp_get_field(block, "description", t_desc, sizeof(t_desc));
                resp_get_field(block, "type",        t_type, sizeof(t_type));
                resp_get_field(block, "amount",      t_amt,  sizeof(t_amt));
                resp_get_field(block, "bal_after",   t_bal,  sizeof(t_bal));
                resp_get_field(block, "ref_id",      t_ref,  sizeof(t_ref));

                const char *type_class = strcmp(t_type, "Credit") == 0
                                         ? "badge-credit" : "badge-debit";
                const char *amt_class  = strcmp(t_type, "Credit") == 0
                                         ? "amt-credit" : "amt-debit";
                const char *sign = strcmp(t_type, "Credit") == 0 ? "+" : "-";

                fprintf(cgiOut,
                    "<tr>\n"
                    "  <td class=\"mono\">%s</td>\n"
                    "  <td>%s</td>\n"
                    "  <td><span class=\"badge %s\">%s</span></td>\n"
                    "  <td class=\"%s\">%s&#8377; %s</td>\n"
                    "  <td class=\"mono\">&#8377; %s</td>\n"
                    "  <td class=\"mono ref-id\">%s</td>\n"
                    "</tr>\n",
                    t_date, t_desc, type_class, t_type,
                    amt_class, sign, t_amt, t_bal, t_ref
                );

                found_any = 1;
                cursor = end_obj + 1;
            } else {
                cursor++;
            }
        }
    }

    if (!found_any) {
        fprintf(cgiOut,
            "<tr><td colspan=\"6\" class=\"empty-row\">"
            "No transactions found for the selected filters."
            "</td></tr>\n"
        );
    }

    fprintf(cgiOut, "</tbody>\n</table>\n</div>\n");

    /* Pagination */
    fprintf(cgiOut,
        "<div class=\"pagination\">\n"
    );
    if (page_num > 1) {
        fprintf(cgiOut,
            "  <a href=\"/cgi-bin/transactions.cgi?page=%d&type=%s&search=%s\" "
            "class=\"btn btn-outline\">&larr; Previous</a>\n",
            page_num - 1, filter_type, search_txt
        );
    }
    if (found_any) {
        fprintf(cgiOut,
            "  <a href=\"/cgi-bin/transactions.cgi?page=%d&type=%s&search=%s\" "
            "class=\"btn btn-outline\">Next &rarr;</a>\n",
            page_num + 1, filter_type, search_txt
        );
    }
    fprintf(cgiOut, "</div>\n");

    fprintf(cgiOut, "</main></div>");
    page_end();
    return 0;
}
