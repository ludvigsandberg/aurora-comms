#ifndef AC_IO_H
#define AC_IO_H

#include <stdint.h>
#include <stdbool.h>

#include <ac/app.h>
#include <ac/meta.h>
#include <ac/str.h>

#define AC_COMMAND_PREFIX '/'

/**
 * @brief Consume and sanitize a line of input.
 *
 * @param line The line to store the sanitized input.
 * @param in The input string to sanitize.
 * @return true if a complete line was consumed, false otherwise.
 */
bool ac_get_line(ac_string_t *line, ac_string_t *in);

/**
 * @brief Prompt the user for input.
 *
 * @param user The user to prompt.
 * @param app The application context.
 */
void ac_prompt(const ac_user_t *user, ac_app_t *app);

typedef enum ac_print_type_e {
    /** @brief Print after user hits enter.
     * A leading newline will <b>not</b> be inserted.
     */
    AC_PRINT_AFTER_ENTER,
    /** @brief Print while user is prompted.
     * A leading newline <b>will</b> be inserted.
     */
    AC_PRINT_INTERRUPT
} ac_print_type_t;

/** 
 * @brief Send formatted output to a user without inserting a newline or a prompt.
 *
 * @param user The user to send the output to.
 * @param app The application context.
 * @param fmt The format string.
 * @param ... The values to format.
 */
void ac_send_fmt(const ac_user_t *user, ac_app_t *app, const char *fmt, ...);

/** 
 * @brief Send a message to a user without inserting a newline or a prompt.
 *
 * @param user The user to send the message to.
 * @param app The application context.
 * @param msg The message to send.
 */
void ac_send(const ac_user_t *user, ac_app_t *app, const char *msg);

/** 
 * @brief Print formatted output to a user.
 *
 * @param user The user to print the output for.
 * @param app The application context.
 * @param action The print action type.
 * @param fmt The format string.
 * @param ... The values to format.
 */
void ac_print_fmt(const ac_user_t *user, ac_app_t *app, ac_print_type_t action,
                  const char *fmt, ...);

/** 
 * @brief Print a message to a user.
 *
 * @param user The user to print the message for.
 * @param app The application context.
 * @param action The print action type.
 * @param msg The message to print.
 */
void ac_print(const ac_user_t *user, ac_app_t *app, ac_print_type_t action,
                  const char *msg);

/**
 * @brief Validate a username.
 *
 * @param username The username to validate.
 * @return true if the username is valid, false otherwise.
 */
bool ac_validate_username(const ac_string_t username);

/** 
 * @brief Check if a line is a command.
 *
 * @param line The line to check.
 * @return true if the line is a command, false otherwise.
 */
bool ac_is_command(const ac_string_t line);

/** 
 * @brief Handle a command from the user.
 *
 * @param user The user who sent the command.
 * @param app The application context.
 * @param line The command line to handle.
 */
void ac_handle_command(ac_user_t *user, ac_app_t *app, const ac_string_t line);

#endif
