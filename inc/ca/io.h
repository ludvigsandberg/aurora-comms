#ifndef CA_IO_H
#define CA_IO_H

#include <stdint.h>
#include <stdbool.h>

#include <ca/app.h>
#include <ca/meta.h>
#include <ca/string.h>

/**
 * @brief Consume and sanitize a line of input.
 *
 * @param line The line to store the sanitized input.
 * @param in The input string to sanitize.
 * @return true if a complete line was consumed, false otherwise.
 */
bool ca_get_line(ca_string_t *line, ca_string_t *in);

/**
 * @brief Prompt the user for input.
 *
 * @param user The user to prompt.
 * @param app The application context.
 */
void ca_prompt(const ca_user_t *user, ca_app_t *app);

typedef enum ca_print_type_e {
    /** @brief Print after user hits enter.
     * A leading newline will <b>not</b> be inserted.
     */
    CA_PRINT_AFTER_ENTER,
    /** @brief Print while user is prompted.
     * A leading newline <b>will</b> be inserted.
     */
    CA_PRINT_INTERRUPT
} ca_print_type_t;

/** @brief Print formatted output.
 *
 * @param user The user to print the output for.
 * @param app The application context.
 * @param action The print action type.
 * @param fmt The format string.
 * @param ... The values to format.
 */
void ca_print_fmt(const ca_user_t *user, ca_app_t *app, ca_print_type_t action,
                  const char *fmt, ...);

/** @brief Validate a username.
 *
 * @param username The username to validate.
 * @return true if the username is valid, false otherwise.
 */
bool ca_validate_username(const ca_string_t username);

#endif
