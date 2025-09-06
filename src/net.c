#include <ca/net.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <errno.h>

#include <ca/log.h>
#include <ca/meta.h>

static int ca_socket_set_blocking(ca_socket_t socket, bool blocking) {
    int flags = fcntl(socket, F_GETFL, 0);

    if (flags == -1) {
        return EXIT_FAILURE;
    }

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(socket, F_SETFL, flags) == -1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

uint64_t ca_handle_hash(const ca_client_handle_t *handle) {
    return (uint64_t)*handle;
}

bool ca_handle_eq(const ca_client_handle_t *a, const ca_client_handle_t *b) {
    return *a == *b;
}

void ca_server_new(ca_server_t *server) {
    ca_map_new_reserve(server->clients, CA_CLIENTS_MAX);
}

void ca_server_free(ca_server_t *server) {
    ca_map_free(server->clients);
}

void ca_server_listen(ca_server_t *server, int port) {
    /* Create socket. */

    server->listener = socket(AF_INET, SOCK_STREAM, 0);

    if (server->listener == -1) {
        ca_log_fmt(CA_LOG_ERROR,
                   "socket(): failed to create listener socket.");
        exit(EXIT_FAILURE);
    }

    /* Set socket to be non-blocking. */

    if (ca_socket_set_blocking(server->listener, false) == -1) {
        ca_log_fmt(CA_LOG_ERROR, "ca_socket_set_blocking(): failed to set "
                                 "listener socket to non-blocking.");
        exit(EXIT_FAILURE);
    }

    /* Allow socket to be reusable. Avoids address-in-use error. */
    int yes = 1;
    setsockopt(server->listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    /* Bind socket. */

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server->listener, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        ca_log_fmt(CA_LOG_ERROR, "bind(): failed to bind listener socket.");
        exit(EXIT_FAILURE);
    }

    /* Listen on socket. */

    if (listen(server->listener, CA_CLIENTS_MAX) == -1) {
        ca_log_fmt(CA_LOG_ERROR, "listen(): failed to listen.");
        exit(EXIT_FAILURE);
    }
}

static void ca_disconnect_client(ca_server_t *server, ca_client_t *client) {
    ca_log_fmt(CA_LOG_INFO, "Client disconnected (%s).", client->ip);

    close(client->conn.socket);

    ca_arr_free(client->in);
    ca_arr_free(client->out);

    ca_map_remove(server->clients, ca_handle_hash, ca_handle_eq,
                  client->conn.handle);
}

static void ca_handle_conn(ca_server_t *server) {
    /* Accept connection. */

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;

    ca_socket_t socket =
        accept(server->listener, (struct sockaddr *)&addr, &len);

    if (socket == -1) {
        return;
    }

    /* Set socket to non-blocking mode. */
    if (ca_socket_set_blocking(socket, false) != EXIT_SUCCESS) {
        ca_log_fmt(CA_LOG_WARNING, "ca_socket_set_blocking(): failed to set "
                                   "client socket to non-blocking. "
                                   "Disconnecting client.");
        close(socket);
        return;
    }

    /* If too many clients, reject socket with message. */
    if (server->clients.len == CA_CLIENTS_MAX) {
        const char response[] = "\r\nConnection refused. Server is full.\r\n";
        send(socket, response, strlen(response), 0);
        close(socket);
        return;
    }

    /* Create client and add to array. */

    ca_client_t client;
    client.conn.socket = socket;
    client.state       = CA_CLIENT_STATE_NEW;

    if (!inet_ntop(addr.ss_family, &(((struct sockaddr_in *)&addr)->sin_addr),
                   client.ip, INET_ADDRSTRLEN)) {
        close(socket);
        ca_log_fmt(CA_LOG_ERROR, "inet_ntop(): fail.");
        return;
    }

    ca_arr_new(client.in);
    ca_arr_new(client.out);

    ca_map_set(server->clients, ca_handle_hash, ca_handle_eq,
               client.conn.handle, client);

    ca_log_fmt(CA_LOG_INFO, "Client connected (%s).", client.ip);
}

void ca_server_poll(ca_server_t *server) {
    /* Array of file descriptors (sockets) to be polled.
       Set first element to listener socket and the rest to client sockets. */

    ca_arr(struct pollfd) polled_sockets;
    ca_arr_new_reserve(polled_sockets, 1 + server->clients.len);

    struct pollfd fd = {.fd = server->listener, .events = POLLIN};
    ca_arr_append(polled_sockets, fd);

    ca_client_handle_t *handle;
    ca_client_t *client;

    ca_map_foreach(server->clients, handle, client) {
        if (client->state == CA_CLIENT_STATE_NEW) {
            client->state = CA_CLIENT_STATE_ONLINE;
        }

        else if (client->state == CA_CLIENT_STATE_TO_BE_REMOVED) {
            ca_disconnect_client(server, client);
            continue;
        }

        /* Add client socket to array of sockets to be polled for events. */
        fd.fd = client->conn.socket;
        ca_arr_append(polled_sockets, fd);
    }

    /* poll(): while there are no connected clients, wait indefinitely (-1). */

    int timeout = server->clients.pop_bkts == 0 ? -1 : 0;

    switch (poll(polled_sockets, (nfds_t)ca_alen(polled_sockets), timeout)) {
        case -1:
            ca_log_fmt(CA_LOG_ERROR, "poll(): error.");
            exit(EXIT_FAILURE);

        /* Timeout. No events. */
        case 0:
            goto no_events;
    }

    /* Check listener socket for incoming connection. */
    if (polled_sockets[0].revents & POLLIN) {
        ca_handle_conn(server);
    }

    /* Check if client sockets have received data. */
    for (size_t i = 1; i < ca_alen(polled_sockets); i += 1) {
        struct pollfd polled_client_socket = polled_sockets[i];

        if (polled_client_socket.revents & POLLIN) {
            char buf[512];
            int len = (int)recv(polled_client_socket.fd, buf, sizeof buf, 0);

            ca_map_get(server->clients, ca_handle_hash, ca_handle_eq,
                       polled_client_socket.fd, client);
            assert(client);

            /* No data to read. */
            if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                ca_log_fmt(CA_LOG_WARNING,
                           "recv(): returned empty for client (%s).",
                           client->ip);
                continue;
            }

            /* Client disconnected gracefully (0) or error (-1). */
            if (len <= 0) {
                client->state = CA_CLIENT_STATE_TO_BE_REMOVED;
                continue;
            }

            ca_arr_append_n(client->in, (size_t)len, buf);
        }
    }

no_events:
    ca_arr_free(polled_sockets);

    /* Send outgoing data to clients. */
    ca_map_foreach(server->clients, handle, client) {
        if (ca_alen(client->out) > 0) {
            ssize_t num_bytes_sent = send(client->conn.socket, client->out,
                                          ca_alen(client->out), 0);

            if (num_bytes_sent != -1) {
                ca_arr_remove_n(client->out, 0, (size_t)num_bytes_sent);
            }
        }
    }
}

void ca_server_remove_client(ca_server_t *server, ca_client_handle_t handle) {
    ca_client_t *client;
    ca_map_get(server->clients, ca_handle_hash, ca_handle_eq, handle, client);

    char *goodbye = "\r\nGoodbye!\r\n";
    send(client->conn.socket, goodbye, strlen(goodbye), 0);

    client->state = CA_CLIENT_STATE_TO_BE_REMOVED;
}

void ca_server_send(ca_server_t *server, ca_client_handle_t handle,
                    const ca_bytes_t data) {
    ca_client_t *client;
    ca_map_get(server->clients, ca_handle_hash, ca_handle_eq, handle, client);

    /* Append message to out stream. */
    ca_arr_append_n(client->out, ca_alen(data), data);
}
