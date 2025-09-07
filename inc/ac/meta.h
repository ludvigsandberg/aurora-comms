#ifndef AC_META_H
#define AC_META_H

/* -------------------------------------------------------------------------
   Macro metaprogramming.
   ------------------------------------------------------------------------- */

#define ac_concat2(A, B) A##B
#define ac_concat(A, B)  ac_concat2(A, B)

/** @brief Generate a unique name for internal macro identifiers. */
#define ac_uniq(NAME) ac_concat(NAME, __LINE__)

/** @brief Static assertion for compile-time checks. */
#ifdef __GNUC__
#define ac_static_assert(COND, MSG)                                           \
    __attribute__((unused)) typedef char ac_uniq(                             \
        ac_static_assert)[(COND) ? 1 : -1]
#else
#define ac_static_assert(COND, MSG)
#endif

/** @brief Assign pointer to pointer without type cast. */
#define ac_generic_assign(A, B)                                               \
    do {                                                                      \
        void *ac_uniq(m) = B;                                                 \
        memcpy(&A, &ac_uniq(m), sizeof(void *));                              \
    } while (0)

/** @brief Iterate over a range of values. */
#define ac_foreach(N, I) for (size_t I = 0; I < (N); I++)

/* -------------------------------------------------------------------------
   Generic type-safe dynamic array.
   Metadata about array length and capacity is stored at
   indices -2 and -1 and array elements begin like normal at index 0.
   ------------------------------------------------------------------------- */

#define ac_arr(T) T *

typedef ac_arr(unsigned char) ac_bytes_t;
typedef ac_arr(int) ac_ints_t;
typedef ac_arr(double) ac_doubles_t;

/** @brief Get the length of an array. */
#define ac_alen(A) ((size_t *)(A))[-2]
/** @brief Get the capacity of an array. */
#define ac_acap(A) ((size_t *)(A))[-1]

/** @brief Iterate over the elements of an array. */
#define ac_arr_foreach(A, I) for (size_t I = 0; I < ac_alen(A); I++)

/** @brief Create a new empty array. */
#define ac_arr_new(A)                                                         \
    ac_generic_assign((A), (size_t *)(calloc(2, sizeof(size_t))) + 2);

/** @brief Create a new array with a specific length. */
#define ac_arr_new_n(A, N)                                                    \
    do {                                                                      \
        ac_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)malloc(2 * sizeof(size_t) + (N) * sizeof((A)[0])) + 2); \
        ac_alen(A) = ac_acap(A) = (N);                                        \
    } while (0);

/** @brief Create a new array with all elements initialized to zero. */
#define ac_arr_new_n_zero(A, N)                                               \
    do {                                                                      \
        ac_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)calloc(1, 2 * sizeof(size_t) + (N) * sizeof((A)[0])) +  \
                2);                                                           \
        ac_alen(A) = ac_acap(A) = (N);                                        \
    } while (0);

/** @brief Create a new array with a specific length. */
#define ac_arr_new_reserve(A, N)                                              \
    do {                                                                      \
        ac_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)malloc(2 * sizeof(size_t) + (N) * sizeof((A)[0])) + 2); \
        ac_alen(A) = 0;                                                       \
        ac_acap(A) = (N);                                                     \
    } while (0);

/** @brief Create a new array with a specific length with all elements
 * initialized to zero. */
#define ac_arr_new_reserve_zero(A, N)                                         \
    do {                                                                      \
        ac_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)calloc(1, 2 * sizeof(size_t) + (N) * sizeof((A)[0])) +  \
                2);                                                           \
        ac_alen(A) = 0;                                                       \
        ac_acap(A) = (N);                                                     \
    } while (0);

#if !defined(__SIZEOF_SIZE_T__) || __SIZEOF_SIZE_T__ == 8
#define AC_ZEROES "\x00\x00\x00\x00\x00\x00"
#elif __SIZEOF_SIZE_T__ == 16
#define AC_ZEROES "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#elif __SIZEOF_SIZE_T__ == 4
#define AC_ZEROES "\x00\x00"
#else
#define AC_ZEROES
#endif

#include <ac/hex.h>

#if !defined(__BYTE_ORDER__) || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ac_literal(LO, HI)                                                    \
    ac_concat(AC_HEX_, LO) ac_concat(AC_HEX_, HI) AC_ZEROES
#else
#define ac_literal(LO, HI)                                                    \
    ac_concat(AC_HEX_, HI) ac_concat(AC_HEX_, LO) AC_ZEROES
#endif

