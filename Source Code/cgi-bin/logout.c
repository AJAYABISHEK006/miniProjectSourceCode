/* =========================================================
   logout.c - Clear session and redirect to home
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include "../common/api_client.h"
#include "../common/session.h"

int cgiMain() {
    char *token = read_session_cookie();

    /* Tell Rust to kill this session */
    if (token && strlen(token) > 0) {
        char jbody[256];
        snprintf(jbody, sizeof(jbody), "{\"token\":\"%s\"}", token);

        char resp[512];
        rust_post("/api/auth/logout", jbody, resp, sizeof(resp));
    }

    /* Clear the session cookie and redirect to home */
    cgiHeaderContentType("text/html");
    clear_session_cookie();
    cgiHeaderLocation("/cgi-bin/home.cgi");
    return 0;
}
