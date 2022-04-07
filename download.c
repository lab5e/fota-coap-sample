#include <coap2/coap.h>
#include <stdio.h>

#include "download.h"
#include "handlers.h"

static coap_state_t state;
static coap_pdu_t *download;
static coap_optlist_t *optlist;
static download_cb_t download_handler;

static void message_handler(coap_context_t *ctx, coap_session_t *session,
                            coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id);

bool coap_download_firmware(const uint8_t *hostname, const int port,
                            const uint8_t *path, download_cb_t callback,
                            const char *cert_file, const char *key_file) {
  if (!coap_connect(&state, (const char *)hostname, port, cert_file,
                    key_file)) {
    printf("Error connecting to CoAP server\n");
    return false;
  }
  download_handler = callback;

  coap_register_response_handler(state.ctx, message_handler);
  coap_register_nack_handler(state.ctx, nack_handler);
  coap_register_event_handler(state.ctx, event_handler);

  // Create a new request (aka PDU) that we'll send
  download = coap_new_pdu(state.session);
  if (!download) {
    printf("Could not create CoAP request\n");
    return false;
  }
  download->type = COAP_MESSAGE_CON;
  download->tid = coap_new_message_id(state.session);
  download->code = COAP_REQUEST_GET;
  uint8_t *pathptr = (uint8_t *)path;
  if (pathptr[0] == '/') {
    pathptr++;
  }
  coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_URI_PATH,
                                                 strlen((const char *)pathptr),
                                                 (const uint8_t *)pathptr));

  coap_add_optlist_pdu(download, &optlist);

  // Send it
  coap_tid_t tid = coap_send(state.session, download);
  if (tid == COAP_INVALID_TID) {
    printf("*** Error sending request\n");
    return false;
  }

  printf("Request sent\n");
  coap_wait_for_exchange(&state);
  return true;
}

// Handle image download messages
void handle_download_message(coap_pdu_t *received) {
  printf("Handle download\n");
  coap_opt_iterator_t opt_iter;
  coap_opt_t *block_opt =
      coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);

  if (block_opt) {
    uint16_t block_type = opt_iter.type;
    unsigned int block_num = coap_opt_block_num(block_opt);
    printf("Block %d\n", block_num);

    if (download_handler) {
      size_t len = 0;
      uint8_t *data = NULL;
      if (coap_get_data(received, &len, &data) == 0) {
        // No data - ignore
        printf("No data in payload\n");
        return;
      }
      if (len == 0) {
        printf("Zero lengt buffer returned\n");
        return;
      }
      if (!download_handler(block_num, 1000, data, len)) {
        printf("Aborting download\n");
        return;
      }
    }

    if (COAP_OPT_BLOCK_MORE(block_opt)) {
      // There is another block after this - generate a new request for that
      // block
      coap_pdu_t *next = coap_new_pdu(state.session);
      next->type = download->type;
      next->tid = download->tid;
      next->code = download->code;
      //      coap_add_token(next, download->token_length, download->token);
      download = next;

      coap_add_optlist_pdu(download, &optlist);

      unsigned char buf[4];
      coap_add_option(download, block_type,
                      coap_encode_var_safe(buf, sizeof(buf),
                                           ((block_num + 1) << 4) |
                                               COAP_OPT_BLOCK_SZX(block_opt)),
                      buf);

      coap_tid_t tid = coap_send(state.session, download);
      if (tid == COAP_INVALID_TID) {
        printf("message_handler: error sending new request\n");
      }
      printf("Next block request sent\n");
    }
  }
}

/**
 * Message handler function for CoAP messages received from the server.
 */
static void message_handler(coap_context_t *ctx, coap_session_t *session,
                            coap_pdu_t *sent, coap_pdu_t *received,
                            const coap_tid_t id) {
  switch (COAP_RESPONSE_CLASS(received->code)) {
  case 2:
    handle_download_message(received);

    break;
  default:
    // Any other code is an error
    printf("Got response code %d from server. Don't know how to handle it\n",
           received->code);
    break;
  }
}