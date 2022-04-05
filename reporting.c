#include "reporting.h"
#include <stdbool.h>
#include <string.h>

#define FIRMWARE_VER_ID 1
#define MODEL_NUMBER_ID 2
#define SERIAL_NUMBER_ID 3
#define CLIENT_MANUFACTURER_ID 4

#define HOST_ID 1
#define PORT_ID 2
#define PATH_ID 3
#define AVAILABLE_ID 4

static size_t encode_tlv_string(uint8_t *buf, uint8_t id, const char *str);
static bool decode_tlv_string(const uint8_t *buf, size_t *idx, char *str);
static int decode_tlv_uint32(const uint8_t *buf, size_t *idx, uint32_t *val);
static bool decode_tlv_bool(const uint8_t *buf, size_t *idx, bool *val);

bool fota_encode_report(t_fota_report *report, uint8_t *buf, size_t *len) {
  size_t sz = encode_tlv_string(buf, FIRMWARE_VER_ID, report->version);
  sz +=
      encode_tlv_string(buf + sz, CLIENT_MANUFACTURER_ID, report->manufacturer);
  sz += encode_tlv_string(buf + sz, SERIAL_NUMBER_ID, report->serial);
  sz += encode_tlv_string(buf + sz, MODEL_NUMBER_ID, report->model);
  *len = sz;
  return true;
}

static size_t encode_tlv_string(uint8_t *buf, uint8_t id, const char *str) {
  size_t ret = 0;
  buf[ret++] = id;
  buf[ret++] = strlen(str);
  for (uint8_t i = 0; i < strlen(str); i++) {
    buf[ret++] = str[i];
  }
  return ret;
}

bool fota_decode_response(uint8_t *buf, size_t len, t_fota_response *resp) {
  size_t idx = 0;
  while (idx < len) {
    uint8_t id = buf[idx++];
    switch (id) {
    case HOST_ID:
      if (!decode_tlv_string(buf, &idx, resp->hostname)) {
        return false;
      }
      break;
    case PORT_ID:
      if (!decode_tlv_uint32(buf, &idx, &resp->port)) {
        return false;
      }
      break;
    case PATH_ID:
      if (!decode_tlv_string(buf, &idx, resp->path)) {
        return false;
      }
      break;
    case AVAILABLE_ID:
      if (!decode_tlv_bool(buf, &idx, &resp->has_new_version)) {
        return false;
      }
      break;
    default:
      // Unknown ID in response. Return with error
      return false;
    }
  }
  return true;
}

static bool decode_tlv_string(const uint8_t *buf, size_t *idx, char *str) {
  int len = (int)buf[(*idx)++];
  int i = 0;
  for (i = 0; i < len; i++) {
    str[i] = buf[(*idx)++];
  }
  str[i] = 0;
  return true;
}

static int decode_tlv_uint32(const uint8_t *buf, size_t *idx, uint32_t *val) {
  size_t len = (size_t)buf[(*idx)++];
  if (len != 4) {
    // uint32 should be 4 bytes
    return false;
  }
  *val = 0;
  *val += (buf[(*idx)++] << 24);
  *val += (buf[(*idx)++] << 16);
  *val += (buf[(*idx)++] << 8);
  *val += (buf[(*idx)++]);
  return true;
}

static bool decode_tlv_bool(const uint8_t *buf, size_t *idx, bool *val) {
  size_t len = (size_t)buf[(*idx)++];
  if (len != 1) {
    // Should be 1 byte long
    return false;
  }

  *val = (buf[(*idx)++] == 1);
  return true;
}
