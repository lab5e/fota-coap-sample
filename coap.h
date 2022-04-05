#pragma once

#include <stdbool.h>

#include "reporting.h"

/**
 * Callback for upgrade handler.
 */
typedef void (*t_upgrade_cb)(t_fota_response *resp);

/**
 * Initialise the CoAP library
 */
bool coap_init(const char *cert_file, const char *keyfile);

void coap_shutdown();

/**
 * Send a version report to the Span backend
 */
bool coap_send_report(t_fota_report *report);

/**
 * Set handler callback for upgrades
 */
void coap_set_upgrade_handler(t_upgrade_cb handler);

/**
 * Wait until CoAP exchange is completed. This will return when all exchanges
 * are completed.
 */
void coap_wait_for_exchange();
