#pragma once

#include <stdint.h>
#include <stdio.h>

#include <coap2/coap.h>

void new_token(coap_pdu_t *pdu);
uint32_t uint_opt_value(const uint8_t *data, const size_t len);