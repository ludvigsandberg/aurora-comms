#include "ca/log.h"
#include "ca/meta.h"
#include <ca/app.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <ca/net.h>
#include <ca/io.h>

void ca_user_new(ca_user_t *user, ca_app_t *app, ca_client_handle_t handle) {
    user->handle = handle;

    ca_arr_new(user->username);

    user->state = CA_STATE_LOGIN;
    ca_state_new(user, app);
}

void ca_user_free(ca_user_t *user) {
    ca_arr_free(user->username);
}

void ca_user_update(ca_user_t *user, ca_app_t *app, ca_bytes_t *in) {
    ca_state_update(user, app, in);
}

void ca_app_new(ca_app_t *app) {
    ca_map_new_reserve(app->users.from_handle, CA_CLIENTS_MAX);
    ca_map_new_reserve(app->users.from_username, CA_CLIENTS_MAX);

    app->app_start_time = time(NULL);
}

void ca_app_free(ca_app_t *app) {
    ca_map_free(app->users.from_handle);
    ca_map_free(app->users.from_username);
}

void ca_app_update(ca_app_t *app) {
    ca_client_handle_t *handle;
    ca_client_t *client;

    /* Update users. */

    ca_user_t **user;

    ca_map_foreach(app->users.from_handle, handle, user) {
        ca_map_get(app->server.clients, ca_handle_hash, ca_handle_eq,
                   (*user)->handle, client);

        ca_user_update(*user, app, &client->in);
    }

    ca_map_foreach(app->server.clients, handle, client) {
        if (client->state == CA_CLIENT_STATE_NEW) {
            ca_user_t *new_user = malloc(sizeof(ca_user_t));
            assert(new_user);
            ca_user_new(new_user, app, client->conn.handle);

            ca_map_set(app->users.from_handle, ca_handle_hash, ca_handle_eq,
                       new_user->handle, new_user);
        } else if (client->state == CA_CLIENT_STATE_TO_BE_REMOVED) {
            ca_map_get(app->users.from_handle, ca_handle_hash, ca_handle_eq,
                       client->conn.handle, user);
            ca_map_remove(app->users.from_handle, ca_handle_hash, ca_handle_eq,
                          (*user)->handle);

            bool handle_exists;
            ca_map_contains(app->users.from_handle, ca_handle_hash,
                            ca_handle_eq, (*user)->handle, handle_exists);

            bool username_exists;
            ca_map_contains(app->users.from_username, ca_string_hash,
                            ca_string_eq, (*user)->username, username_exists);

            if (username_exists) {
                ca_map_remove(app->users.from_username, ca_string_hash,
                              ca_string_eq, (*user)->username);
            }

            ca_user_free(*user);
            free(*user);
        }
    }
}
