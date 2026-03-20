# =========================================================
# Makefile - Compile all C CGI scripts
# Usage: make          (build everything)
#        make clean    (remove all binaries)
#        make install  (copy to Apache CGI directory)
# =========================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -I/usr/include/cgic -Icommon
LDFLAGS  = -lcgic -lcurl
CGI_DIR  = /usr/lib/cgi-bin

# All common object files (shared helpers)
COMMON_OBJS = common/session.o common/api_client.o common/html_utils.o

# All CGI binaries to build
CGI_BINS = \
	home.cgi \
	login.cgi \
	activate.cgi \
	forgot.cgi \
	dashboard.cgi \
	transfer.cgi \
	transactions.cgi \
	cards.cgi \
	loans.cgi \
	services.cgi \
	calculator.cgi \
	profile.cgi \
	logout.cgi

.PHONY: all clean install

all: $(CGI_BINS)

# Build common object files
common/session.o: common/session.c common/session.h
	$(CC) $(CFLAGS) -c $< -o $@

common/api_client.o: common/api_client.c common/api_client.h
	$(CC) $(CFLAGS) -c $< -o $@

common/html_utils.o: common/html_utils.c common/html_utils.h
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern: each .cgi is built from its .c + the common objects
%.cgi: %.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) $< $(COMMON_OBJS) $(LDFLAGS) -o $@

# Copy all .cgi binaries to Apache's CGI directory
install: all
	install -d $(CGI_DIR)
	install -m 755 $(CGI_BINS) $(CGI_DIR)/

clean:
	rm -f $(CGI_BINS) $(COMMON_OBJS)
