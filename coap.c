#include <coap2/coap.h>
#include <stdio.h>

#include "coap.h"
#include "handlers.h"
#include "resolve.h"

#define LOG_LEVEL LOG_NOTICE
#define KEEPALIVE_SECONDS 10
#define BLOCK_MODE (COAP_BLOCK_1)

#define SERVER_ADDR "data.lab5e.com"
#define SERVER_PORT 5684

typedef struct {
  coap_address_t server;
  coap_address_t local;
  coap_context_t *ctx;
  coap_dtls_pki_t dtls;
  coap_session_t *session;
  coap_pdu_t *report;
  coap_optlist_t *optlist;
  t_upgrade_cb upgrade_handler;
} t_coap_state;

static t_coap_state state;

// This is the message handler that will process responses from the server.
static void message_handler(coap_context_t *ctx, coap_session_t *session,
                            coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id);

bool coap_init(const char *cert_file, const char *key_file) {
  memset(&state, 0, sizeof(state));

  // Initialize the CoAP library
  coap_startup();
  coap_dtls_set_log_level(LOG_LEVEL);
  coap_set_log_level(LOG_LEVEL);

  // Resolve server's address

  coap_address_init(&state.server);
  if (!resolve_address(SERVER_ADDR, &state.server.addr.sa)) {
    printf("Error resolving server address %s\n", SERVER_ADDR);
    return 2;
  }
  state.server.addr.sin.sin_port = htons(SERVER_PORT);

  // Resolve local interface address

  coap_address_init(&state.local);
  if (!resolve_address("0.0.0.0", &state.local.addr.sa)) {
    printf("Error resolving loopback address\n");
    return false;
  }

  // Create a context for the session we'll run
  state.ctx = coap_new_context(NULL);
  if (!state.ctx) {
    printf("Could not create CoAP context\n");
    return false;
  }
  coap_context_set_keepalive(state.ctx, KEEPALIVE_SECONDS);

  // Create the DTLS session
  memset(&state.dtls, 0, sizeof(state.dtls));
  state.dtls.version = COAP_DTLS_PKI_SETUP_VERSION;

  // Note the depth of validation; this is the max number of intermediate and
  // root certificates it will validate before giving up. If you set this to 1
  // the server certificate validation will fail since there is at least
  // one intermediate certificate between the root certificate and the server
  // certificate.

  state.dtls.verify_peer_cert = 1;        // Verify peer certificate
  state.dtls.require_peer_cert = 1;       // Require a server certificate
  state.dtls.allow_self_signed = 1;       // Allow self signed certificate
  state.dtls.allow_expired_certs = 0;     // No expired certificates
  state.dtls.cert_chain_validation = 1;   // Validate the chain
  state.dtls.check_cert_revocation = 0;   // Check the revocation list
  state.dtls.cert_chain_verify_depth = 2; // Depth of validation.

  state.dtls.validate_cn_call_back = NULL;  // CN callback (not used)
  state.dtls.cn_call_back_arg = NULL;       // CN callback
  state.dtls.validate_sni_call_back = NULL; // SNI callback
  state.dtls.sni_call_back_arg = NULL;      // SNI callback

  // Set up public key and certificates. Libcoap reads this directly from the
  // file in this version of the library
  state.dtls.pki_key.key_type = COAP_PKI_KEY_PEM;
  state.dtls.pki_key.key.pem.public_cert = cert_file;
  state.dtls.pki_key.key.pem.private_key = key_file;
  state.dtls.pki_key.key.pem.ca_file = cert_file;

  state.session = coap_new_client_session_pki(
      state.ctx, &state.local, &state.server, COAP_PROTO_DTLS, &state.dtls);

  if (!state.session) {
    printf("Could not create CoAP session object\n");
    return false;
  }

  // Register a message handler to process responses from the server.
  coap_register_response_handler(state.ctx, message_handler);
  coap_register_nack_handler(state.ctx, nack_handler);
  coap_register_event_handler(state.ctx, event_handler);

  return true;
}

bool coap_send_report(t_fota_report *report) {
  // Create a new request (aka PDU) that we'll send
  state.report = coap_new_pdu(state.session);
  if (!state.report) {
    printf("Could not create CoAP request\n");
    return false;
  }

  state.report->type = COAP_MESSAGE_CON;
  state.report->tid = coap_new_message_id(state.session);
  state.report->code = COAP_REQUEST_POST;

  coap_insert_optlist(&state.optlist, coap_new_optlist(COAP_OPTION_URI_PATH, 1,
                                                       (const uint8_t *)"u"));

  coap_add_optlist_pdu(state.report, &state.optlist);

  uint8_t report_buf[512];
  size_t report_len = 0;

  if (!fota_encode_report(report, report_buf, &report_len)) {
    printf("Error enoding report\n");
    return false;
  }

  // Add the payload to the PDU
  coap_add_data(state.report, report_len, report_buf);

  // Send it
  coap_tid_t tid = coap_send(state.session, state.report);
  if (tid == COAP_INVALID_TID) {
    printf("*** Error sending request\n");
    return false;
  }

  return true;
}

void coap_set_upgrade_handler(t_upgrade_cb handler) {
  state.upgrade_handler = handler;
}

void coap_wait_for_exchange() {
  while (!coap_can_exit(state.ctx)) {
    coap_run_once(state.ctx, 1000);
  }
}

void coap_shutdown() {
  coap_delete_optlist(state.optlist);
  coap_session_release(state.session);
  coap_free_context(state.ctx);
  coap_cleanup();
}
/**
 * Message handler function for CoAP messages received from the server.
 */
static void message_handler(coap_context_t *ctx, coap_session_t *session,
                            coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id) {
  switch (COAP_RESPONSE_CLASS(received->code)) {

  case 4:
    // 4xx responses are sent when there's an error with the response, f.e. an
    // unknown resource (4.04), invalid parameters (4.00), access denied
    // (4.01/4.03) or bad options (4.02). In short - the request is invalid and
    // has not been processed.
    printf("Got 4.xx response from server. The request is invalid\n");
    break;

  case 5:
    // 5xx responses are error responses from the server where there's an error
    // on the server side.
    printf("Got 5.xx response from server. Request failed\n");
    break;

  case 2:
    // 2.xx response codes indicates success and the server have received the
    // message.
    // Check if there is data waiting
    if (received->tid == state.report->tid) {
    }
    size_t len = 0;
    uint8_t *data = NULL;
    if (coap_get_data(received, &len, &data) == 0) {
      printf("No data in response\n");
      break;
    }
    if (len > 0) {
      t_fota_response resp;
      memset(&resp, 0, sizeof(resp));

      if (!fota_decode_response(data, len, &resp)) {
        printf("Error decoding response\n");
        return;
      }
      if (!state.upgrade_handler) {
        printf("no response handler set\n");
        return;
      }
      state.upgrade_handler(&resp);
    }
    break;
  default:
    printf("Got response code %d from server. Don't know how to handle it\n",
           received->code);
    break;
  }
}
