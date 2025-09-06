#ifndef CA_NET_H
#define CA_NET_H

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include <ca/meta.h>

#define CA_CLIENTS_MAX 50

typedef int ca_socket_t;
typedef int ca_client_handle_t;

typedef enum ca_client_state_e {
    CA_CLIENT_STATE_NEW,
    CA_CLIENT_STATE_ONLINE,
    CA_CLIENT_STATE_TO_BE_REMOVED
} ca_client_state_t;

typedef struct ca_client_s {
    union {
        ca_client_handle_t handle;
        ca_socket_t socket;
    } conn;

    ca_client_state_t state;

    /* Received data. */
    ca_bytes_t in;

    /* Outgoing data. */
    ca_bytes_t out;

    char ip[INET_ADDRSTRLEN];
} ca_client_t;

typedef ca_map(ca_client_handle_t, ca_client_t) ca_handle_to_client_map_t;
uint64_t ca_handle_hash(const ca_client_handle_t *handle);
bool ca_handle_eq(const ca_client_handle_t *a, const ca_client_handle_t *b);

typedef struct ca_server_s {
    ca_socket_t listener;
    ca_handle_to_client_map_t clients;
} ca_server_t;

void ca_server_new(ca_server_t *server);
void ca_server_free(ca_server_t *server);
void ca_server_listen(ca_server_t *server, int port);
void ca_server_poll(ca_server_t *server);
void ca_server_remove_client(ca_server_t *server, ca_client_handle_t handle);
void ca_server_send(ca_server_t *server, ca_client_handle_t handle,
                    const ca_bytes_t data);

#endif
