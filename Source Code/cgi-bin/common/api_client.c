/* =========================================================
   api_client.c - libcurl wrapper for talking to Rust backend
   ========================================================= */

#include "api_client.h"
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

/* libcurl needs this callback to collect the response body */
static size_t collect_response(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total    = size * nmemb;
    char  *buf      = (char*)userp;
    size_t curr_len = strlen(buf);

    /* Don't overflow the buffer */
    if (curr_len + total < MAX_RESP_LEN - 1) {
        strncat(buf, (char*)contents, total);
    }

    return total;
}

/* POST request to Rust */
int rust_post(const char *endpoint, const char *json_body, char *response, int max_len) {
    CURL    *curl;
    CURLcode res;
    char     full_url[512];
    int      success = 0;

    /* Build full URL */
    snprintf(full_url, sizeof(full_url), "%s%s", RUST_BASE_URL, endpoint);

    memset(response, 0, max_len);

    curl = curl_easy_init();
    if (!curl) {
        return 0;
    }

    /* Set up the request */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,            full_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     json_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  collect_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        success = (http_code == 200) ? 1 : 0;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return success;
}

/* GET request */
int rust_get(const char *url, char *response, int max_len) {
    CURL    *curl;
    CURLcode res;
    int      success = 0;

    memset(response, 0, max_len);

    curl = curl_easy_init();
    if (!curl) {
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL,           url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, collect_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        success = (http_code == 200) ? 1 : 0;
    }

    curl_easy_cleanup(curl);

    return success;
}

/* Check if the response JSON says ok:true */
int resp_is_ok(const char *response) {
    return strstr(response, "\"ok\":true") != NULL ? 1 : 0;
}

/* Pull a string field out of a simple JSON response */
/* This is a basic parser - good enough for our controlled responses */
void resp_get_field(const char *response, const char *field, char *out, int max_out) {
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\":\"", field);

    memset(out, 0, max_out);

    const char *found = strstr(response, search_key);
    if (!found) {
        /* Also try without quotes (for numbers/booleans) */
        snprintf(search_key, sizeof(search_key), "\"%s\":", field);
        found = strstr(response, search_key);
        if (!found) return;
        found += strlen(search_key);
        /* Copy until comma or } */
        int i = 0;
        while (found[i] && found[i] != ',' && found[i] != '}' && i < max_out-1) {
            out[i] = found[i];
            i++;
        }
        return;
    }

    found += strlen(search_key);

    /* Copy until closing quote */
    int i = 0;
    while (found[i] && found[i] != '"' && i < max_out-1) {
        out[i] = found[i];
        i++;
    }
}
