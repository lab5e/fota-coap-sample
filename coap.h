#pragma once
#include <coap2/coap.h>
#include <stdbool.h>

#include "reporting.h"

typedef struct {
  coap_address_t server;
  coap_address_t local;
  coap_context_t *ctx;
  coap_dtls_pki_t dtls;
  coap_session_t *session;
} coap_state_t;

/**
 * Callback for upgrade handler.
 */
typedef void (*upgrade_cb_t)(fota_response_t *resp);

/**
 * Initialise the CoAP library
 */
bool coap_init(coap_state_t *state, const char *cert_file, const char *keyfile);

bool coap_connect(coap_state_t *state, const char *server_addr, const int port,
                  const char *cert_file, const char *key_file);

void coap_shutdown(coap_state_t *state);

/**
 * Send a version report to the Span backend
 */
bool coap_send_report(coap_state_t *state, fota_report_t *report);

/**
 * Set handler callback for upgrades
 */
void coap_set_upgrade_handler(upgrade_cb_t handler);

/**
 * Wait until CoAP exchange is completed. This will return when all exchanges
 * are completed.
 */
void coap_wait_for_exchange(coap_state_t *state);
