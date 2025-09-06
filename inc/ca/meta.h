#ifndef CA_META_H
#define CA_META_H

/* -------------------------------------------------------------------------
   Macro metaprogramming.
   ------------------------------------------------------------------------- */

#define ca_concat2(A, B) A##B
#define ca_concat(A, B)  ca_concat2(A, B)

/** @brief Generate a unique name for internal macro identifiers. */
#define ca_uniq(NAME) ca_concat(NAME, __LINE__)

/** @brief Static assertion for compile-time checks. */
#ifdef __GNUC__
#define ca_static_assert(COND, MSG)                                           \
    __attribute__((unused)) typedef char ca_uniq(                             \
        ca_static_assert)[(COND) ? 1 : -1]
#else
#define ca_static_assert(COND, MSG)
#endif

/** @brief Assign pointer to pointer without type cast. */
#define ca_generic_assign(A, B)                                               \
    do {                                                                      \
        void *ca_uniq(m) = B;                                                 \
        memcpy(&A, &ca_uniq(m), sizeof(void *));                              \
    } while (0)

/** @brief Iterate over a range of values. */
#define ca_foreach(N, I) for (size_t I = 0; I < (N); I++)

/* -------------------------------------------------------------------------
   Generic type-safe dynamic array.
   Metadata about array length and capacity is stored at
   indices -2 and -1 and array elements begin like normal at index 0.
   ------------------------------------------------------------------------- */

#define ca_arr(T) T *

typedef ca_arr(unsigned char) ca_bytes_t;
typedef ca_arr(int) ca_ints_t;
typedef ca_arr(double) ca_doubles_t;

/** @brief Get the length of an array. */
#define ca_alen(A) ((size_t *)(A))[-2]
/** @brief Get the capacity of an array. */
#define ca_acap(A) ((size_t *)(A))[-1]

/** @brief Iterate over the elements of an array. */
#define ca_arr_foreach(A, I) for (size_t I = 0; I < ca_alen(A); I++)

/** @brief Create a new empty array. */
#define ca_arr_new(A)                                                         \
    ca_generic_assign((A), (size_t *)(calloc(2, sizeof(size_t))) + 2);

/** @brief Create a new array with a specific length. */
#define ca_arr_new_n(A, N)                                                    \
    do {                                                                      \
        ca_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)malloc(2 * sizeof(size_t) + (N) * sizeof((A)[0])) + 2); \
        ca_alen(A) = ca_acap(A) = (N);                                        \
    } while (0);

/** @brief Create a new array with all elements initialized to zero. */
#define ca_arr_new_n_zero(A, N)                                               \
    do {                                                                      \
        ca_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)calloc(1, 2 * sizeof(size_t) + (N) * sizeof((A)[0])) +  \
                2);                                                           \
        ca_alen(A) = ca_acap(A) = (N);                                        \
    } while (0);

/** @brief Create a new array with a specific length. */
#define ca_arr_new_reserve(A, N)                                              \
    do {                                                                      \
        ca_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)malloc(2 * sizeof(size_t) + (N) * sizeof((A)[0])) + 2); \
        ca_alen(A) = 0;                                                       \
        ca_acap(A) = (N);                                                     \
    } while (0);

/** @brief Create a new array with a specific length with all elements
 * initialized to zero. */
#define ca_arr_new_reserve_zero(A, N)                                         \
    do {                                                                      \
        ca_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)calloc(1, 2 * sizeof(size_t) + (N) * sizeof((A)[0])) +  \
                2);                                                           \
        ca_alen(A) = 0;                                                       \
        ca_acap(A) = (N);                                                     \
    } while (0);

#if !defined(__SIZEOF_SIZE_T__) || __SIZEOF_SIZE_T__ == 8
#define CA_ZEROES "\x00\x00\x00\x00\x00\x00"
#elif __SIZEOF_SIZE_T__ == 16
#define CA_ZEROES "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#elif __SIZEOF_SIZE_T__ == 4
#define CA_ZEROES "\x00\x00"
#else
#define CA_ZEROES
#endif

#include <ca/hex.h>

#if !defined(__BYTE_ORDER__) || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ca_literal(LO, HI)                                                    \
    ca_concat(CA_HEX_, LO) ca_concat(CA_HEX_, HI) CA_ZEROES
#else
#define ca_literal(LO, HI)                                                    \
    ca_concat(CA_HEX_, HI) ca_concat(CA_HEX_, LO) CA_ZEROES
#endif

