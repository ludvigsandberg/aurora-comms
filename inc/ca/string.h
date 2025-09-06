#ifndef CA_STRING_H
#define CA_STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ca/meta.h>

typedef ca_arr(char) ca_string_t;
typedef ca_map(ca_string_t, int) ca_string_to_int_map_t;
typedef ca_map(ca_string_t, ca_string_t) ca_string_to_string_map_t;

uint64_t ca_string_hash(const ca_string_t *key);
bool ca_string_eq(const ca_string_t *a, const ca_string_t *b);

/** @brief Compare two strings for equality, ignoring case.
 *
 * @param a The first string.
 * @param b The second string.
 * @return true if the strings are equal (case-insensitive), false otherwise.
 */
bool ca_string_eq_ignore_case(const ca_string_t a, const ca_string_t b);

#endif
