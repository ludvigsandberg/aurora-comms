#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <unity.h>
#include <ac/meta.h>

uint64_t int_hash(const int *key) {
    return (uint64_t)(*key);
}

bool int_eq(const int *a, const int *b) {
    return *a == *b;
}

static ac_map(int, int) map;
static int keys[5] = {1, 2, 3, 4, 5};
static int vals[5] = {6, 7, 8, 9, 10};

void setUp(void) {
    // Called before each test
    ac_map_new(map);
}

void tearDown(void) {
    // Called after each test
    ac_map_free(map);
}

void test_map_initially_empty(void) {
    TEST_ASSERT_EQUAL_INT(0, map.len);
}

void test_map_insert_and_len(void) {
    for (size_t i = 0; i < 5; i++) {
        ac_map_set(map, int_hash, int_eq, keys[i], vals[i]);
    }
    TEST_ASSERT_EQUAL_INT(5, map.len);
}

void test_map_contains_keys(void) {
    for (size_t i = 0; i < 5; i++) {
        ac_map_set(map, int_hash, int_eq, keys[i], vals[i]);
    }
    for (size_t i = 0; i < 5; i++) {
        bool exists;
        ac_map_contains(map, int_hash, int_eq, keys[i], exists);
        TEST_ASSERT_TRUE(exists);
    }
}

void test_map_retrieve_values(void) {
    for (size_t i = 0; i < 5; i++) {
        ac_map_set(map, int_hash, int_eq, keys[i], vals[i]);
    }
    for (size_t i = 0; i < 5; i++) {
        int *retrieved;
        ac_map_get(map, int_hash, int_eq, keys[i], retrieved);
        TEST_ASSERT_EQUAL_INT(vals[i], *retrieved);
    }
}

void test_map_remove_keys(void) {
    for (size_t i = 0; i < 5; i++) {
        ac_map_set(map, int_hash, int_eq, keys[i], vals[i]);
    }
    ac_map_remove(map, int_hash, int_eq, keys[0]);
    ac_map_remove(map, int_hash, int_eq, keys[1]);
    TEST_ASSERT_EQUAL_INT(3, map.len);

    for (size_t i = 2; i < 5; i++) {
        int *retrieved;
        ac_map_get(map, int_hash, int_eq, keys[i], retrieved);
        TEST_ASSERT_EQUAL_INT(vals[i], *retrieved);
    }

    // Remove the rest
    for (size_t i = 2; i < 5; i++) {
        ac_map_remove(map, int_hash, int_eq, keys[i]);
    }
    TEST_ASSERT_EQUAL_INT(0, map.len);
}