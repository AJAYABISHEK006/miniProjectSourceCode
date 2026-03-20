/* =========================================================
   session.h - Session management for CGI programs
   Read the NEXUSBANK_SESSION cookie and verify with Rust
   ========================================================= */

#ifndef SESSION_H
#define SESSION_H

#define SESSION_COOKIE_NAME  "NEXUSBANK_SESSION"
#define SESSION_VERIFY_URL   "http://rust-backend:8080/api/auth/verify-session"
#define MAX_TOKEN_LEN        128
#define MAX_CID_LEN          20

/* Holds what we get back after verifying a session */
typedef struct {
    int   is_valid;
    char  customer_id[MAX_CID_LEN + 1];
} SessionInfo;

/* Read the session cookie from the HTTP request */
char* read_session_cookie(void);

/* Ask the Rust backend if this token is still alive */
SessionInfo verify_session(const char *token);

/* Set the session cookie (after login) */
void set_session_cookie(const char *token);

/* Clear the cookie (on logout) */
void clear_session_cookie(void);

#endif /* SESSION_H */