/** @brief Embed a string literal and its metadata in an array without using
 * heap memory. */
#define ac_arr_from_string_literal(A, L, LO, HI)                              \
    do {                                                                      \
        ac_static_assert(sizeof(L) - 1 == (((HI) << 8) | (LO)),               \
                         "Invalid length.");                                  \
        ac_generic_assign((A), (ac_literal(LO, HI) ac_literal(LO, HI) L) +    \
                                   2 * sizeof(size_t));                       \
    } while (0)

/** @brief Free the memory allocated for an array. */
#define ac_arr_free(A) free(((size_t *)A) - 2);

/** @brief Resize an array to a new length. */
#define ac_arr_resize(A, N)                                                   \
    do {                                                                      \
        ac_generic_assign(                                                    \
            (A),                                                              \
            (size_t *)(realloc((size_t *)(A) - 2,                             \
                               2 * sizeof(size_t) + (N) * sizeof((A)[0]))) +  \
                2);                                                           \
        ac_alen(A) = ac_acap(A) = (N);                                        \
    } while (0)

/** @brief Insert N uninitialized elements into an array at a specific index.
 */
#define ac_arr_insert_raw_n(A, I, N)                                          \
    do {                                                                      \
        assert((I) <= ac_alen(A));                                            \
                                                                              \
        if (ac_alen(A) + (N) > ac_acap(A)) {                                  \
            ac_acap(A) = (ac_alen(A) + (N)) * 2;                              \
                                                                              \
            ac_generic_assign(                                                \
                (A), (size_t *)(realloc((size_t *)(A) - 2,                    \
                                        2 * sizeof(size_t) +                  \
                                            ac_acap(A) * sizeof((A)[0]))) +   \
                         2);                                                  \
        }                                                                     \
                                                                              \
        if ((I) != ac_alen(A)) {                                              \
            memmove((A) + (I) + (N), (A) + (I),                               \
                    (ac_alen(A) - (I)) * sizeof((A)[0]));                     \
        }                                                                     \
                                                                              \
        ac_alen(A) += (N);                                                    \
    } while (0)

/** @brief Insert a single uninitialized element into an array at a specific
 * index.
 */
#define ac_arr_insert_raw(A, I) ac_arr_insert_raw_n(A, I, 1)

/** @brief Append N uninitialized elements to the end of an array. */
#define ac_arr_append_n_raw(A, N)                                             \
    do {                                                                      \
        /* Capture I before ac_alen(A) mutates. */                            \
        size_t ac_uniq(i) = ac_alen(A);                                       \
        ac_arr_insert_raw_n(A, ac_uniq(i), N);                                \
    } while (0)

/** @brief Append a single uninitialized element to the end of an array. */
#define ac_arr_append_raw(A) ac_arr_append_n_raw(A, 1)

/** @brief Insert N elements into an array at a specific index. */
#define ac_arr_insert_n(A, I, N, B)                                           \
    do {                                                                      \
        ac_arr_insert_raw_n(A, I, N);                                         \
        memcpy((A) + (I), (B), (N) * sizeof((A)[0]));                         \
    } while (0)

/** @brief Insert a single element into an array at a specific index. */
#define ac_arr_insert(A, I, E) ac_arr_insert_n(A, I, 1, &(E))

/** @brief Append N elements to the end of an array. */
#define ac_arr_append_n(A, N, B)                                              \
    do {                                                                      \
        /* Capture I before ac_alen(A) mutates. */                            \
        size_t ac_uniq(i) = ac_alen(A);                                       \
        ac_arr_insert_n(A, ac_uniq(i), N, B);                                 \
    } while (0)

/** @brief Append a single element to the end of an array. */
#define ac_arr_append(A, E) ac_arr_append_n(A, 1, &(E))

/** @brief Remove N elements from an array at a specific index. */
#define ac_arr_remove_n(A, I, N)                                              \
    do {                                                                      \
        assert((I) < ac_alen(A));                                             \
        assert((I) + (N) <= ac_alen(A));                                      \
                                                                              \
        if ((I) + (N) < ac_alen(A)) {                                         \
            memmove((A) + (I), (A) + (I) + (N),                               \
                    (ac_alen(A) - (I) - (N)) * sizeof((A)[0]));               \
        }                                                                     \
                                                                              \
        ac_alen(A) -= (N);                                                    \
    } while (0)

/** @brief Remove a single element from an array at a specific index. */
#define ac_arr_remove(A, I) ac_arr_remove_n(A, I, 1)

