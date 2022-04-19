#include "coap_util.h"

void new_token(coap_pdu_t *pdu) {
  // Add a new token
  uint8_t token[8];
  for (int i = 0; i < sizeof(token); i++) {
    token[i] = (uint8_t)(rand() % 255);
  }
  coap_add_token(pdu, sizeof(token), token);
}

uint32_t uint_opt_value(const uint8_t *data, const size_t len) {
  uint16_t tmpval16 = 0;
  uint32_t tmpval32 = 0;
  uint32_t calc_len = 0;
  switch (len) {
  case 1:
    calc_len = (uint32_t)data[0];
    break;
  case 2:
    memcpy(&tmpval16, data, 2);
    calc_len = (uint32_t)ntohs(tmpval16);
    break;
  case 3:
  case 4:
    memcpy(&tmpval32, data, 3);
    calc_len = ntohl(tmpval32);
    break;
  default:
    calc_len = 0;
    break;
  }
  return calc_len;
}