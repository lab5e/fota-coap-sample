#include <stdio.h>
#include <stdlib.h>

#include "reporting.h"

#define VERSION "1.0.0"

int main(int argc, char **argv)
{
    char *version = VERSION;
    printf("FOTA demo client, file name: %s\n", argv[0]);
    if (argc == 2)
    {
        version = argv[1];
    }

    t_fota_report report = {
        .manufacturer = "Lab5e Demo Corp",
        .model = "model 01",
        .serial = "0001",
        .version = version,
    };

    uint8_t buf[512];
    size_t len = 0;

    if (!fota_encode_report(&report, buf, &len))
    {
        printf("Error enoding report\n");
        exit(1);
    }
    printf("Reporting version %s, report is %zu bytes long\n", report.version, len);
    return 0;
}