/** @brief Embed a string literal and its metadata in an array without using
 * heap memory. */
#define ca_arr_from_string_literal(A, L, LO, HI)                              \
    do {                                                                      \
        ca_static_assert(sizeof(L) - 1 == (((HI) << 8) | (LO)),               \
                         "Invalid length.");                                  \
        ca_generic_assign((A), (ca_literal(LO, HI) ca_literal(LO, HI) L) +    \
                                   2 * sizeof(size_t));                       \
    } while (0)

/** @brief Free the memory allocated for an array. */
#define ca_arr_free(A) free(((size_t *)A) - 2);

/** @brief Resize an array to a new length. */
#define ca_arr_resize(A, N)                                                   \
    do {                                                                      \
        ca_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)(realloc((size_t *)(A) - 2,                             \
                               2 * sizeof(size_t) + (N) * sizeof((A)[0]))) +  \
                2);                                                           \
        ca_alen(A) = ca_acap(A) = (N);                                        \
    } while (0)

/** @brief Insert N uninitialized elements into an array at a specific index.
 */
#define ca_arr_insert_raw_n(A, I, N)                                          \
    do {                                                                      \
        assert((I) <= ca_alen(A));                                            \
                                                                              \
        if (ca_alen(A) + (N) > ca_acap(A)) {                                  \
            ca_acap(A) = (ca_alen(A) + (N)) * 2;                              \
                                                                              \
            ca_generic_assign(                                                \
                (A), (size_t *)(realloc((size_t *)(A) - 2,                    \
                                        2 * sizeof(size_t) +                  \
                                            ca_acap(A) * sizeof((A)[0]))) +   \
                         2);                                                  \
        }                                                                     \
                                                                              \
        if ((I) != ca_alen(A)) {                                              \
            memmove((A) + (I) + (N), (A) + (I),                               \
                    (ca_alen(A) - (I)) * sizeof((A)[0]));                     \
        }                                                                     \
                                                                              \
        ca_alen(A) += (N);                                                    \
    } while (0)

/** @brief Insert a single uninitialized element into an array at a specific
 * index.
 */
#define ca_arr_insert_raw(A, I) ca_arr_insert_raw_n(A, I, 1)

/** @brief Append N uninitialized elements to the end of an array. */
#define ca_arr_append_n_raw(A, N)                                             \
    do {                                                                      \
        /* Capture I before ca_alen(A) mutates. */                            \
        size_t ca_uniq(i) = ca_alen(A);                                       \
        ca_arr_insert_raw_n(A, ca_uniq(i), N);                                \
    } while (0)

/** @brief Append a single uninitialized element to the end of an array. */
#define ca_arr_append_raw(A) ca_arr_append_n_raw(A, 1)

/** @brief Insert N elements into an array at a specific index. */
#define ca_arr_insert_n(A, I, N, B)                                           \
    do {                                                                      \
        ca_arr_insert_raw_n(A, I, N);                                         \
        memcpy((A) + (I), (B), (N) * sizeof((A)[0]));                         \
    } while (0)

/** @brief Insert a single element into an array at a specific index. */
#define ca_arr_insert(A, I, E) ca_arr_insert_n(A, I, 1, &(E))

/** @brief Append N elements to the end of an array. */
#define ca_arr_append_n(A, N, B)                                              \
    do {                                                                      \
        /* Capture I before ca_alen(A) mutates. */                            \
        size_t ca_uniq(i) = ca_alen(A);                                       \
        ca_arr_insert_n(A, ca_uniq(i), N, B);                                 \
    } while (0)

/** @brief Append a single element to the end of an array. */
#define ca_arr_append(A, E) ca_arr_append_n(A, 1, &(E))

/** @brief Remove N elements from an array at a specific index. */
#define ca_arr_remove_n(A, I, N)                                              \
    do {                                                                      \
        assert((I) < ca_alen(A));                                             \
        assert((I) + (N) <= ca_alen(A));                                      \
                                                                              \
        if ((I) + (N) < ca_alen(A)) {                                         \
            memmove((A) + (I), (A) + (I) + (N),                               \
                    (ca_alen(A) - (I) - (N)) * sizeof((A)[0]));               \
        }                                                                     \
                                                                              \
        ca_alen(A) -= (N);                                                    \
    } while (0)

/** @brief Remove a single element from an array at a specific index. */
#define ca_arr_remove(A, I) ca_arr_remove_n(A, I, 1)

/** @brief Remove an element from an array at a specific pointer. */
#define ca_arr_remove_ptr(A, P) ca_arr_remove(A, (size_t)((P) - (A)))

