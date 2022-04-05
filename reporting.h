#pragma once
#include <sys/types.h>
#include <stdbool.h>

/**
 * This is the FOTA report sent to the server;
 */
typedef struct
{
    char *version;
    char *manufacturer;
    char *serial;
    char *model;
} t_fota_report;

/**
 * FOTA report response.
 */
typedef struct
{
    bool has_new_version;
    char hostname[32];
    char path[10];
    uint16_t port;
} t_fota_response;

/**
 * Encode a report. This returns a negative value on error.
 */
bool fota_encode_report(t_fota_report *report, uint8_t *buf, size_t *len);

/**
 * Decode a response.
 */
int fota_decode_response(uint8_t *buf, size_t len, t_fota_response *resp);

/*
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>



size_t encode_tlv_string(uint8_t *buf, uint8_t id, const uint8_t *str);

uint8_t tlv_id(const uint8_t *buf, size_t idx);

int decode_tlv_string(const uint8_t *buf, size_t *idx, char *str);

int decode_tlv_uint32(const uint8_t *buf, size_t *idx, uint32_t *val);

int decode_tlv_bool(const uint8_t *buf, size_t *idx, bool *val);

/ -------



static int fota_encode_simple_report(uint8_t *buffer, size_t *len)
{
    size_t sz = encode_tlv_string(buffer, FIRMWARE_VER_ID, CLIENT_FIRMWARE_VER);
    sz += encode_tlv_string(buffer + sz, CLIENT_MANUFACTURER_ID, CLIENT_MANUFACTURER);
    sz += encode_tlv_string(buffer + sz, SERIAL_NUMBER_ID, CLIENT_SERIAL_NUMBER);
    sz += encode_tlv_string(buffer + sz, MODEL_NUMBER_ID, CLIENT_MODEL_NUMBER);
    *len = sz;
    return 0;
}

static int fota_decode_simple_response(simple_fota_response_t *resp, const uint8_t *buf, size_t len)
{
    size_t idx = 0;
    int err = 0;
    while (idx < len)
    {
        uint8_t id = buf[idx++];
        switch (id)
        {
        case HOST_ID:
            err = decode_tlv_string(buf, &idx, resp->host);
            if (err)
            {
                return err;
            }
            break;
        case PORT_ID:
            err = decode_tlv_uint32(buf, &idx, &resp->port);
            if (err)
            {
                return err;
            }
            break;
        case PATH_ID:
            err = decode_tlv_string(buf, &idx, resp->path);
            if (err)
            {
                return err;
            }
            break;
        case AVAILABLE_ID:
            err = decode_tlv_bool(buf, &idx, &resp->scheduled_update);
            if (err)
            {
                return err;
            }
            break;
        default:
            LOG_ERR("Unknown field id in FOTA response: %d", id);
            return -1;
        }
    }
    return 0;
}


*/
