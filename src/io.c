#include "ac/app.h"
#include <ac/io.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include <ac/meta.h>
#include <ac/net.h>
#include <ac/str.h>
#include <ac/log.h>

static bool ac_has_complete_line(const ac_string_t in, size_t *len) {
    for (*len = 0; *len < ac_alen(in); *len += 1) {
        if (in[*len] == '\r' || in[*len] == '\n') {
            return true;
        }
    }
    return false;
}

/** @brief Trim leading and trailing whitespace from a string.
 *
 * @param str The string to trim.
 */
static void ac_trim_whitespace(ac_string_t *str) {
    if (ac_alen(*str) == 0) {
        return;
    }

    /* Remove leading whitespace. */

    size_t start;
    for (start = 0; start < ac_alen(*str) && (*str)[start] == ' '; start++)
        ;
    if (start == ac_alen(*str)) {
        ac_alen(*str) = 0;
        return;
    } else {
        ac_arr_remove_n(*str, 0, start);
    }

    /* Remove trailing whitespace. */

    size_t end = ac_alen(*str);
    while (end > 0 && (*str)[end - 1] == ' ') {
        end--;
    }

    ac_alen(*str) = end;
}

bool ac_get_line(ac_string_t *line, ac_string_t *in) {
    /* Find line length. */

    size_t len;

    if (!ac_has_complete_line(*in, &len)) {
        return false;
    }

    /* Append printable characters to line buffer. */
    for (size_t i = 0; i < len; i++) {
        if ((*in)[i] >= 0x20 && (*in)[i] <= 0x7e) {
            ac_arr_append(*line, (*in)[i]);
        }
    }

    /* Consume line and newline characters. */

    for (; len < ac_alen(*in) && ((*in)[len] == '\r' || (*in)[len] == '\n');
         len += 1)
        ;

    ac_arr_remove_n((*in), 0, len);

    ac_trim_whitespace(line);

    return true;
}

void ac_send_fmt(const ac_user_t *user, ac_app_t *app, const char *fmt, ...) {
    ac_bytes_t out;
    ac_arr_new_reserve(out, 1024);

    va_list args;
    va_start(args, fmt);
    ac_alen(out) = (size_t)vsnprintf((char *)out, ac_acap(out), fmt, args);
    va_end(args);

    ac_server_send(&app->server, user->handle, out);
    ac_arr_free(out);
}

void ac_send(const ac_user_t *user, ac_app_t *app, const char *msg) {
    ac_send_fmt(user, app, "%s", msg);
}

void ac_prompt(const ac_user_t *user, ac_app_t *app) {
    ac_bytes_t msg;
    ac_arr_from_string_literal(msg, ">", 1, 0);
    ac_server_send(&app->server, user->handle, msg);
}

void ac_print_fmt(const ac_user_t *user, ac_app_t *app, ac_print_type_t action,
                  const char *fmt, ...) {
    ac_bytes_t nl;
    ac_arr_from_string_literal(nl, "\r\n", 2, 0);

    if (action == AC_PRINT_INTERRUPT) {
        ac_server_send(&app->server, user->handle, nl);
    }

    ac_bytes_t out;
    ac_arr_new_reserve(out, 1024);

    va_list args;
    va_start(args, fmt);
    ac_alen(out) = (size_t)vsnprintf((char *)out, ac_acap(out), fmt, args);
    va_end(args);

    ac_server_send(&app->server, user->handle, out);
    ac_arr_free(out);

    ac_server_send(&app->server, user->handle, nl);
    ac_prompt(user, app);
}

void ac_print(const ac_user_t *user, ac_app_t *app, ac_print_type_t action,
              const char *msg) {
    ac_print_fmt(user, app, action, "%s", msg);
}

bool ac_validate_username(const ac_string_t username) {
    if (ac_alen(username) < 2 || ac_alen(username) > 16) {
        return false;
    }

    for (size_t i = 0; i < ac_alen(username); i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }

    return true;
}

bool ac_is_command(const ac_string_t line) {
    return ac_alen(line) > 0 && line[0] == AC_COMMAND_PREFIX;
}

typedef enum ac_cmd_e {
    AC_CMD_HELP,
    AC_CMD_EXIT,
    AC_CMD_INFO,
    AC_CMD_LIST,
    AC_CMD_WHISPER
} ac_cmd_t;

static ac_map(ac_string_t, ac_cmd_t) ac_commands;

static void ac_init_aliases(const char *aliases[], ac_cmd_t cmd) {
    for (size_t i = 0; aliases[i]; i++) {
        ac_string_t alias;
        ac_arr_new_reserve(alias, strlen(aliases[i]));
        ac_arr_append_n(alias, strlen(aliases[i]), aliases[i]);

        ac_map_set(ac_commands, ac_string_hash, ac_string_eq, alias, cmd);
    }
}

static void ac_init_commands(void) {
    ac_map_new(ac_commands);

#define AC_INIT_ALIASES(cmd, ...)                                             \
    ac_init_aliases((const char *[]){__VA_ARGS__, NULL}, cmd)

    AC_INIT_ALIASES(AC_CMD_HELP, "help", "h");
    AC_INIT_ALIASES(AC_CMD_EXIT, "exit", "e", "quit", "q");
    AC_INIT_ALIASES(AC_CMD_INFO, "info", "i");
    AC_INIT_ALIASES(AC_CMD_LIST, "list", "l");
    AC_INIT_ALIASES(AC_CMD_WHISPER, "whisper", "w", "msg", "m");

#undef AC_INIT_ALIASES
}

