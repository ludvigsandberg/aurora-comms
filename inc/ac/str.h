#ifndef AC_STRING_H
#define AC_STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ac/meta.h>

typedef ac_arr(char) ac_string_t;
typedef ac_map(ac_string_t, int) ac_string_to_int_map_t;
typedef ac_map(ac_string_t, ac_string_t) ac_string_to_string_map_t;

uint64_t ac_string_hash(const ac_string_t *key);
bool ac_string_eq(const ac_string_t *a, const ac_string_t *b);

/** @brief Compare two strings for equality, ignoring case.
 *
 * @param a The first string.
 * @param b The second string.
 * @return true if the strings are equal (case-insensitive), false otherwise.
 */
bool ac_string_eq_ignore_case(const ac_string_t a, const ac_string_t b);

#endif
