#include <coap2/coap.h>
#include <stdio.h>

#include "coap.h"
#include "download.h"
#include "handlers.h"
#include "resolve.h"

#define LOG_LEVEL LOG_NOTICE
#define KEEPALIVE_SECONDS 10
#define BLOCK_MODE (COAP_BLOCK_1)

#define SERVER_ADDR "data.lab5e.com"
#define SERVER_PORT 5684

static coap_pdu_t *report_request;
static upgrade_cb_t upgrade_handler;

// This is the message handler that will process responses from the server.
static void message_handler(coap_context_t *ctx, coap_session_t *session,
                            coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id);

bool coap_connect(coap_state_t *state, const char *server_addr, const int port,
                  const char *cert_file, const char *key_file) {
  // Resolve server's address
  coap_address_init(&state->server);
  if (!resolve_address(server_addr, &state->server.addr.sa)) {
    printf("Error resolving server address %s\n", server_addr);
    return false;
  }
  state->server.addr.sin.sin_port = htons(port);

  // Resolve local interface address

  coap_address_init(&state->local);
  if (!resolve_address("0.0.0.0", &state->local.addr.sa)) {
    printf("Error resolving loopback address\n");
    return false;
  }

  // Create a context for the session we'll run
  state->ctx = coap_new_context(NULL);
  if (!state->ctx) {
    printf("Could not create CoAP context\n");
    return false;
  }
  coap_context_set_keepalive(state->ctx, KEEPALIVE_SECONDS);

  // Create the DTLS session
  memset(&state->dtls, 0, sizeof(state->dtls));
  state->dtls.version = COAP_DTLS_PKI_SETUP_VERSION;

  // Note the depth of validation; this is the max number of intermediate and
  // root certificates it will validate before giving up. If you set this to 1
  // the server certificate validation will fail since there is at least
  // one intermediate certificate between the root certificate and the server
  // certificate.

  state->dtls.verify_peer_cert = 1;        // Verify peer certificate
  state->dtls.require_peer_cert = 1;       // Require a server certificate
  state->dtls.allow_self_signed = 1;       // Allow self signed certificate
  state->dtls.allow_expired_certs = 0;     // No expired certificates
  state->dtls.cert_chain_validation = 1;   // Validate the chain
  state->dtls.check_cert_revocation = 0;   // Check the revocation list
  state->dtls.cert_chain_verify_depth = 2; // Depth of validation.

  state->dtls.validate_cn_call_back = NULL;  // CN callback (not used)
  state->dtls.cn_call_back_arg = NULL;       // CN callback
  state->dtls.validate_sni_call_back = NULL; // SNI callback
  state->dtls.sni_call_back_arg = NULL;      // SNI callback

  // Set up public key and certificates. Libcoap reads this directly from the
  // file in this version of the library
  state->dtls.pki_key.key_type = COAP_PKI_KEY_PEM;
  state->dtls.pki_key.key.pem.public_cert = cert_file;
  state->dtls.pki_key.key.pem.private_key = key_file;
  state->dtls.pki_key.key.pem.ca_file = cert_file;

  state->session = coap_new_client_session_pki(
      state->ctx, &state->local, &state->server, COAP_PROTO_DTLS, &state->dtls);

  if (!state->session) {
    printf("Could not create CoAP session object\n");
    return false;
  }

  return true;
}

bool coap_init(coap_state_t *state, const char *cert_file,
               const char *key_file) {
  memset(state, 0, sizeof(*state));

  // Initialize the CoAP library
  coap_startup();
  coap_dtls_set_log_level(LOG_LEVEL);
  coap_set_log_level(LOG_LEVEL);

  if (!coap_connect(state, SERVER_ADDR, SERVER_PORT, cert_file, key_file)) {
    return false;
  }
  // Register a message handler to process responses from the server.
  coap_register_response_handler(state->ctx, message_handler);
  coap_register_nack_handler(state->ctx, nack_handler);
  coap_register_event_handler(state->ctx, event_handler);
  return true;
}

bool coap_send_report(coap_state_t *state, fota_report_t *report) {
  // Create a new request (aka PDU) that we'll send
  report_request = coap_new_pdu(state->session);
  if (!report_request) {
    printf("Could not create CoAP request\n");
    return false;
  }

  report_request->type = COAP_MESSAGE_CON;
  report_request->tid = coap_new_message_id(state->session);
  report_request->code = COAP_REQUEST_POST;

  coap_optlist_t *optlist = NULL;
  coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_URI_PATH, 1,
                                                 (const uint8_t *)"u"));

  coap_add_optlist_pdu(report_request, &optlist);

  coap_delete_optlist(optlist);

  uint8_t report_buf[512];
  size_t report_len = 0;

  if (!fota_encode_report(report, report_buf, &report_len)) {
    printf("Error enoding report\n");
    return false;
  }

  // Add the payload to the PDU
  coap_add_data(report_request, report_len, report_buf);

  // Send it
  coap_tid_t tid = coap_send(state->session, report_request);
  if (tid == COAP_INVALID_TID) {
    printf("*** Error sending request\n");
    return false;
  }

  return true;
}

void coap_set_upgrade_handler(upgrade_cb_t handler) {
  upgrade_handler = handler;
}

void coap_wait_for_exchange(coap_state_t *state) {
  while (!coap_can_exit(state->ctx)) {
    coap_run_once(state->ctx, 1000);
  }
}

void coap_shutdown(coap_state_t *state) {
  coap_session_release(state->session);
  coap_free_context(state->ctx);
  coap_cleanup();
}

// Handle FOTA response from server
static void handle_report_callback(coap_pdu_t *received) {

  size_t len = 0;
  uint8_t *data = NULL;
  if (coap_get_data(received, &len, &data) == 0) {
    // No data - ignore
    return;
  }
  if (len == 0) {
    // zero bytes
    return;
  }

  fota_response_t resp;
  memset(&resp, 0, sizeof(resp));

  if (!fota_decode_response(data, len, &resp)) {
    // Error decoding response
    return;
  }
  if (!upgrade_handler) {
    // no respnse handler set
    return;
  }
  upgrade_handler(&resp);
}

/**
 * Message handler function for CoAP messages received from the server.
 */
static void message_handler(coap_context_t *ctx, coap_session_t *session,
                            coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id) {

  switch (COAP_RESPONSE_CLASS(received->code)) {
  case 2:
    if (report_request && id == report_request->tid) {
      handle_report_callback(received);
    }

    break;
  default:
    // Any other code is an error
    printf("Got response code %d from server. Don't know how to handle it\n",
           received->code);
    break;
  }
}
