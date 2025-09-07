#include <ac/net.h>

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

#include <ac/log.h>
#include <ac/meta.h>

static int ac_socket_set_blocking(ac_socket_t socket, bool blocking) {
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

uint64_t ac_handle_hash(const ac_client_handle_t *handle) {
    return (uint64_t)*handle;
}

bool ac_handle_eq(const ac_client_handle_t *a, const ac_client_handle_t *b) {
    return *a == *b;
}

void ac_server_new(ac_server_t *server) {
    ac_map_new_reserve(server->clients, AC_CLIENTS_MAX);
}

void ac_server_free(ac_server_t *server) {
    ac_map_free(server->clients);
}

void ac_server_listen(ac_server_t *server, int port) {
    /* Create socket. */

    server->listener = socket(AF_INET, SOCK_STREAM, 0);

    if (server->listener == -1) {
        ac_log_fmt(AC_LOG_ERROR,
                   "socket(): failed to create listener socket.");
        exit(EXIT_FAILURE);
    }

    /* Set socket to be non-blocking. */

    if (ac_socket_set_blocking(server->listener, false) == -1) {
        ac_log_fmt(AC_LOG_ERROR, "ac_socket_set_blocking(): failed to set "
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
        ac_log_fmt(AC_LOG_ERROR, "bind(): failed to bind listener socket.");
        exit(EXIT_FAILURE);
    }

    /* Listen on socket. */

    if (listen(server->listener, AC_CLIENTS_MAX) == -1) {
        ac_log_fmt(AC_LOG_ERROR, "listen(): failed to listen.");
        exit(EXIT_FAILURE);
    }
}

static void ac_disconnect_client(ac_server_t *server, ac_client_t *client) {
    ac_log_fmt(AC_LOG_INFO, "Client disconnected (%s).", client->ip);

    close(client->conn.socket);

    ac_arr_free(client->in);
    ac_arr_free(client->out);

    ac_map_remove(server->clients, ac_handle_hash, ac_handle_eq,
                  client->conn.handle);
}

static void ac_handle_conn(ac_server_t *server) {
    /* Accept connection. */

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;

    ac_socket_t socket =
        accept(server->listener, (struct sockaddr *)&addr, &len);

    if (socket == -1) {
        return;
    }

    /* Set socket to non-blocking mode. */
    if (ac_socket_set_blocking(socket, false) != EXIT_SUCCESS) {
        ac_log_fmt(AC_LOG_WARNING, "ac_socket_set_blocking(): failed to set "
                                   "client socket to non-blocking. "
                                   "Disconnecting client.");
        close(socket);
        return;
    }

    /* If too many clients, reject socket with message. */
    if (server->clients.len == AC_CLIENTS_MAX) {
        const char response[] = "\r\nConnection refused. Server is full.\r\n";
        send(socket, response, strlen(response), 0);
        close(socket);
        return;
    }

    /* Create client and add to array. */

    ac_client_t client;
    client.conn.socket = socket;
    client.state       = AC_CLIENT_STATE_NEW;

    if (!inet_ntop(addr.ss_family, &(((struct sockaddr_in *)&addr)->sin_addr),
                   client.ip, INET_ADDRSTRLEN)) {
        close(socket);
        ac_log_fmt(AC_LOG_ERROR, "inet_ntop(): fail.");
        return;
    }

    ac_arr_new(client.in);
    ac_arr_new(client.out);

    ac_map_set(server->clients, ac_handle_hash, ac_handle_eq,
               client.conn.handle, client);

    ac_log_fmt(AC_LOG_INFO, "Client connected (%s).", client.ip);
}

void ac_server_poll(ac_server_t *server) {
    /* Array of file descriptors (sockets) to be polled.
       Set first element to listener socket and the rest to client sockets. */

    ac_arr(struct pollfd) polled_sockets;
    ac_arr_new_reserve(polled_sockets, 1 + server->clients.len);

    struct pollfd fd = {.fd = server->listener, .events = POLLIN};
    ac_arr_append(polled_sockets, fd);

    ac_client_handle_t *handle;
    ac_client_t *client;

    ac_map_foreach(server->clients, handle, client) {
        if (client->state == AC_CLIENT_STATE_NEW) {
            client->state = AC_CLIENT_STATE_ONLINE;
        }

        else if (client->state == AC_CLIENT_STATE_TO_BE_REMOVED) {
            ac_disconnect_client(server, client);
            continue;
        }

        /* Add client socket to array of sockets to be polled for events. */
        fd.fd = client->conn.socket;
        ac_arr_append(polled_sockets, fd);
    }

    /* poll(): while there are no connected clients, wait indefinitely (-1). */

    int timeout = server->clients.pop_bkts == 0 ? -1 : 0;

    switch (poll(polled_sockets, (nfds_t)ac_alen(polled_sockets), timeout)) {
        case -1:
            ac_log_fmt(AC_LOG_ERROR, "poll(): error.");
            exit(EXIT_FAILURE);

        /* Timeout. No events. */
        case 0:
            goto no_events;
    }

    /* Check listener socket for incoming connection. */
    if (polled_sockets[0].revents & POLLIN) {
        ac_handle_conn(server);
    }

    /* Check if client sockets have received data. */
    for (size_t i = 1; i < ac_alen(polled_sockets); i += 1) {
        struct pollfd polled_client_socket = polled_sockets[i];

        if (polled_client_socket.revents & POLLIN) {
            char buf[512];
            int len = (int)recv(polled_client_socket.fd, buf, sizeof buf, 0);

            ac_map_get(server->clients, ac_handle_hash, ac_handle_eq,
                       polled_client_socket.fd, client);
            assert(client);

            /* No data to read. */
            if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                ac_log_fmt(AC_LOG_WARNING,
                           "recv(): returned empty for client (%s).",
                           client->ip);
                continue;
            }

            /* Client disconnected gracefully (0) or error (-1). */
            if (len <= 0) {
                client->state = AC_CLIENT_STATE_TO_BE_REMOVED;
                continue;
            }

            ac_arr_append_n(client->in, (size_t)len, buf);
        }
    }

no_events:
    ac_arr_free(polled_sockets);

    /* Send outgoing data to clients. */
    ac_map_foreach(server->clients, handle, client) {
        if (ac_alen(client->out) > 0) {
            ssize_t num_bytes_sent = send(client->conn.socket, client->out,
                                          ac_alen(client->out), 0);

            if (num_bytes_sent != -1) {
                ac_arr_remove_n(client->out, 0, (size_t)num_bytes_sent);
            }
        }
    }
}

void ac_server_remove_client(ac_server_t *server, ac_client_handle_t handle) {
    ac_client_t *client;
    ac_map_get(server->clients, ac_handle_hash, ac_handle_eq, handle, client);

    char *goodbye = "\r\nGoodbye!\r\n";
    send(client->conn.socket, goodbye, strlen(goodbye), 0);

    client->state = AC_CLIENT_STATE_TO_BE_REMOVED;
}

void ac_server_send(ac_server_t *server, ac_client_handle_t handle,
                    const ac_bytes_t data) {
    ac_client_t *client;
    ac_map_get(server->clients, ac_handle_hash, ac_handle_eq, handle, client);

    /* Append message to out stream. */
    ac_arr_append_n(client->out, ac_alen(data), data);
}