/* -------------------------------------------------------------------------
   Generic type-safe hash map.
   ------------------------------------------------------------------------- */

#define ca_map(K, V)                                                          \
    struct {                                                                  \
        ca_arr(ca_arr(struct {                                                \
            K key;                                                            \
            V val;                                                            \
        })) bkts;                                                             \
                                                                              \
        size_t len;      /* Number of entries. */                             \
        size_t pop_bkts; /* Number of populated buckets. */                   \
    }

/** @brief Iterate over all key-value pairs in a hash map. */
#define ca_map_foreach(M, K, V)                                               \
    ca_arr_foreach((M).bkts, ca_uniq(i)) ca_arr_foreach(                      \
        (M).bkts[ca_uniq(i)],                                                 \
        ca_uniq(                                                              \
            j)) for (bool ca_uniq(once) = true;                               \
                     ca_uniq(                                                 \
                         once);) for ((K) =                                   \
                                          &((M).bkts[ca_uniq(i)][ca_uniq(j)]  \
                                                .key),                        \
                                      (V) =                                   \
                                          &((M).bkts[ca_uniq(i)][ca_uniq(j)]  \
                                                .val);                        \
                                      ca_uniq(once); ca_uniq(once) = false)

/** @brief Create a new hash map with a specific initial capacity. */
#define ca_map_new_reserve(M, N)                                              \
    do {                                                                      \
        ca_arr_new_n((M).bkts, N);                                            \
        (M).len = (M).pop_bkts = 0;                                           \
                                                                              \
        ca_arr_foreach((M).bkts, ca_uniq(i)) {                                \
            ca_arr_new_reserve((M).bkts[ca_uniq(i)], 2);                      \
        }                                                                     \
    } while (0)

/** @brief Create a new hash map with a default initial capacity. */
#define ca_map_new(M) ca_map_new_reserve(M, 16)

/** @brief Free the memory allocated for a hash map. */
#define ca_map_free(M)                                                        \
    do {                                                                      \
        ca_arr_foreach((M).bkts, ca_uniq(i)) {                                \
            ca_arr_free((M).bkts[ca_uniq(i)]);                                \
        }                                                                     \
        ca_arr_free((M).bkts);                                                \
    } while (0)

/** @brief Get a value from a hash map, returning NULL if not found. */
#define ca_map_get_maybe_null(M, HASH, EQ, K, V)                              \
    do {                                                                      \
        (V)                  = NULL;                                          \
        size_t ca_uniq(hash) = HASH(&(K)) % ca_acap((M).bkts);                \
                                                                              \
        ca_arr_foreach((M).bkts[ca_uniq(hash)], ca_uniq(i)) {                 \
            if (EQ(&(M).bkts[ca_uniq(hash)][ca_uniq(i)].key, &(K))) {         \
                (V) = &(M).bkts[ca_uniq(hash)][ca_uniq(i)].val;               \
                break;                                                        \
            }                                                                 \
        }                                                                     \
    } while (0)

/** @brief Get a value from a hash map, it must exist. */
#define ca_map_get(M, HASH, EQ, K, V)                                         \
    do {                                                                      \
        ca_map_get_maybe_null(M, HASH, EQ, K, V);                             \
        assert(V);                                                            \
    } while (0)

/** @brief Check if a key exists in a hash map. */
#define ca_map_contains(M, HASH, EQ, K, BOOL)                                 \
    do {                                                                      \
        void *ca_uniq(v);                                                     \
        ca_map_get_maybe_null(M, HASH, EQ, K, ca_uniq(v));                    \
        (BOOL) = ca_uniq(v) != NULL;                                          \
    } while (0)

/** @brief Internal macro for setting a value in a hash map without rehashing.
 */
