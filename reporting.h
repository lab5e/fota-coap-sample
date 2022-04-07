#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * This is the FOTA report sent to the server;
 */
typedef struct {
  uint8_t *version;
  uint8_t *manufacturer;
  uint8_t *serial;
  uint8_t *model;
} fota_report_t;

/**
 * FOTA report response.
 */
typedef struct {
  bool has_new_version;
  uint8_t hostname[32];
  uint8_t path[10];
  uint32_t port;
} fota_response_t;

/**
 * Encode a report. This returns a negative value on error.
 */
bool fota_encode_report(fota_report_t *report, uint8_t *buf, size_t *len);

/**
 * Decode a response.
 */
bool fota_decode_response(uint8_t *buf, size_t len, fota_response_t *resp);
