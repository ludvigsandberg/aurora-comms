#include "ca/log.h"
#include <ca/app.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/sysinfo.h>

#include <ca/io.h>
#include <ca/meta.h>
#include <ca/net.h>
#include <ca/string.h>

void ca_state_new(ca_user_t *user, ca_app_t *app) {
    switch (user->state) {
        case CA_STATE_LOGIN: {
            /* Calculate server uptime in days. */

            char uptime[16];
            snprintf(uptime, sizeof uptime, "%d days",
                     (int)((time(NULL) - app->app_start_time) / 86400));

            /* Format greeting and send to user. */

            char user_count_msg[64];
            if (app->users.from_handle.len == 1) {
                snprintf(user_count_msg, sizeof user_count_msg,
                         "There is currently 1 user online.");
            } else {
                snprintf(user_count_msg, sizeof user_count_msg,
                         "There are currently %d users online.",
                         (int)app->users.from_handle.len);
            }

            ca_print_fmt(user, app, CA_PRINT_AFTER_ENTER,
                         "Welcome to ChatApp!\r\n"
                         "%s\r\n"
                         "Server uptime: %s\r\n"
                         "\r\n"
                         "Enter a username between 2-16 characters long.\r\n"
                         "It may include letters, numbers and underscores.",
                         user_count_msg, uptime);

            break;
        }

        case CA_STATE_CHAT: {
            ca_print_fmt(user, app, CA_PRINT_AFTER_ENTER,
                         "You may now chat with "
                         "others, %.*s!",
                         ca_alen(user->username), user->username);

            /* Broadcast new user to all clients. */

            ca_client_handle_t *handle;
            ca_user_t **other_user;

            ca_map_foreach(app->users.from_handle, handle, other_user) {
                if (user->handle == *handle) {
                    continue;
                }

                if ((*other_user)->state == CA_STATE_CHAT) {
                    ca_print_fmt(*other_user, app, CA_PRINT_INTERRUPT,
                                 "%.*s joins the chat!",
                                 ca_alen(user->username), user->username);
                }
            }

            break;
        }

        case CA_STATE_EXIT: {
            ca_print_fmt(user, app, CA_PRINT_AFTER_ENTER,
                         "Are you sure you want to exit? (y/n)");
            break;
        }
    }
}

void ca_state_free(ca_user_t *user, ca_app_t *app) {
    (void)app;

    switch (user->state) {
        case CA_STATE_LOGIN:
            break;

        case CA_STATE_CHAT:
            break;

        case CA_STATE_EXIT:
            break;
    }
}

void ca_state_update(ca_user_t *user, ca_app_t *app, ca_bytes_t *in) {
    switch (user->state) {
        case CA_STATE_LOGIN: {
            ca_string_t line;
            ca_arr_new(line);

            if (ca_get_line(&line, (ca_string_t *)in)) {
                if (ca_alen(line) == 0) {
                    ca_prompt(user, app);
                } else {
                    /* Check for exit command. */
                    ca_string_t exit_cmd;
                    ca_arr_from_string_literal(exit_cmd, "/exit", 5, 0);
                    if (ca_string_eq_ignore_case(line, exit_cmd)) {
                        ca_state_switch(user, app, CA_STATE_EXIT);
                        goto cleanup_login;
                    }

                    /* Check if username is taken. */
                    bool username_taken;
                    ca_map_contains(app->users.from_username, ca_string_hash,
                                    ca_string_eq, line, username_taken);

                    if (username_taken) {
                        ca_print_fmt(
                            user, app, CA_PRINT_AFTER_ENTER,
                            "Username is taken. Please choose another one.");
                    } else if (!ca_validate_username(line)) {
                        ca_print_fmt(
                            user, app, CA_PRINT_AFTER_ENTER,
                            "Username must be between 2-16 characters long "
                            "and may only contain letters, numbers, and "
                            "underscores. Please try again.");
                    } else {
                        /* Username is valid and not taken. */
                        ca_arr_append_n(user->username, ca_alen(line), line);
                        ca_state_switch(user, app, CA_STATE_CHAT);
                    }
                }
            }

        cleanup_login:
            ca_arr_free(line);
            break;
        }

        case CA_STATE_CHAT: {
            ca_string_t line;
            ca_arr_new(line);

            if (ca_get_line(&line, (ca_string_t *)in)) {
                if (ca_alen(line) == 0) {
                    ca_prompt(user, app);
                    goto cleanup_chat;
                }

                /* Check for exit command. */
                ca_string_t exit_cmd;
                ca_arr_from_string_literal(exit_cmd, "/exit", 5, 0);
                if (ca_string_eq_ignore_case(line, exit_cmd)) {
                    ca_state_switch(user, app, CA_STATE_EXIT);
                    goto cleanup_chat;
                } else {
                    /* Broadcast message to all users. */

                    ca_client_handle_t *handle;
                    ca_user_t **other_user;

                    ca_map_foreach(app->users.from_handle, handle,
                                   other_user) {
                        if (user->handle == *handle) {
                            continue;
                        }

                        if ((*other_user)->state == CA_STATE_CHAT) {
                            ca_print_fmt(*other_user, app, CA_PRINT_INTERRUPT,
                                         "[%.*s]: %.*s",
                                         ca_alen(user->username),
                                         user->username, ca_alen(line), line);
                        }
                    }

                    ca_print_fmt(user, app, CA_PRINT_AFTER_ENTER,
                                 "[You]: %.*s", ca_alen(line), line);
                }
            }

        cleanup_chat:
            ca_arr_free(line);
            break;
        }

        case CA_STATE_EXIT: {
            ca_string_t line;
            ca_arr_new(line);

            if (ca_get_line(&line, (ca_string_t *)in)) {
                /* If yes, disconnect user. */
                ca_string_t yes_cmd;
                ca_arr_from_string_literal(yes_cmd, "y", 1, 0);
                if (ca_string_eq_ignore_case(line, yes_cmd)) {
                    /* Broadcast user exit to all clients. */

                    ca_client_handle_t *handle;
                    ca_user_t **other_user;

                    ca_map_foreach(app->users.from_handle, handle,
                                   other_user) {
                        if (user->handle == *handle) {
                            continue;
                        }

                        if ((*other_user)->state == CA_STATE_CHAT) {
                            ca_print_fmt(*other_user, app, CA_PRINT_INTERRUPT,
                                         "%.*s has left the chat.",
                                         ca_alen(user->username),
                                         user->username);
                        }
                    }

                    /* Remove user from app. */
                    ca_server_remove_client(&app->server, user->handle);

                    goto cleanup_exit;
                }
                /* If no, return to chat state. */
                ca_state_switch(user, app, CA_STATE_CHAT);
            }

        cleanup_exit:
            ca_arr_free(line);
            break;
        }
    }
}

void ca_state_switch(ca_user_t *user, ca_app_t *app, ca_state_t state) {
    ca_state_free(user, app);
    user->state = state;
    ca_state_new(user, app);
}
