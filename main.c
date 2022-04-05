#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "coap.h"
#include "reporting.h"

#define VERSION "1.0.0"

#define CERT_FILE "cert.crt"
#define KEY_FILE "key.pem"

void upgrade_handler(t_fota_response *resp);

int main(int argc, char **argv) {
  char *version = VERSION;
  printf("FOTA demo client, file name: %s\n", argv[0]);
  if (argc == 2) {
    version = argv[1];
  }

  t_fota_report report = {
      .manufacturer = "Lab5e Demo Corp",
      .model = "model 01",
      .serial = "0001",
      .version = version,
  };

  if (!coap_init(CERT_FILE, KEY_FILE)) {
    printf("Could not init CoAP library\n");
    exit(1);
  }

  // The response is a callback from the CoAP library and the upgrade handler
  // function is called when there's a new version available.
  coap_set_upgrade_handler(upgrade_handler);

  if (!coap_send_report(&report)) {
    printf("Error sending report to server\n");
    exit(3);
  }

  // Wait for the exchange to complete
  coap_wait_for_exchange();

  coap_shutdown();
  return 0;
}

void upgrade_handler(t_fota_response *resp) {
  if (!resp->has_new_version) {
    printf("No new version available\n");
    return;
  }

  printf("There's a new version available at %s:%d/%s\n", resp->hostname,
         resp->port, resp->path);

  // Download from the host/port combo. This is *usually* the same as the
  //  coap_download_image(resp->hostname, resp->port, resp->path);
}
