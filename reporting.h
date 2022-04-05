#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * This is the FOTA report sent to the server;
 */
typedef struct {
  char *version;
  char *manufacturer;
  char *serial;
  char *model;
} t_fota_report;

/**
 * FOTA report response.
 */
typedef struct {
  bool has_new_version;
  char hostname[32];
  char path[10];
  uint32_t port;
} t_fota_response;

/**
 * Encode a report. This returns a negative value on error.
 */
bool fota_encode_report(t_fota_report *report, uint8_t *buf, size_t *len);

/**
 * Decode a response.
 */
bool fota_decode_response(uint8_t *buf, size_t len, t_fota_response *resp);