static void ac_handle_help_cmd(ac_user_t *user, ac_app_t *app) {
    ac_print(user, app, AC_PRINT_AFTER_ENTER,
             "Available commands:\n"
             " - help (h): Show this help message.\n"
             " - exit (e / quit / q): Exit AuroraComms.\n"
             " - info (i): Show server information.\n"
             " - list (l): List online users.\n"
             " - whisper (w / msg / m): Send a private message.");
}

void ac_handle_command(ac_user_t *user, ac_app_t *app,
                       const ac_string_t line) {
    assert(ac_is_command(line) &&
           "Don't forget to call ac_is_command() first.");

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        ac_init_commands();
    }

    /* Check if input consists only of the command prefix. */
    if (ac_alen(line) == 1) {
        ac_handle_help_cmd(user, app);
        return;
    }

    /* Extract command (first word after prefix). */

    size_t cmd_end;
    for (cmd_end = 1; cmd_end < ac_alen(line) && line[cmd_end] != ' ';
         cmd_end++)
        ;

    ac_string_t cmd_str;
    ac_arr_new_reserve(cmd_str, cmd_end - 1);
    ac_arr_append_n(cmd_str, cmd_end - 1, line + 1);

    /* Lookup command. */
    ac_cmd_t *cmd;
    ac_map_get_maybe_null(ac_commands, ac_string_hash, ac_string_eq, cmd_str,
                          cmd);

    /* If unknown command, show help. */
    if (!cmd) {
        ac_handle_help_cmd(user, app);
        return;
    }

    switch (*cmd) {
        case AC_CMD_HELP:
            ac_handle_help_cmd(user, app);
            break;

        case AC_CMD_EXIT:
            ac_state_switch(user, app, AC_STATE_EXIT);
            break;

        case AC_CMD_INFO: {
            /* Calculate server uptime in days. */
            char uptime[16];
            snprintf(uptime, sizeof uptime, "%d days",
                     (int)((time(NULL) - app->app_start_time) / 86400));

            ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                         "AuroraComms Server\n"
                         " - Uptime: %s\n"
                         " - Connected users: %d",
                         uptime, (int)app->users.from_handle.len);
            break;
        }

        case AC_CMD_LIST: {
            ac_send(user, app, "Online users:\r\n");

            ac_send_fmt(user, app, " - %.*s (You)\r\n",
                        ac_alen(user->username), user->username);

            ac_client_handle_t *handle;
            ac_user_t **other_user;

            ac_map_foreach(app->users.from_handle, handle, other_user) {
                if (user->handle == *handle) {
                    continue;
                }

                if (user->state != AC_STATE_CHAT) {
                    continue;
                }

                ac_send_fmt(user, app, " - %.*s\r\n",
                            ac_alen((*other_user)->username),
                            (*other_user)->username);
            }

            ac_prompt(user, app);

            break;
        }

        case AC_CMD_WHISPER: {
            /* Extract recipient username. */
            size_t recipient_start = cmd_end + 1;
            for (; recipient_start < ac_alen(line) &&
                   line[recipient_start] == ' ';
                 recipient_start++)
                ;

            size_t recipient_end = recipient_start;
            for (; recipient_end < ac_alen(line) && line[recipient_end] != ' ';
                 recipient_end++)
                ;

            ac_string_t recipient;
            ac_arr_new_reserve(recipient, recipient_end - recipient_start);
            ac_arr_append_n(recipient, recipient_end - recipient_start,
                            line + recipient_start);

            ac_user_t **other_user;
            ac_map_get_maybe_null(app->users.from_username, ac_string_hash,
                                  ac_string_eq, recipient, other_user);

            if (!other_user) {
                ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                             "User '%.*s' not found.", ac_alen(recipient),
                             recipient);
            }

            else if ((*other_user)->state != AC_STATE_CHAT) {
                ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                             "User '%.*s' is not in the chat.",
                             ac_alen(recipient), recipient);
            }

            else {
                /* Extract message. */
                size_t msg_start = recipient_end + 1;
                for (; msg_start < ac_alen(line) && line[msg_start] == ' ';
                     msg_start++)
                    ;

                if (msg_start >= ac_alen(line)) {
                    ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                                 "Usage: /whisper <username> <message>");
                    ac_arr_free(recipient);
                    break;
                }

                ac_string_t msg;
                ac_arr_new_reserve(msg, ac_alen(line) - msg_start);
                ac_arr_append_n(msg, ac_alen(line) - msg_start,
                                line + msg_start);

                /* Send message to recipient. */
                ac_print_fmt(*other_user, app, AC_PRINT_INTERRUPT,
                             "[%.*s -> You]: %.*s",
                             ac_alen(user->username), user->username,
                             ac_alen(msg), msg);

                /* Acknowledge sender. */
                ac_print_fmt(user, app, AC_PRINT_AFTER_ENTER,
                             "[You -> %.*s]: %.*s",
                             ac_alen((*other_user)->username),
                             (*other_user)->username, ac_alen(msg), msg);

                ac_arr_free(msg);
            }

            ac_arr_free(recipient);
            break;
        }
    }

    ac_arr_free(cmd_str);
}
