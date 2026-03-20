/* =========================================================
   html_utils.h - Common HTML building helpers for all pages
   ========================================================= */

#ifndef HTML_UTILS_H
#define HTML_UTILS_H

/* Print the HTML head section with CSS link */
void page_start(const char *title, const char *extra_css);

/* Print closing HTML tags */
void page_end(void);

/* Top navbar for public pages (home, login) */
void render_public_nav(void);

/* Left sidebar for logged-in pages */
void render_sidebar(const char *active_page, const char *user_name, const char *customer_id);

/* Red error box */
void show_error(const char *msg);

/* Green success box */
void show_success(const char *msg);

/* Render a standard form input field */
void form_field(const char *label, const char *name, const char *type,
                const char *value, const char *placeholder, int required);

/* Make user input safe to put in HTML (prevent XSS) */
void html_safe(const char *input, char *output, int max_len);

/* Render the OTP input section with 60s timer */
void render_otp_section(const char *customer_id, const char *purpose);

/* Progress steps bar for multi-step flows */
void render_steps(int total_steps, int current_step, const char *step_names[]);

#endif /* HTML_UTILS_H */
