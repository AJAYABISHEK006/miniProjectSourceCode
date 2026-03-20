/* =========================================================
   home.c - NexusBank Home Page (Public)
   This is the landing page everyone sees first
   ========================================================= */

#include <cgic.h>
#include <stdio.h>
#include <string.h>
#include "../common/html_utils.h"
#include "../common/session.h"

int cgiMain() {
    /* If someone is already logged in, just redirect to dashboard */
    char *existing_token = read_session_cookie();
    if (existing_token && strlen(existing_token) > 0) {
        cgiHeaderLocation("/cgi-bin/dashboard.cgi");
        return 0;
    }

    page_start("Welcome", "");
    render_public_nav();

    /* ---- Hero Section ---- */
    fprintf(cgiOut,
        "<section class=\"hero\">\n"
        "  <div class=\"hero-inner\">\n"
        "    <div class=\"hero-text\">\n"
        "      <div class=\"hero-tag\">Trusted by 50L+ Customers</div>\n"
        "      <h1 class=\"hero-heading\">\n"
        "        Banking<br>Made <span class=\"heading-accent\">Simple.</span>\n"
        "      </h1>\n"
        "      <p class=\"hero-sub\">\n"
        "        Secure, fast, and reliable NetBanking for modern India.\n"
        "        Transfer money, apply for loans, manage your cards — all in one place.\n"
        "      </p>\n"
        "      <div class=\"hero-cta\">\n"
        "        <a href=\"/cgi-bin/login.cgi\" class=\"btn btn-primary btn-lg\">Login to NetBanking</a>\n"
        "        <a href=\"/cgi-bin/activate.cgi\" class=\"btn btn-outline btn-lg\">Activate NetBanking</a>\n"
        "      </div>\n"
        "    </div>\n"
        "    <div class=\"hero-card-preview\">\n"
        "      <div class=\"preview-card\">\n"
        "        <div class=\"card-top\">\n"
        "          <span class=\"card-bank\">NexusBank</span>\n"
        "          <span class=\"card-chip\">&#9632;</span>\n"
        "        </div>\n"
        "        <div class=\"card-number\">4111 2233 4455 6601</div>\n"
        "        <div class=\"card-bottom\">\n"
        "          <div>\n"
        "            <div class=\"card-label\">Card Holder</div>\n"
        "            <div class=\"card-value\">ARJUN SHARMA</div>\n"
        "          </div>\n"
        "          <div>\n"
        "            <div class=\"card-label\">Expires</div>\n"
        "            <div class=\"card-value\">08/27</div>\n"
        "          </div>\n"
        "        </div>\n"
        "      </div>\n"
        "      <div class=\"preview-balance\">\n"
        "        <div class=\"bal-label\">Available Balance</div>\n"
        "        <div class=\"bal-amount\">&#8377; 4,28,750</div>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"
        "</section>\n"
    );

    /* ---- Stats Bar ---- */
    fprintf(cgiOut,
        "<section class=\"stats-bar\">\n"
        "  <div class=\"stats-inner\">\n"
        "    <div class=\"stat-item\">\n"
        "      <div class=\"stat-num\">50L+</div>\n"
        "      <div class=\"stat-lbl\">Happy Customers</div>\n"
        "    </div>\n"
        "    <div class=\"stat-divider\"></div>\n"
        "    <div class=\"stat-item\">\n"
        "      <div class=\"stat-num\">&#8377;10,000 Cr+</div>\n"
        "      <div class=\"stat-lbl\">Deposits Managed</div>\n"
        "    </div>\n"
        "    <div class=\"stat-divider\"></div>\n"
        "    <div class=\"stat-item\">\n"
        "      <div class=\"stat-num\">500+</div>\n"
        "      <div class=\"stat-lbl\">Branches Across India</div>\n"
        "    </div>\n"
        "    <div class=\"stat-divider\"></div>\n"
        "    <div class=\"stat-item\">\n"
        "      <div class=\"stat-num\">99.9%%</div>\n"
        "      <div class=\"stat-lbl\">Uptime Guaranteed</div>\n"
        "    </div>\n"
        "  </div>\n"
        "</section>\n"
    );

    /* ---- Services Section ---- */
    fprintf(cgiOut,
        "<section class=\"services\" id=\"services\">\n"
        "  <div class=\"section-inner\">\n"
        "    <div class=\"section-header\">\n"
        "      <h2>Everything You Need</h2>\n"
        "      <p>A complete banking suite, designed for simplicity.</p>\n"
        "    </div>\n"
        "    <div class=\"services-grid\">\n"
        "      <div class=\"service-card\">\n"
        "        <div class=\"service-icon\">&#8381;</div>\n"
        "        <h3>Savings Account</h3>\n"
        "        <p>Earn competitive interest on your savings with zero hidden charges.</p>\n"
        "      </div>\n"
        "      <div class=\"service-card\">\n"
        "        <div class=\"service-icon\">&#9646;</div>\n"
        "        <h3>Credit Cards</h3>\n"
        "        <p>Cashback, rewards, and zero fraud liability on all card transactions.</p>\n"
        "      </div>\n"
        "      <div class=\"service-card\">\n"
        "        <div class=\"service-icon\">&#9679;</div>\n"
        "        <h3>Personal Loans</h3>\n"
        "        <p>Quick loans up to &#8377;40 Lakhs with minimal documentation.</p>\n"
        "      </div>\n"
        "      <div class=\"service-card\">\n"
        "        <div class=\"service-icon\">&#8652;</div>\n"
        "        <h3>Instant Transfers</h3>\n"
        "        <p>NEFT, IMPS, and UPI transfers 24x7 with instant confirmation.</p>\n"
        "      </div>\n"
        "      <div class=\"service-card\">\n"
        "        <div class=\"service-icon\">&#9783;</div>\n"
        "        <h3>Fixed Deposits</h3>\n"
        "        <p>Guaranteed returns with flexible tenure options from 7 days to 10 years.</p>\n"
        "      </div>\n"
        "      <div class=\"service-card\">\n"
        "        <div class=\"service-icon\">&#9737;</div>\n"
        "        <h3>Home Loans</h3>\n"
        "        <p>Make your dream home a reality with loans starting at 8.5%% p.a.</p>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"
        "</section>\n"
    );

    /* ---- Why Choose Us ---- */
    fprintf(cgiOut,
        "<section class=\"why-us\" id=\"about\">\n"
        "  <div class=\"section-inner\">\n"
        "    <div class=\"section-header\">\n"
        "      <h2>Why NexusBank?</h2>\n"
        "      <p>We built the bank we always wished existed.</p>\n"
        "    </div>\n"
        "    <div class=\"why-grid\">\n"
        "      <div class=\"why-item\">\n"
        "        <div class=\"why-num\">01</div>\n"
        "        <h3>Bank-grade Security</h3>\n"
        "        <p>256-bit encryption, OTP verification, and real-time fraud monitoring protect every transaction.</p>\n"
        "      </div>\n"
        "      <div class=\"why-item\">\n"
        "        <div class=\"why-num\">02</div>\n"
        "        <h3>Simple Interface</h3>\n"
        "        <p>Clean design with no jargon. If your grandmother can use it, we've done our job.</p>\n"
        "      </div>\n"
        "      <div class=\"why-item\">\n"
        "        <div class=\"why-num\">03</div>\n"
        "        <h3>Always On</h3>\n"
        "        <p>99.9%% uptime with 24x7 IMPS transfers — banking doesn't stop when we sleep.</p>\n"
        "      </div>\n"
        "      <div class=\"why-item\">\n"
        "        <div class=\"why-num\">04</div>\n"
        "        <h3>Instant Support</h3>\n"
        "        <p>Resolve issues through in-app service requests. No calls, no queues.</p>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"
        "</section>\n"
    );

    /* ---- Quick Login CTA Banner ---- */
    fprintf(cgiOut,
        "<section class=\"cta-banner\">\n"
        "  <div class=\"cta-inner\">\n"
        "    <h2>Ready to get started?</h2>\n"
        "    <p>Login to your account or activate NetBanking in under 2 minutes.</p>\n"
        "    <div class=\"cta-buttons\">\n"
        "      <a href=\"/cgi-bin/login.cgi\" class=\"btn btn-white\">Login Now</a>\n"
        "      <a href=\"/cgi-bin/activate.cgi\" class=\"btn btn-outline-white\">Activate NetBanking</a>\n"
        "    </div>\n"
        "  </div>\n"
        "</section>\n"
    );

    /* ---- Footer ---- */
    fprintf(cgiOut,
        "<footer class=\"site-footer\">\n"
        "  <div class=\"footer-inner\">\n"
        "    <div class=\"footer-brand\">\n"
        "      <div class=\"footer-logo\">\n"
        "        <span class=\"logo-nx\">Nexus</span><span class=\"logo-bk\">Bank</span>\n"
        "      </div>\n"
        "      <p>Secure banking for modern India.</p>\n"
        "    </div>\n"
        "    <div class=\"footer-links\">\n"
        "      <div class=\"footer-col\">\n"
        "        <h4>Products</h4>\n"
        "        <a href=\"#\">Savings Account</a>\n"
        "        <a href=\"#\">Credit Cards</a>\n"
        "        <a href=\"#\">Personal Loans</a>\n"
        "        <a href=\"#\">Home Loans</a>\n"
        "      </div>\n"
        "      <div class=\"footer-col\">\n"
        "        <h4>Services</h4>\n"
        "        <a href=\"#\">Fund Transfer</a>\n"
        "        <a href=\"#\">Bill Payments</a>\n"
        "        <a href=\"#\">Fixed Deposits</a>\n"
        "        <a href=\"#\">EMI Calculator</a>\n"
        "      </div>\n"
        "      <div class=\"footer-col\">\n"
        "        <h4>Contact</h4>\n"
        "        <a href=\"#\">1800-XXX-XXXX (Toll Free)</a>\n"
        "        <a href=\"#\">support@nexusbank.in</a>\n"
        "        <a href=\"#\">Find a Branch</a>\n"
        "      </div>\n"
        "    </div>\n"
        "  </div>\n"
        "  <div class=\"footer-bottom\">\n"
        "    <p>&copy; 2024 NexusBank. All rights reserved.</p>\n"
        "    <div class=\"footer-legal\">\n"
        "      <a href=\"#\">Privacy Policy</a>\n"
        "      <a href=\"#\">Terms of Use</a>\n"
        "      <a href=\"#\">Grievance Redressal</a>\n"
        "    </div>\n"
        "  </div>\n"
        "</footer>\n"
    );

    page_end();
    return 0;
}
