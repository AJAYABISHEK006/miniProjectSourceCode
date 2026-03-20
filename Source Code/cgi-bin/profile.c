/* =========================================================
   profile.c - Profile and Settings page
   View and update personal details, change password
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

    /* Handle password change */
    if (strcmp(req_method, "POST") == 0) {
        char action[32];
        cgiFormString("action", action, 32);

        if (strcmp(action, "change_password") == 0) {
            char cur_pwd[128], new_pwd[128], confirm_pwd[128];
            cgiFormString("current_pwd", cur_pwd,     128);
            cgiFormString("new_pwd",     new_pwd,     128);
            cgiFormString("confirm_pwd", confirm_pwd, 128);

            if (strcmp(new_pwd, confirm_pwd) != 0) {
                /* Re-render with error */
                goto show_profile;
            }

            char jbody[512];
            snprintf(jbody, sizeof(jbody),
                     "{\"session_token\":\"%s\",\"current_pwd\":\"%s\",\"new_pwd\":\"%s\"}",
                     token, cur_pwd, new_pwd);

            char resp[1024];
            rust_post("/api/profile/change-password", jbody, resp, sizeof(resp));

            /* Will show result after redirect */
            cgiHeaderLocation("/cgi-bin/profile.cgi");
            return 0;
        }
    }

