/* =========================================================
   session.c - Implementation of session helper functions
   ========================================================= */

#include "session.h"
#include "api_client.h"
#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Read the session token from the cookie header */
char* read_session_cookie(void) {
    static char token_buf[MAX_TOKEN_LEN + 1];
    memset(token_buf, 0, sizeof(token_buf));

    /* cgic gives us cookie values */
    if (cgiCookieString(SESSION_COOKIE_NAME, token_buf, MAX_TOKEN_LEN) == cgiFormSuccess) {
        return token_buf;
    }

    return NULL;
}

/* Verify the session token with Rust backend */
SessionInfo verify_session(const char *token) {
    SessionInfo info;
    info.is_valid = 0;
    memset(info.customer_id, 0, sizeof(info.customer_id));

    if (!token || strlen(token) == 0) {
        return info;
    }

    /* Build the URL with token as query param */
    char url[512];
    snprintf(url, sizeof(url), "%s?token=%s", SESSION_VERIFY_URL, token);

    char response[2048];
    memset(response, 0, sizeof(response));

    int ok = rust_get(url, response, sizeof(response));
    if (!ok) {
        return info;
    }

    /* Parse the JSON response manually - look for "valid":true */
    if (strstr(response, "\"valid\":true") != NULL) {
        info.is_valid = 1;

        /* Extract customer_id from JSON */
        char *cid_start = strstr(response, "\"customer_id\":\"");
        if (cid_start) {
            cid_start += 15; /* skip past the key */
            char *cid_end = strchr(cid_start, '"');
            if (cid_end) {
                int len = (int)(cid_end - cid_start);
                if (len > MAX_CID_LEN) len = MAX_CID_LEN;
                strncpy(info.customer_id, cid_start, len);
            }
        }
    }

    return info;
}

/* Set the session cookie - called right after successful login */
void set_session_cookie(const char *token) {
    /* HttpOnly means JavaScript can't read it (more secure) */
    /* Max-Age is 30 minutes = 1800 seconds */
    fprintf(cgiOut, "Set-Cookie: %s=%s; Path=/; Max-Age=1800; HttpOnly\r\n",
            SESSION_COOKIE_NAME, token);
}

/* Expire the cookie to log the user out */
void clear_session_cookie(void) {
    fprintf(cgiOut, "Set-Cookie: %s=; Path=/; Max-Age=0; HttpOnly\r\n",
            SESSION_COOKIE_NAME);
}
