# =========================================================
# Dockerfile.cgi  -  Apache + cgic + libcurl
# Serves C CGI scripts and static CSS/assets
# =========================================================

FROM debian:bookworm-slim

# ── system packages ───────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
        apache2 \
        gcc \
        make \
        libcurl4-openssl-dev \
        libcurl4 \
        ca-certificates \
        wget \
    && rm -rf /var/lib/apt/lists/*

# ── build cgic library ────────────────────────────────────
WORKDIR /tmp/cgic
RUN wget -q https://www.boutell.com/cgic/cgic218.tar.gz \
    && tar -xzf cgic218.tar.gz \
    && cd cgic218 \
    && make \
    && cp libcgic.a /usr/local/lib/ \
    && cp cgic.h    /usr/local/include/ \
    && ldconfig

# ── enable Apache CGI module ──────────────────────────────
RUN a2enmod cgid headers rewrite

# ── Apache virtual host config ────────────────────────────
COPY docker/apache-nexusbank.conf /etc/apache2/sites-available/nexusbank.conf
RUN a2dissite 000-default && a2ensite nexusbank

# ── copy source and build CGI binaries ───────────────────
WORKDIR /build
COPY cgi-bin/ ./cgi-bin/
RUN cd cgi-bin && make && make install

# ── copy static assets ────────────────────────────────────
COPY frontend/ /var/www/nexusbank/

# ── permissions ───────────────────────────────────────────
RUN chown -R www-data:www-data /var/www/nexusbank \
    && chmod -R 755 /usr/lib/cgi-bin/nexusbank

EXPOSE 80

CMD ["apachectl", "-D", "FOREGROUND"]
