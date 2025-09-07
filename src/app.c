#include <ac/app.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <ac/net.h>
#include <ac/io.h>
#include <ac/meta.h>

void ac_user_new(ac_user_t *user, ac_app_t *app, ac_client_handle_t handle) {
    user->handle = handle;

    ac_arr_new(user->username);

    user->state = AC_STATE_LOGIN;
    ac_state_new(user, app);
}

void ac_user_free(ac_user_t *user) {
    ac_arr_free(user->username);
}

void ac_user_update(ac_user_t *user, ac_app_t *app, ac_bytes_t *in) {
    ac_state_update(user, app, in);
}

void ac_app_new(ac_app_t *app) {
    ac_map_new_reserve(app->users.from_handle, AC_CLIENTS_MAX);
    ac_map_new_reserve(app->users.from_username, AC_CLIENTS_MAX);

    app->app_start_time = time(NULL);
}

void ac_app_free(ac_app_t *app) {
    ac_map_free(app->users.from_handle);
    ac_map_free(app->users.from_username);
}

void ac_app_update(ac_app_t *app) {
    /* Update users. */

    ac_client_handle_t *handle;
    ac_user_t **user;

    ac_map_foreach(app->users.from_handle, handle, user) {
        ac_client_t *client;
        ac_map_get(app->server.clients, ac_handle_hash, ac_handle_eq,
                   (*user)->handle, client);

        ac_user_update(*user, app, &client->in);
    }

    ac_client_t *client;

    ac_map_foreach(app->server.clients, handle, client) {
        if (client->state == AC_CLIENT_STATE_NEW) {
            /* Create user. */

            ac_user_t *new_user = malloc(sizeof(ac_user_t));
            assert(new_user);
            ac_user_new(new_user, app, client->conn.handle);

            ac_map_set(app->users.from_handle, ac_handle_hash, ac_handle_eq,
                       new_user->handle, new_user);
        } else if (client->state == AC_CLIENT_STATE_TO_BE_REMOVED) {
            /* Remove user from maps. */

            ac_map_get(app->users.from_handle, ac_handle_hash, ac_handle_eq,
                       client->conn.handle, user);
            ac_map_remove(app->users.from_handle, ac_handle_hash, ac_handle_eq,
                          (*user)->handle);

            bool username_exists;
            ac_map_contains(app->users.from_username, ac_string_hash,
                            ac_string_eq, (*user)->username, username_exists);

            if (username_exists) {
                ac_map_remove(app->users.from_username, ac_string_hash,
                              ac_string_eq, (*user)->username);
            }

            /* Notify other users that a user has left the chat. */

            ac_client_handle_t *other_handle;
            ac_user_t **other_user;

            ac_map_foreach(app->users.from_handle, other_handle,
                            other_user) {
                if ((*other_user)->state == AC_STATE_CHAT) {
                    ac_print_fmt(*other_user, app, AC_PRINT_INTERRUPT,
                                    "%.*s has left the chat.",
                                    ac_alen((*user)->username),
                                    (*user)->username);
                }
            }

            /* Free the user object. */
            ac_user_free(*user);
            free(*user);
        }
    }
}
