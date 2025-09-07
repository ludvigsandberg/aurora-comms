#ifndef AC_NET_H
#define AC_NET_H

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include <ac/meta.h>

#define AC_CLIENTS_MAX 50

typedef int ac_socket_t;
typedef int ac_client_handle_t;

typedef enum ac_client_state_e {
    AC_CLIENT_STATE_NEW,
    AC_CLIENT_STATE_ONLINE,
    AC_CLIENT_STATE_TO_BE_REMOVED
} ac_client_state_t;

typedef struct ac_client_s {
    union {
        ac_client_handle_t handle;
        ac_socket_t socket;
    } conn;

    ac_client_state_t state;

    /* Received data. */
    ac_bytes_t in;

    /* Outgoing data. */
    ac_bytes_t out;

    char ip[INET_ADDRSTRLEN];
} ac_client_t;

typedef ac_map(ac_client_handle_t, ac_client_t) ac_handle_to_client_map_t;
uint64_t ac_handle_hash(const ac_client_handle_t *handle);
bool ac_handle_eq(const ac_client_handle_t *a, const ac_client_handle_t *b);

typedef struct ac_server_s {
    ac_socket_t listener;
    ac_handle_to_client_map_t clients;
} ac_server_t;

void ac_server_new(ac_server_t *server);
void ac_server_free(ac_server_t *server);
void ac_server_listen(ac_server_t *server, int port);
void ac_server_poll(ac_server_t *server);
void ac_server_remove_client(ac_server_t *server, ac_client_handle_t handle);
void ac_server_send(ac_server_t *server, ac_client_handle_t handle,
                    const ac_bytes_t data);

#endif