show_profile:;
    /* Get profile data */
    char api_url[256];
    snprintf(api_url, sizeof(api_url),
             "http://rust-backend:8080/api/profile/%s", cid);

    char resp[4096];
    rust_get(api_url, resp, sizeof(resp));

    char full_name[128], mobile[16], email[128], pan[20], aadhaar[24];
    char city[64], state_n[64], pin_code[8], dob[16], gender[16];
    char acc_num[24], acc_type[32], ifsc[16], branch[100];

    resp_get_field(resp, "full_name",     full_name, sizeof(full_name));
    resp_get_field(resp, "mobile",        mobile,    sizeof(mobile));
    resp_get_field(resp, "email",         email,     sizeof(email));
    resp_get_field(resp, "pan_masked",    pan,       sizeof(pan));
    resp_get_field(resp, "aadhaar_masked",aadhaar,   sizeof(aadhaar));
    resp_get_field(resp, "city",          city,      sizeof(city));
    resp_get_field(resp, "state",         state_n,   sizeof(state_n));
    resp_get_field(resp, "pin_code",      pin_code,  sizeof(pin_code));
    resp_get_field(resp, "dob",           dob,       sizeof(dob));
    resp_get_field(resp, "gender",        gender,    sizeof(gender));

    /* Account sub-object */
    char *acc_block = strstr(resp, "\"account\":");
    if (acc_block) {
        char tmp[1024];
        strncpy(tmp, acc_block, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        resp_get_field(tmp, "acc_number", acc_num,  sizeof(acc_num));
        resp_get_field(tmp, "acc_type",   acc_type, sizeof(acc_type));
        resp_get_field(tmp, "ifsc",       ifsc,     sizeof(ifsc));
        resp_get_field(tmp, "branch",     branch,   sizeof(branch));
    }

    page_start("My Profile", "");
    render_sidebar("profile", full_name, cid);

    fprintf(cgiOut,
        "<div class=\"page-header\">\n"
        "  <div class=\"profile-avatar-big\">%c</div>\n"
        "  <div>\n"
        "    <h1 class=\"page-title\">%s</h1>\n"
        "    <p class=\"page-sub\">Customer ID: %s</p>\n"
        "  </div>\n"
        "</div>\n",
        strlen(full_name) > 0 ? full_name[0] : 'U',
        full_name, cid
    );

    /* CSS radio-button tab system */
    fprintf(cgiOut,
        "<div class=\"profile-tabs\">\n"
        "<input type=\"radio\" name=\"ptab\" id=\"pt1\" class=\"ptab-radio\" checked>\n"
        "<input type=\"radio\" name=\"ptab\" id=\"pt2\" class=\"ptab-radio\">\n"
        "<input type=\"radio\" name=\"ptab\" id=\"pt3\" class=\"ptab-radio\">\n"
        "<input type=\"radio\" name=\"ptab\" id=\"pt4\" class=\"ptab-radio\">\n"
        "<div class=\"ptab-labels\">\n"
        "  <label for=\"pt1\" class=\"ptab-label\">Personal Details</label>\n"
        "  <label for=\"pt2\" class=\"ptab-label\">Contact Details</label>\n"
        "  <label for=\"pt3\" class=\"ptab-label\">Account Details</label>\n"
        "  <label for=\"pt4\" class=\"ptab-label\">Security</label>\n"
        "</div>\n"

        /* Tab 1: Personal details */
        "<div class=\"ptab-panel\" id=\"pp1\">\n"
        "  <div class=\"detail-grid\">\n"
        "    <div class=\"detail-item\"><span>Full Name</span><strong>%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>Date of Birth</span><strong>%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>Gender</span><strong>%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>PAN Number</span><strong class=\"mono\">%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>Aadhaar</span><strong class=\"mono\">%s</strong></div>\n"
        "  </div>\n"
        "</div>\n",
        full_name, dob, gender,
        strlen(pan) > 0 ? pan : "Not on file",
        strlen(aadhaar) > 0 ? aadhaar : "Not on file"
    );

    fprintf(cgiOut,
        /* Tab 2: Contact details */
        "<div class=\"ptab-panel\" id=\"pp2\">\n"
        "  <div class=\"detail-grid\">\n"
        "    <div class=\"detail-item\"><span>Mobile Number</span><strong>%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>Email Address</span><strong>%s</strong></div>\n"
        "    <div class=\"detail-item detail-full\"><span>Address</span><strong>%s, %s, %s - %s</strong></div>\n"
        "  </div>\n"
        "  <a href=\"/cgi-bin/services.cgi?sr_type=Address+Change\" class=\"btn btn-outline\">Update Address</a>\n"
        "  <a href=\"/cgi-bin/services.cgi?sr_type=Update+Email+ID\" class=\"btn btn-outline\">Update Email</a>\n"
        "</div>\n"

        /* Tab 3: Account details */
        "<div class=\"ptab-panel\" id=\"pp3\">\n"
        "  <div class=\"detail-grid\">\n"
        "    <div class=\"detail-item\"><span>Account Number</span><strong class=\"mono\">%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>Account Type</span><strong>%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>IFSC Code</span><strong class=\"mono\">%s</strong></div>\n"
        "    <div class=\"detail-item\"><span>Branch</span><strong>%s</strong></div>\n"
        "  </div>\n"
        "</div>\n",
        mobile, email,
        strlen(city) > 0 ? city : "", state_n, pin_code, "",
        acc_num, acc_type, ifsc, branch
    );

    fprintf(cgiOut,
        /* Tab 4: Security / Change password */
        "<div class=\"ptab-panel\" id=\"pp4\">\n"
        "  <h3>Change Password</h3>\n"
        "  <div class=\"form-card\">\n"
        "  <form method=\"POST\" action=\"/cgi-bin/profile.cgi\">\n"
        "    <input type=\"hidden\" name=\"action\" value=\"change_password\">\n"
        "    <div class=\"field-group\">\n"
        "      <label class=\"field-label\">Current Password *</label>\n"
        "      <input class=\"field-input\" type=\"password\" name=\"current_pwd\" required>\n"
        "    </div>\n"
        "    <div class=\"field-group\">\n"
        "      <label class=\"field-label\">New Password *</label>\n"
        "      <input class=\"field-input\" type=\"password\" name=\"new_pwd\"\n"
        "             minlength=\"8\" required>\n"
        "    </div>\n"
        "    <div class=\"field-group\">\n"
        "      <label class=\"field-label\">Confirm New Password *</label>\n"
        "      <input class=\"field-input\" type=\"password\" name=\"confirm_pwd\" required>\n"
        "    </div>\n"
        "    <button class=\"btn btn-primary\" type=\"submit\">Update Password</button>\n"
        "  </form>\n"
        "  </div>\n"
        "</div>\n"
        "</div>\n"
    );

    fprintf(cgiOut, "</main></div>");
    page_end();
    return 0;
}
