/* =========================================================
   html_utils.c - HTML helper function implementations
   ========================================================= */

#include "html_utils.h"
#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Print Content-Type header and open HTML tags */
void page_start(const char *title, const char *extra_css) {
    cgiHeaderContentType("text/html");
    fprintf(cgiOut,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "  <title>%s - NexusBank</title>\n"
        "  <link rel=\"stylesheet\" href=\"/css/main.css\">\n"
        "  <link rel=\"stylesheet\" href=\"/css/layout.css\">\n"
        "  <link rel=\"stylesheet\" href=\"/css/components.css\">\n"
        "  <link rel=\"stylesheet\" href=\"/css/pages.css\">\n"
        "  <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n"
        "  <link href=\"https://fonts.googleapis.com/css2?family=DM+Sans:wght@300;400;500;600&family=DM+Mono:wght@400;500&display=swap\" rel=\"stylesheet\">\n",
        title
    );
    if (extra_css && strlen(extra_css) > 0) {
        fprintf(cgiOut, "  <link rel=\"stylesheet\" href=\"/css/%s\">\n", extra_css);
    }
    fprintf(cgiOut,
        "</head>\n"
        "<body>\n"
    );
}

void page_end(void) {
    fprintf(cgiOut,
        "\n</body>\n"
        "</html>\n"
    );
}

/* Top nav for public pages */
void render_public_nav(void) {
    fprintf(cgiOut,
        "<nav class=\"pub-nav\">\n"
        "  <div class=\"nav-inner\">\n"
        "    <a href=\"/cgi-bin/home.cgi\" class=\"nav-logo\">\n"
        "      <span class=\"logo-nx\">Nexus</span><span class=\"logo-bk\">Bank</span>\n"
        "    </a>\n"
        "    <div class=\"nav-links\">\n"
        "      <a href=\"/cgi-bin/home.cgi\">Home</a>\n"
        "      <a href=\"/cgi-bin/home.cgi#services\">Services</a>\n"
        "      <a href=\"/cgi-bin/home.cgi#about\">About</a>\n"
        "    </div>\n"
        "    <div class=\"nav-actions\">\n"
        "      <a href=\"/cgi-bin/login.cgi\" class=\"btn btn-outline\">Login</a>\n"
        "      <a href=\"/cgi-bin/activate.cgi\" class=\"btn btn-primary\">Open Account</a>\n"
        "    </div>\n"
        "  </div>\n"
        "</nav>\n"
    );
}

/* Left sidebar shown on all protected pages */
void render_sidebar(const char *active_page, const char *user_name, const char *customer_id) {
    /* Helper macro to check active page for nav highlighting */
    #define IS_ACTIVE(p) (strcmp(active_page, p) == 0 ? "active" : "")

    fprintf(cgiOut,
        "<div class=\"app-layout\">\n"
        "  <aside class=\"sidebar\">\n"
        "    <div class=\"sidebar-logo\">\n"
        "      <span class=\"logo-nx\">Nexus</span><span class=\"logo-bk\">Bank</span>\n"
        "    </div>\n"
        "    <div class=\"sidebar-user\">\n"
        "      <div class=\"user-avatar\">%c</div>\n"
        "      <div class=\"user-info\">\n"
        "        <div class=\"user-name\">%s</div>\n"
        "        <div class=\"user-cid\">%s</div>\n"
        "      </div>\n"
        "    </div>\n"
        "    <nav class=\"sidebar-nav\">\n"
        "      <a href=\"/cgi-bin/dashboard.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9700;</span> Dashboard\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/transfer.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#8652;</span> Send Money\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/transactions.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9783;</span> Transactions\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/cards.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9646;</span> Cards\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/loans.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9679;</span> Loans\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/services.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9741;</span> Services\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/calculator.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9636;</span> EMI Calc\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/profile.cgi\" class=\"nav-item %s\">\n"
        "        <span class=\"nav-icon\">&#9737;</span> Profile\n"
        "      </a>\n"
        "      <a href=\"/cgi-bin/logout.cgi\" class=\"nav-item nav-logout\">\n"
        "        <span class=\"nav-icon\">&#10006;</span> Logout\n"
        "      </a>\n"
        "    </nav>\n"
        "  </aside>\n"
        "  <main class=\"main-content\">\n",
        /* First letter of name as avatar */
        (user_name && strlen(user_name) > 0) ? toupper(user_name[0]) : 'U',
        user_name    ? user_name    : "User",
        customer_id  ? customer_id  : "",
        /* Active states */
        IS_ACTIVE("dashboard"),
        IS_ACTIVE("transfer"),
        IS_ACTIVE("transactions"),
        IS_ACTIVE("cards"),
        IS_ACTIVE("loans"),
        IS_ACTIVE("services"),
        IS_ACTIVE("calculator"),
        IS_ACTIVE("profile")
    );
}

