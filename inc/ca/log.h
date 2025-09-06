#ifndef CA_LOG_H
#define CA_LOG_H

#define CA_LOG_INFO    "info"
#define CA_LOG_WARNING "\x1b[38;2;255;204;0mwarn\x1b[0m"
#define CA_LOG_ERROR   "\x1b[48;2;204;51;0merror\x1b[0m"
#define CA_LOG_DEBUG   "\x1b[38;2;0;255;255mdebug\x1b[0m"

/** @brief Log a message at a specific log level.
 *
 * @param level The log level (e.g., info, warn, error).
 * @param msg The message to log.
 */
void ca_log(const char *level, const char *msg);

/** @brief Log a formatted message at a specific log level.
 *
 * @param level The log level (e.g., info, warn, error).
 * @param fmt The format string.
 * @param ... The values to format.
 */
void ca_log_fmt(const char *level, const char *fmt, ...);

#endif
