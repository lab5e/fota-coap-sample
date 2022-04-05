#include <coap2/coap.h>
#include <stdio.h>

int event_handler(coap_context_t *ctx, coap_event_t event,
                  coap_session_t *session) {
  switch (event) {
  case COAP_EVENT_DTLS_CLOSED:
    printf("Event: DTLS closed\n");
    // This exits the program but you might want to restart the connection
    // process somehow.
    exit(1);
    break;
  case COAP_EVENT_DTLS_CONNECTED:
    printf("Event: DTLS connected\n");
    break;
  case COAP_EVENT_DTLS_RENEGOTIATE:
    printf("Event: DTLS renegotiate\n");
    break;
  case COAP_EVENT_DTLS_ERROR:
    printf("Event: DTLS error\n");
    break;
  case COAP_EVENT_SESSION_CONNECTED:
    printf("Event: Session connected\n");
    break;
  case COAP_EVENT_SESSION_CLOSED:
    printf("Event: Session closed\n");
    break;
  case COAP_EVENT_SESSION_FAILED:
    printf("Event: Session failed\n");
    break;
  default:
    printf("Unhandled CoAP event: %04x\n", event);
    break;
  }
  return 0;
}

void nack_handler(coap_context_t *context, coap_session_t *session,
                  coap_pdu_t *sent, coap_nack_reason_t reason,
                  const coap_tid_t id) {
  switch (reason) {
  case COAP_NACK_TOO_MANY_RETRIES:
    printf("CoAP NACK handler: reason: too many retries id=%d\n", id);
    break;
  case COAP_NACK_NOT_DELIVERABLE:
    printf("CoAP NACK handler: reason: not deliverable id=%d\n", id);
    break;
  case COAP_NACK_RST:
    printf("CoAP NACK handler: reason: RST id=%d\n", id);
    break;
  case COAP_NACK_TLS_FAILED:
    printf("CoAP NACK handler: reason: TLS failed id=%d\n", id);
    break;
  default:
    printf("CoAP NACK handler: reason=%d id=%d\n", reason, id);
    break;
  }

  // This terminates the program but you should obviously handle this a bit more
  // graceful in a real production setting.
  exit(1);
}