/** @brief Remove an element from an array at a specific pointer. */
#define ac_arr_remove_ptr(A, P) ac_arr_remove(A, (size_t)((P) - (A)))

/* -------------------------------------------------------------------------
   Generic type-safe hash map.
   ------------------------------------------------------------------------- */

#define ac_map(K, V)                                                          \
    struct {                                                                  \
        ac_arr(ac_arr(struct {                                                \
            K key;                                                            \
            V val;                                                            \
        })) bkts;                                                             \
                                                                              \
        size_t len;      /* Number of entries. */                             \
        size_t pop_bkts; /* Number of populated buckets. */                   \
    }

/** @brief Iterate over all key-value pairs in a hash map. */
#define ac_map_foreach(M, K, V)                                               \
    ac_arr_foreach((M).bkts, ac_uniq(i)) ac_arr_foreach(                      \
        (M).bkts[ac_uniq(i)],                                                 \
        ac_uniq(                                                              \
            j)) for (bool ac_uniq(once) = true;                               \
                     ac_uniq(                                                 \
                         once);) for ((K) =                                   \
                                          &((M).bkts[ac_uniq(i)][ac_uniq(j)]  \
                                                .key),                        \
                                      (V) =                                   \
                                          &((M).bkts[ac_uniq(i)][ac_uniq(j)]  \
                                                .val);                        \
                                      ac_uniq(once); ac_uniq(once) = false)

/** @brief Create a new hash map with a specific initial capacity. */
#define ac_map_new_reserve(M, N)                                              \
    do {                                                                      \
        ac_arr_new_n((M).bkts, N);                                            \
        (M).len = (M).pop_bkts = 0;                                           \
                                                                              \
        ac_arr_foreach((M).bkts, ac_uniq(i)) {                                \
            ac_arr_new_reserve((M).bkts[ac_uniq(i)], 2);                      \
        }                                                                     \
    } while (0)

/** @brief Create a new hash map with a default initial capacity. */
#define ac_map_new(M) ac_map_new_reserve(M, 16)

/** @brief Free the memory allocated for a hash map. */
#define ac_map_free(M)                                                        \
    do {                                                                      \
        ac_arr_foreach((M).bkts, ac_uniq(i)) {                                \
            ac_arr_free((M).bkts[ac_uniq(i)]);                                \
        }                                                                     \
        ac_arr_free((M).bkts);                                                \
    } while (0)

/** @brief Get a value from a hash map, returning NULL if not found. */
#define ac_map_get_maybe_null(M, HASH, EQ, K, V)                              \
    do {                                                                      \
        (V)                  = NULL;                                          \
        size_t ac_uniq(hash) = HASH(&(K)) % ac_acap((M).bkts);                \
                                                                              \
        ac_arr_foreach((M).bkts[ac_uniq(hash)], ac_uniq(i)) {                 \
            if (EQ(&(M).bkts[ac_uniq(hash)][ac_uniq(i)].key, &(K))) {         \
                (V) = &(M).bkts[ac_uniq(hash)][ac_uniq(i)].val;               \
                break;                                                        \
            }                                                                 \
        }                                                                     \
    } while (0)

/** @brief Get a value from a hash map, it must exist. */
#define ac_map_get(M, HASH, EQ, K, V)                                         \
    do {                                                                      \
        ac_map_get_maybe_null(M, HASH, EQ, K, V);                             \
        assert(V);                                                            \
    } while (0)

/** @brief Check if a key exists in a hash map. */
#define ac_map_contains(M, HASH, EQ, K, BOOL)                                 \
    do {                                                                      \
        void *ac_uniq(v);                                                     \
        ac_map_get_maybe_null(M, HASH, EQ, K, ac_uniq(v));                    \
        (BOOL) = ac_uniq(v) != NULL;                                          \
    } while (0)

/** @brief Internal macro for setting a value in a hash map without rehashing.
 */
