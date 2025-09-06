#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <unity.h>
#include <ca/meta.h>

uint64_t int_hash(const int *key) {
    return (uint64_t)(*key);
}

bool int_eq(const int *a, const int *b) {
    return *a == *b;
}

void test_map(void) {
    ca_map(int, int) map;
    ca_map_new(map);

    TEST_ASSERT_EQUAL_INT(map.len, 0);

    int keys[5] = {1, 2, 3, 4, 5};
    int vals[5] = {6, 7, 8, 9, 10};

    ca_foreach(5, i) {
        ca_map_set(map, int_hash, int_eq, keys[i], vals[i]);
    }

    TEST_ASSERT_EQUAL_INT(map.len, 5);

    ca_foreach(5, i) {
        bool key_exists;
        ca_map_contains(map, int_hash, int_eq, keys[i], key_exists);
        TEST_ASSERT_TRUE(key_exists);
    }

    ca_foreach(5, i) {
        int *retrieved_val;
        ca_map_get(map, int_hash, int_eq, keys[i], retrieved_val);
        TEST_ASSERT_EQUAL_INT(*retrieved_val, vals[i]);
    }

    ca_map_remove(map, int_hash, int_eq, keys[1]);
    ca_map_remove(map, int_hash, int_eq, keys[0]);
    TEST_ASSERT_EQUAL_INT(map.len, 3);

    ca_foreach(3, i) {
        int *retrieved_val;
        ca_map_get(map, int_hash, int_eq, keys[i + 2], retrieved_val);
        TEST_ASSERT_EQUAL_INT(*retrieved_val, vals[i + 2]);
    }

    return;

    ca_map_remove(map, int_hash, int_eq, keys[4]);
    ca_map_remove(map, int_hash, int_eq, keys[5]);
    ca_map_remove(map, int_hash, int_eq, keys[3]);

    TEST_ASSERT_EQUAL_INT(map.len, 0);

    ca_map_free(map);
}