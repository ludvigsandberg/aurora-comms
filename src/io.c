#include <ca/io.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include <ca/meta.h>
#include <ca/net.h>
#include <ca/string.h>
#include <sys/types.h>

static bool ca_has_complete_line(const ca_string_t in, size_t *len) {
    for (*len = 0; *len < ca_alen(in); *len += 1) {
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
static void ca_trim_whitespace(ca_string_t *str) {
    if (ca_alen(*str) == 0) {
        return;
    }

    /* Remove leading whitespace. */

    size_t start;
    for (start = 0; start < ca_alen(*str) && (*str)[start] == ' '; start++)
        ;
    if (start == ca_alen(*str)) {
        ca_alen(*str) = 0;
        return;
    } else {
        ca_arr_remove_n(*str, 0, start);
    }

    /* Remove trailing whitespace. */

    size_t end = ca_alen(*str);
    while (end > 0 && (*str)[end - 1] == ' ') {
        end--;
    }

    ca_alen(*str) = end;
}

bool ca_get_line(ca_string_t *line, ca_string_t *in) {
    /* Find line length. */

    size_t len;

    if (!ca_has_complete_line(*in, &len)) {
        return false;
    }

    /* Append printable characters to line buffer. */
    for (size_t i = 0; i < len; i++) {
        if ((*in)[i] >= 0x20 && (*in)[i] <= 0x7e) {
            ca_arr_append(*line, (*in)[i]);
        }
    }

    /* Consume line and newline characters. */

    for (; len < ca_alen(*in) && ((*in)[len] == '\r' || (*in)[len] == '\n');
         len += 1)
        ;

    ca_arr_remove_n((*in), 0, len);

    ca_trim_whitespace(line);

    return true;
}

void ca_prompt(const ca_user_t *user, ca_app_t *app) {
    ca_bytes_t msg;
    ca_arr_from_string_literal(msg, ">", 1, 0);
    ca_server_send(&app->server, user->handle, msg);
}

void ca_print_fmt(const ca_user_t *user, ca_app_t *app, ca_print_type_t action,
                  const char *fmt, ...) {
    ca_bytes_t nl;
    ca_arr_from_string_literal(nl, "\r\n", 2, 0);

    if (action == CA_PRINT_INTERRUPT) {
        ca_server_send(&app->server, user->handle, nl);
    }

    ca_bytes_t out;
    ca_arr_new_reserve(out, 1024);

    va_list args;
    va_start(args, fmt);
    ca_alen(out) = (size_t)vsnprintf((char *)out, ca_acap(out), fmt, args);
    va_end(args);

    ca_server_send(&app->server, user->handle, out);
    ca_arr_free(out);

    ca_server_send(&app->server, user->handle, nl);
    ca_prompt(user, app);
}

bool ca_validate_username(const ca_string_t username) {
    if (ca_alen(username) < 2 || ca_alen(username) > 16) {
        return false;
    }

    for (size_t i = 0; i < ca_alen(username); i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }

    return true;
}
