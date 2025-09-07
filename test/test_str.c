#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <unity.h>
#include <ac/meta.h>
#include <ac/str.h>

void test_string_length(void) {
    ac_string_t a;
    ac_arr_from_string_literal(a, "hello world", 11, 0);
    TEST_ASSERT_EQUAL_INT(11, (int)ac_alen(a));
}

void test_string_concat(void) {
    ac_string_t b;
    ac_arr_new(b);
    ac_arr_append_n(b, 5, "hello");
    ac_arr_append_n(b, 6, " world");
    TEST_ASSERT_EQUAL_INT(11, (int)ac_alen(b));
}