#define ac_map_set_no_rehash(M, CAP, HASH, EQ, K, V)                          \
    do {                                                                      \
        size_t ac_uniq(hash) = HASH(&(K)) % (CAP);                            \
        /* If two keys within a bucket match the value will be replaced.      \
           Otherwise append new entry to bucket. */                           \
        bool ac_uniq(replaced) = false;                                       \
                                                                              \
        ac_foreach(ac_alen((M).bkts[ac_uniq(hash)]), ac_uniq(k)) {            \
            if (EQ(&(M).bkts[ac_uniq(hash)][ac_uniq(k)].key, &(K))) {         \
                (M).bkts[ac_uniq(hash)][ac_uniq(k)].val = V;                  \
                ac_uniq(replaced)                       = true;               \
                break;                                                        \
            }                                                                 \
        }                                                                     \
                                                                              \
        if (!ac_uniq(replaced)) {                                             \
            (M).len++;                                                        \
                                                                              \
            if (ac_alen((M).bkts[ac_uniq(hash)]) == 0) {                      \
                (M).pop_bkts++;                                               \
            }                                                                 \
                                                                              \
            ac_arr_append_raw((M).bkts[ac_uniq(hash)]);                       \
            ((M).bkts[ac_uniq(hash)])[ac_alen((M).bkts[ac_uniq(hash)]) - 1]   \
                .key = K;                                                     \
            ((M).bkts[ac_uniq(hash)])[ac_alen((M).bkts[ac_uniq(hash)]) - 1]   \
                .val = V;                                                     \
        }                                                                     \
    } while (0)

/** @brief Set a key-value pair in a hash map. Rehash if needed. */
#define ac_map_set(M, HASH, EQ, K, V)                                         \
    do {                                                                      \
        /* Grow array if needed. */                                           \
        if ((M).pop_bkts == ac_acap((M).bkts) / 2) {                          \
            (M).len = (M).pop_bkts = 0;                                       \
                                                                              \
            size_t ac_uniq(old_cap) = ac_acap((M).bkts);                      \
            size_t ac_uniq(new_cap) = ac_acap((M).bkts) * 2;                  \
                                                                              \
            /* Double number of buckets. Also allocate a 3rd region to        \
               temporarily store the old data while re-hashing. */            \
            ac_arr_resize((M).bkts, ac_uniq(new_cap) + ac_uniq(old_cap));     \
                                                                              \
            /* Move old data to the 3rd region. */                            \
            memcpy((M).bkts + ac_uniq(new_cap), (M).bkts,                     \
                   (ac_uniq(old_cap)) * sizeof((M).bkts[0]));                 \
            memset((M).bkts, 0x0, ac_uniq(old_cap) * sizeof((M).bkts[0]));    \
                                                                              \
            /* Initialize new buckets. */                                     \
            ac_foreach(ac_uniq(new_cap), ac_uniq(bkt)) {                      \
                ac_arr_new_reserve((M).bkts[ac_uniq(bkt)], 2);                \
            }                                                                 \
                                                                              \
            /* Re-hash. */                                                    \
            ac_foreach(ac_uniq(old_cap), ac_uniq(bkt)) {                      \
                ac_arr_foreach(((M).bkts + ac_uniq(new_cap))[ac_uniq(bkt)],   \
                               ac_uniq(j)) {                                  \
                    ac_map_set_no_rehash(                                     \
                        M, ac_uniq(new_cap), HASH, EQ,                        \
                        ((M).bkts +                                           \
                         ac_uniq(new_cap))[ac_uniq(bkt)][ac_uniq(j)]          \
                            .key,                                             \
                        ((M).bkts +                                           \
                         ac_uniq(new_cap))[ac_uniq(bkt)][ac_uniq(j)]          \
                            .val);                                            \
                }                                                             \
                                                                              \
                /* Free old bucket after re-hash. */                          \
                ac_arr_free(((M).bkts + ac_uniq(new_cap))[ac_uniq(bkt)]);     \
            }                                                                 \
                                                                              \
            /* Shrink array to new capacity. */                               \
            ac_acap((M).bkts) = ac_uniq(new_cap);                             \
        }                                                                     \
                                                                              \
        /* Insert new value. */                                               \
        ac_map_set_no_rehash(M, ac_acap((M).bkts), HASH, EQ, K, V);           \
    } while (0)

/** @brief Remove a key-value pair from a hash map. */
#define ac_map_remove(M, HASH, EQ, K)                                         \
    do {                                                                      \
        size_t ac_uniq(hash) = HASH(&(K)) % ac_acap((M).bkts);                \
                                                                              \
        ac_arr_foreach((M).bkts[ac_uniq(hash)], ac_uniq(j)) {                 \
            if (EQ(&(M).bkts[ac_uniq(hash)][ac_uniq(j)].key, &(K))) {         \
                ac_arr_remove((M).bkts[ac_uniq(hash)], ac_uniq(j));           \
                                                                              \
                (M).len--;                                                    \
                                                                              \
                if (ac_alen((M).bkts[ac_uniq(hash)]) == 0) {                  \
                    (M).pop_bkts--;                                           \
                }                                                             \
                break;                                                        \
            }                                                                 \
        }                                                                     \
    } while (0)

#endif
