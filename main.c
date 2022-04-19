#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "coap.h"
#include "download.h"
#include "reporting.h"

#define VERSION "1.0.0"

#define CERT_FILE "cert.crt"
#define KEY_FILE "key.pem"

void upgrade_cb(fota_response_t *resp);

bool download_block_cb(int block_num, uint8_t *buf, size_t len,
                       uint32_t max_size);

int main(int argc, char **argv) {
  char *version = VERSION;
  printf("FOTA demo client, file name: %s\n", argv[0]);
  if (argc == 2) {
    version = argv[1];
  }

  fota_report_t report = {
      .manufacturer = (uint8_t *)"Lab5e Demo Corp",
      .model = (uint8_t *)"model 01",
      .serial = (uint8_t *)"0001",
      .version = (uint8_t *)version,
  };

  coap_state_t state;

  if (!coap_init(&state, CERT_FILE, KEY_FILE)) {
    printf("Could not init CoAP library\n");
    exit(1);
  }

  // The response is a callback from the CoAP library and the upgrade handler
  // function is called when there's a new version available.
  coap_set_upgrade_handler(upgrade_cb);

  if (!coap_send_report(&state, &report)) {
    printf("Error sending report to server\n");
    exit(3);
  }

  printf("Wait for report exchange\n");
  // Wait for the exchange to complete
  coap_wait_for_exchange(&state);

  printf("Shutdown\n");
  coap_shutdown(&state);
  printf("Close\n");
  return 0;
}

void upgrade_cb(fota_response_t *resp) {
  if (!resp->has_new_version) {
    printf("No new version available\n");
    return;
  }

  printf("There's a new version available at coap://%s:%d%s\n", resp->hostname,
         resp->port, resp->path);

  coap_download_firmware(resp->hostname, resp->port, resp->path,
                         download_block_cb, CERT_FILE, KEY_FILE);
  printf("Download started\n");
}

// Block counter for firmware dowload.
static int last_block = -1;
static size_t downloaded_bytes = 0;

// Callback for block download. This checks if the block num is in sequence and
// returns false if the download fails.
bool download_block_cb(int block_num, uint8_t *buf, size_t len,
                       uint32_t max_size) {
  if (block_num != (last_block + 1)) {
    printf("Downloaded block %d but expected block %d\n", block_num,
           (last_block + 1));
    return false;
  }
  downloaded_bytes += len;
  printf("Downloaded %zi of %d bytes (block %d with %zi bytes)\n",
         downloaded_bytes, max_size, block_num, len);
  last_block = block_num;
  return true;
}