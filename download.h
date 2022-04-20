#pragma once

#include <stdbool.h>

#include <coap2/coap.h>

#include "coap.h"

typedef bool (*download_cb_t)(int block_num, uint8_t *buf, size_t block_size,
                              uint32_t max_size);

/**
 * Download the firmware via blockwise transfer.
 */
bool coap_download_firmware(const char *hostname, const int port,
                            const char *path, download_cb_t callback,
                            const char *cert_file, const char *key_file);
