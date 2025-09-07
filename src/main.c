#include <stdlib.h>
#include <assert.h>

#include <ac/app.h>
#include <ac/net.h>
#include <ac/log.h>

#define AC_DEFAULT_PORT 2000

int main(int argc, char *argv[]) {
    int port = argc < 2 ? AC_DEFAULT_PORT : atoi(argv[1]);

    ac_app_t app;
    ac_app_new(&app);
    ac_server_new(&app.server);
    ac_server_listen(&app.server, port);

    ac_log_fmt(AC_LOG_INFO, "Server listening on port %d.", port);

    while (true) {
        ac_server_poll(&app.server);
        ac_app_update(&app);
    }
}
