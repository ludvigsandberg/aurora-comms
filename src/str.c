#include <ac/str.h>

#include <string.h>
#include <ctype.h>

uint64_t ac_string_hash(const ac_string_t *key) {
    uint64_t hash = 14695981039346656037ul;

    for (size_t i = 0; i < ac_alen(*key); i++) {
        hash ^= (uint64_t)(uint8_t)(*key)[i];
        hash *= 1099511628211ul;
    }
    return hash;
}

bool ac_string_eq(const ac_string_t *a, const ac_string_t *b) {
    return ac_alen(*a) == ac_alen(*b) && memcmp(*a, *b, ac_alen(*a)) == 0;
}

bool ac_string_eq_ignore_case(const ac_string_t a, const ac_string_t b) {
    if (ac_alen(a) != ac_alen(b)) {
        return false;
    }

    for (size_t i = 0; i < ac_alen(a); i += 1) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }

    return true;
}
