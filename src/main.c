#include <stdlib.h>
#include <assert.h>

#include <ca/app.h>
#include <ca/net.h>
#include <ca/log.h>

#define CA_DEFAULT_PORT 2000

int main(int argc, char *argv[]) {
    int port = argc < 2 ? CA_DEFAULT_PORT : atoi(argv[1]);

    ca_app_t app;
    ca_app_new(&app);
    ca_server_new(&app.server);
    ca_server_listen(&app.server, port);

    ca_log_fmt(CA_LOG_INFO, "Server listening on port %d.", port);

    while (true) {
        ca_server_poll(&app.server);
        ca_app_update(&app);
    }
}