#define ca_map_set_no_rehash(M, CAP, HASH, EQ, K, V)                          \
    do {                                                                      \
        size_t ca_uniq(hash) = HASH(&(K)) % (CAP);                            \
        /* If two keys within a bucket match the value will be replaced.      \
           Otherwise append new entry to bucket. */                           \
        bool ca_uniq(replaced) = false;                                       \
                                                                              \
        ca_foreach(ca_alen((M).bkts[ca_uniq(hash)]), ca_uniq(k)) {            \
            if (EQ(&(M).bkts[ca_uniq(hash)][ca_uniq(k)].key, &(K))) {         \
                (M).bkts[ca_uniq(hash)][ca_uniq(k)].val = V;                  \
                ca_uniq(replaced)                       = true;               \
                break;                                                        \
            }                                                                 \
        }                                                                     \
                                                                              \
        if (!ca_uniq(replaced)) {                                             \
            (M).len++;                                                        \
                                                                              \
            if (ca_alen((M).bkts[ca_uniq(hash)]) == 0) {                      \
                (M).pop_bkts++;                                               \
            }                                                                 \
                                                                              \
            ca_arr_append_raw((M).bkts[ca_uniq(hash)]);                       \
            ((M).bkts[ca_uniq(hash)])[ca_alen((M).bkts[ca_uniq(hash)]) - 1]   \
                .key = K;                                                     \
            ((M).bkts[ca_uniq(hash)])[ca_alen((M).bkts[ca_uniq(hash)]) - 1]   \
                .val = V;                                                     \
        }                                                                     \
    } while (0)

/** @brief Set a key-value pair in a hash map. Rehash if needed. */
#define ca_map_set(M, HASH, EQ, K, V)                                         \
    do {                                                                      \
        /* Grow array if needed. */                                           \
        if ((M).pop_bkts == ca_acap((M).bkts) / 2) {                          \
            (M).len = (M).pop_bkts = 0;                                       \
                                                                              \
            size_t ca_uniq(old_cap) = ca_acap((M).bkts);                      \
            size_t ca_uniq(new_cap) = ca_acap((M).bkts) * 2;                  \
                                                                              \
            /* Double number of buckets. Also allocate a 3rd region to        \
               temporarily store the old data while re-hashing. */            \
            ca_arr_resize((M).bkts, ca_uniq(new_cap) + ca_uniq(old_cap));     \
                                                                              \
            /* Move old data to the 3rd region. */                            \
            memcpy((M).bkts + ca_uniq(new_cap), (M).bkts,                     \
                   (ca_uniq(old_cap)) * sizeof((M).bkts[0]));                 \
            memset((M).bkts, 0x0, ca_uniq(old_cap) * sizeof((M).bkts[0]));    \
                                                                              \
            /* Initialize new buckets. */                                     \
            ca_foreach(ca_uniq(new_cap), ca_uniq(bkt)) {                      \
                ca_arr_new_reserve((M).bkts[ca_uniq(bkt)], 2);                \
            }                                                                 \
                                                                              \
            /* Re-hash. */                                                    \
            ca_foreach(ca_uniq(old_cap), ca_uniq(bkt)) {                      \
                ca_arr_foreach(((M).bkts + ca_uniq(new_cap))[ca_uniq(bkt)],   \
                               ca_uniq(j)) {                                  \
                    ca_map_set_no_rehash(                                     \
                        M, ca_uniq(new_cap), HASH, EQ,                        \
                        ((M).bkts +                                           \
                         ca_uniq(new_cap))[ca_uniq(bkt)][ca_uniq(j)]          \
                            .key,                                             \
                        ((M).bkts +                                           \
                         ca_uniq(new_cap))[ca_uniq(bkt)][ca_uniq(j)]          \
                            .val);                                            \
                }                                                             \
                                                                              \
                /* Free old bucket after re-hash. */                          \
                ca_arr_free(((M).bkts + ca_uniq(new_cap))[ca_uniq(bkt)]);     \
            }                                                                 \
                                                                              \
            /* Shrink array to new capacity. */                               \
            ca_acap((M).bkts) = ca_uniq(new_cap);                             \
        }                                                                     \
                                                                              \
        /* Insert new value. */                                               \
        ca_map_set_no_rehash(M, ca_acap((M).bkts), HASH, EQ, K, V);           \
    } while (0)

/** @brief Remove a key-value pair from a hash map. */
#define ca_map_remove(M, HASH, EQ, K)                                         \
    do {                                                                      \
        size_t ca_uniq(hash) = HASH(&(K)) % ca_acap((M).bkts);                \
                                                                              \
        ca_arr_foreach((M).bkts[ca_uniq(hash)], ca_uniq(j)) {                 \
            if (EQ(&(M).bkts[ca_uniq(hash)][ca_uniq(j)].key, &(K))) {         \
                ca_arr_remove((M).bkts[ca_uniq(hash)], ca_uniq(j));           \
                                                                              \
                (M).len--;                                                    \
                                                                              \
                if (ca_alen((M).bkts[ca_uniq(hash)]) == 0) {                  \
                    (M).pop_bkts--;                                           \
                }                                                             \
                break;                                                        \
            }                                                                 \
        }                                                                     \
    } while (0)

#endif
