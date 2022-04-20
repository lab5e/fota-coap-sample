#pragma once

#include <stdint.h>
#include <stdio.h>

#include <coap2/coap.h>

void new_token(coap_pdu_t *pdu);
uint32_t uint_opt_value(const uint8_t *data, const size_t len);

/**
 * Asign path option(s) to an option list. When specifying paths as constants
 * (or command line parameters) they are expressed as HTTP-like paths
 * (/foo/bar/baz) but CoAP handles this as three separate path options so it's
 * quite clunky to split and set the path options properly.
 */
void set_path_options(const char *path, coap_optlist_t **optlist);