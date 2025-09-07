#include <ac/app.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/sysinfo.h>

#include <ac/io.h>
#include <ac/log.h>
#include <ac/meta.h>
#include <ac/net.h>
#include <ac/str.h>

void ac_state_new(ac_user_t *user, ac_app_t *app) {
    switch (user->state) {
        case AC_STATE_LOGIN: {
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

            ac_print_fmt(
                user, app, AC_PRINT_AFTER_ENTER,
                "[ AuroraComms - Multi-User Communications Server ]\r\n"
                "%s\r\n"
                "Server uptime: %s\r\n"
                "\r\n"
                "Enter a username between 2-16 characters long.\r\n"
                "It may include letters, numbers and underscores.",
                user_count_msg, uptime);

            break;
        }

        case AC_STATE_CHAT: {
            ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                         "You may now chat with "
                         "others, %.*s!",
                         ac_alen(user->username), user->username);

            break;
        }

        case AC_STATE_EXIT: {
            ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                         "Are you sure you want to exit? (y/n)");
            break;
        }
    }
}

void ac_state_free(ac_user_t *user, ac_app_t *app) {
    (void)app;

    switch (user->state) {
        case AC_STATE_LOGIN:
            break;

        case AC_STATE_CHAT:
            break;

        case AC_STATE_EXIT:
            break;
    }
}

void ac_state_update(ac_user_t *user, ac_app_t *app, ac_bytes_t *in) {
    switch (user->state) {
        case AC_STATE_LOGIN: {
            ac_string_t line;
            ac_arr_new(line);

            if (ac_get_line(&line, (ac_string_t *)in)) {
                if (ac_alen(line) == 0) {
                    ac_prompt(user, app);
                } else {
                    /* Check if username is taken. */
                    bool username_taken;
                    ac_map_contains(app->users.from_username, ac_string_hash,
                                    ac_string_eq, line, username_taken);

                    if (username_taken) {
                        ac_print_fmt(
                            user, app, AC_PRINT_AFTER_ENTER,
                            "Username is taken. Please choose another one.");
                    } else if (!ac_validate_username(line)) {
                        ac_print_fmt(
                            user, app, AC_PRINT_AFTER_ENTER,
                            "Username must be between 2-16 characters long "
                            "and may only contain letters, numbers, and "
                            "underscores. Please try again.");
                    } else {
                        /* Username is valid and not taken. */

                        ac_arr_append_n(user->username, ac_alen(line), line);

                        ac_map_set(app->users.from_username, ac_string_hash,
                                   ac_string_eq, user->username, user);

                        ac_state_switch(user, app, AC_STATE_CHAT);

                        /* Broadcast new user to all clients. */

                        ac_client_handle_t *handle;
                        ac_user_t **other_user;

                        ac_map_foreach(app->users.from_handle, handle,
                                       other_user) {
                            if (user->handle == *handle) {
                                continue;
                            }

                            if ((*other_user)->state == AC_STATE_CHAT) {
                                ac_print_fmt(
                                    *other_user, app, AC_PRINT_INTERRUPT,
                                    "%.*s joins the chat!",
                                    ac_alen(user->username), user->username);
                            }
                        }
                    }
                }
            }

            ac_arr_free(line);
            break;
        }

        case AC_STATE_CHAT: {
            ac_string_t line;
            ac_arr_new(line);

            if (ac_get_line(&line, (ac_string_t *)in)) {
                if (ac_alen(line) == 0) {
                    ac_prompt(user, app);
                    goto cleanup_chat;
                }

                if (ac_is_command(line)) {
                    ac_handle_command(user, app, line);
                } else {
                    /* Broadcast message to all users. */

                    ac_client_handle_t *handle;
                    ac_user_t **other_user;

                    ac_map_foreach(app->users.from_handle, handle,
                                   other_user) {
                        if (user->handle == *handle) {
                            continue;
                        }

                        if ((*other_user)->state == AC_STATE_CHAT) {
                            ac_print_fmt(*other_user, app, AC_PRINT_INTERRUPT,
                                         "[%.*s]: %.*s",
                                         ac_alen(user->username),
                                         user->username, ac_alen(line), line);
                        }
                    }

                    ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                                 "[You]: %.*s", ac_alen(line), line);
                }
            }

        cleanup_chat:
            ac_arr_free(line);
            break;
        }

        case AC_STATE_EXIT: {
            ac_string_t line;
            ac_arr_new(line);

            if (ac_get_line(&line, (ac_string_t *)in)) {
                /* If yes, disconnect user. */
                ac_string_t yes_cmd;
                ac_arr_from_string_literal(yes_cmd, "y", 1, 0);
                if (ac_string_eq_ignore_case(line, yes_cmd)) {
                    /* Broadcast user exit to all clients. */

                    ac_client_handle_t *handle;
                    ac_user_t **other_user;

                    ac_map_foreach(app->users.from_handle, handle,
                                   other_user) {
                        if (user->handle == *handle) {
                            continue;
                        }

                        if ((*other_user)->state == AC_STATE_CHAT) {
                            ac_print_fmt(*other_user, app, AC_PRINT_INTERRUPT,
                                         "%.*s has left the chat.",
                                         ac_alen(user->username),
                                         user->username);
                        }
                    }

                    /* Remove user from app. */
                    ac_server_remove_client(&app->server, user->handle);

                    goto cleanup_exit;
                }
                /* If no, return to chat state. */
                ac_state_switch(user, app, AC_STATE_CHAT);
            }

        cleanup_exit:
            ac_arr_free(line);
            break;
        }
    }
}

void ac_state_switch(ac_user_t *user, ac_app_t *app, ac_state_t state) {
    ac_state_free(user, app);
    user->state = state;
    ac_state_new(user, app);
}
