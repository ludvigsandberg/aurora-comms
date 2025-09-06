#include <ca/string.h>

#include <string.h>
#include <ctype.h>

uint64_t ca_string_hash(const ca_string_t *key) {
    uint64_t hash = 14695981039346656037ul;

    for (size_t i = 0; i < ca_alen(*key); i++) {
        hash ^= (uint64_t)(uint8_t)(*key)[i];
        hash *= 1099511628211ul;
    }
    return hash;
}

bool ca_string_eq(const ca_string_t *a, const ca_string_t *b) {
    return ca_alen(*a) == ca_alen(*b) && memcmp(*a, *b, ca_alen(*a)) == 0;
}

bool ca_string_eq_ignore_case(const ca_string_t a, const ca_string_t b) {
    if (ca_alen(a) != ca_alen(b)) {
        return false;
    }

    for (size_t i = 0; i < ca_alen(a); i += 1) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }

    return true;
}
