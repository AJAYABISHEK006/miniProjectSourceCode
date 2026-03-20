/* =========================================================
   api_client.h - HTTP calls from C CGI to Rust backend
   Uses libcurl to make the requests
   ========================================================= */

#ifndef API_CLIENT_H
#define API_CLIENT_H

#define RUST_BASE_URL  "http://rust-backend:8080"
#define MAX_RESP_LEN   8192

/* Send a POST request to the Rust backend */
/* json_body: JSON string to send as body */
/* response: buffer to write response into */
/* Returns 1 on success, 0 on failure */
int rust_post(const char *endpoint, const char *json_body, char *response, int max_len);

/* Send a GET request */
int rust_get(const char *url, char *response, int max_len);

/* Quick helpers to check the response JSON */
int resp_is_ok(const char *response);
void resp_get_field(const char *response, const char *field, char *out, int max_out);

#endif /* API_CLIENT_H */