/* Close the app layout divs opened by render_sidebar */
void page_end_protected(void) {
    fprintf(cgiOut,
        "  </main>\n"
        "</div>\n"
    );
    page_end();
}

void show_error(const char *msg) {
    fprintf(cgiOut,
        "<div class=\"alert alert-error\">\n"
        "  <span class=\"alert-icon\">&#9888;</span>\n"
        "  <span>%s</span>\n"
        "</div>\n",
        msg
    );
}

void show_success(const char *msg) {
    fprintf(cgiOut,
        "<div class=\"alert alert-success\">\n"
        "  <span class=\"alert-icon\">&#10003;</span>\n"
        "  <span>%s</span>\n"
        "</div>\n",
        msg
    );
}

void form_field(const char *label, const char *name, const char *type,
                const char *value, const char *placeholder, int required) {
    fprintf(cgiOut,
        "<div class=\"field-group\">\n"
        "  <label class=\"field-label\">%s%s</label>\n"
        "  <input class=\"field-input\"\n"
        "         type=\"%s\"\n"
        "         name=\"%s\"\n"
        "         value=\"%s\"\n"
        "         placeholder=\"%s\"\n"
        "         %s>\n"
        "</div>\n",
        label,
        required ? " <span class=\"required\">*</span>" : "",
        type  ? type  : "text",
        name  ? name  : "",
        value ? value : "",
        placeholder ? placeholder : "",
        required ? "required" : ""
    );
}

/* Escape characters that could cause XSS */
void html_safe(const char *input, char *output, int max_len) {
    int i     = 0;
    int o     = 0;

    if (!input) {
        output[0] = '\0';
        return;
    }

    while (input[i] && o < max_len - 10) {
        switch (input[i]) {
            case '&':
                strcpy(output + o, "&amp;");
                o += 5;
                break;
            case '<':
                strcpy(output + o, "&lt;");
                o += 4;
                break;
            case '>':
                strcpy(output + o, "&gt;");
                o += 4;
                break;
            case '"':
                strcpy(output + o, "&quot;");
                o += 6;
                break;
            case '\'':
                strcpy(output + o, "&#39;");
                o += 5;
                break;
            default:
                output[o++] = input[i];
        }
        i++;
    }
    output[o] = '\0';
}

/* OTP input section with a CSS countdown timer */
void render_otp_section(const char *customer_id, const char *purpose) {
    fprintf(cgiOut,
        "<div class=\"otp-section\">\n"
        "  <div class=\"otp-info\">\n"
        "    <p>OTP sent to your registered mobile number.</p>\n"
        "    <p class=\"otp-demo-note\">For demo purposes, OTP is shown here:</p>\n"
        "    <div class=\"otp-display\" id=\"otp-demo-val\">Check below</div>\n"
        "  </div>\n"
        "  <div class=\"field-group\">\n"
        "    <label class=\"field-label\">Enter OTP <span class=\"required\">*</span></label>\n"
        "    <input class=\"field-input otp-input\" type=\"text\" name=\"otp_code\"\n"
        "           maxlength=\"6\" placeholder=\"6-digit OTP\" required>\n"
        "  </div>\n"
        /* CSS animation for 60-second countdown */
        "  <div class=\"otp-timer\">\n"
        "    <div class=\"timer-ring\">\n"
        "      <div class=\"timer-ring-fill\"></div>\n"
        "    </div>\n"
        "    <span class=\"timer-label\">60s</span>\n"
        "  </div>\n"
        "  <input type=\"hidden\" name=\"customer_id\" value=\"%s\">\n"
        "  <input type=\"hidden\" name=\"purpose\" value=\"%s\">\n"
        "</div>\n",
        customer_id ? customer_id : "",
        purpose     ? purpose     : ""
    );
}

/* Show step indicators at top of multi-step forms */
void render_steps(int total, int current, const char *names[]) {
    fprintf(cgiOut, "<div class=\"steps-bar\">\n");

    for (int i = 1; i <= total; i++) {
        const char *state = "";
        if (i < current)  state = "done";
        if (i == current) state = "active";

        fprintf(cgiOut,
            "  <div class=\"step-item %s\">\n"
            "    <div class=\"step-circle\">%s</div>\n"
            "    <div class=\"step-label\">%s</div>\n"
            "  </div>\n",
            state,
            i < current ? "&#10003;" : (char[]){'0' + i, '\0'},
            (names && names[i-1]) ? names[i-1] : ""
        );

        if (i < total) {
            fprintf(cgiOut, "  <div class=\"step-line %s\"></div>\n",
                    i < current ? "done" : "");
        }
    }

    fprintf(cgiOut, "</div>\n");
}
