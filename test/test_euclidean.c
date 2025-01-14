#include <unity.h>

#include <euclidean.h>

// required on Windows
void setUp(void) { }

// required on Windows
void tearDown(void) { }

void test_euclid_length_zero(void) {
    for (uint8_t density = 0; density < 16; density++) {
        TEST_ASSERT_EQUAL_UINT16(0, euclidean_pattern(0, density));
    }
}

void test_euclid_length_one(void) {
    TEST_ASSERT_EQUAL_UINT16(0, euclidean_pattern(1, 0));
    TEST_ASSERT_EQUAL_UINT16(1, euclidean_pattern(1, 1));
}

void test_euclid_density_zero(void) {
    for (uint8_t length = 0; length < 16; length++) {
        TEST_ASSERT_EQUAL_UINT16(0, euclidean_pattern(length, 0));
    }
}

void test_euclid_density_max(void) {
    for (uint8_t length = 1; length < 16; length++) {
        uint16_t expected = (1 << length) - 1;
        TEST_ASSERT_EQUAL_UINT16(expected, euclidean_pattern(length, 16));
    }
}

/// Every Euclidean rhythm pattern example in the original paper
void test_euclid_smoke(void) {
    TEST_ASSERT_EQUAL_UINT16(0b10, euclidean_pattern(2, 1));
    TEST_ASSERT_EQUAL_UINT16(0b100, euclidean_pattern(3, 1));
    TEST_ASSERT_EQUAL_UINT16(0b1000, euclidean_pattern(4, 1));
    TEST_ASSERT_EQUAL_UINT16(0b100100100100, euclidean_pattern(12, 4));
    TEST_ASSERT_EQUAL_UINT16(0b101, euclidean_pattern(3, 2));
    TEST_ASSERT_EQUAL_UINT16(0b10100, euclidean_pattern(5, 2));
    TEST_ASSERT_EQUAL_UINT16(0b1011, euclidean_pattern(4, 3));
    TEST_ASSERT_EQUAL_UINT16(0b10101, euclidean_pattern(5, 3));
    TEST_ASSERT_EQUAL_UINT16(0b1010100, euclidean_pattern(7, 3));
    TEST_ASSERT_EQUAL_UINT16(0b10010010, euclidean_pattern(8, 3));
    TEST_ASSERT_EQUAL_UINT16(0b1010101, euclidean_pattern(7, 4));
    TEST_ASSERT_EQUAL_UINT16(0b101010100, euclidean_pattern(9, 4));
    TEST_ASSERT_EQUAL_UINT16(0b10010010010, euclidean_pattern(11, 4));
    TEST_ASSERT_EQUAL_UINT16(0b101111, euclidean_pattern(6, 5));
    TEST_ASSERT_EQUAL_UINT16(0b1011011, euclidean_pattern(7, 5));
    TEST_ASSERT_EQUAL_UINT16(0b10110110, euclidean_pattern(8, 5));
    TEST_ASSERT_EQUAL_UINT16(0b101010101, euclidean_pattern(9, 5));
    TEST_ASSERT_EQUAL_UINT16(0b10101010100, euclidean_pattern(11, 5));
    TEST_ASSERT_EQUAL_UINT16(0b100101001010, euclidean_pattern(12, 5));
    TEST_ASSERT_EQUAL_UINT16(0b1001010010100, euclidean_pattern(13, 5));
    TEST_ASSERT_EQUAL_UINT16(0b1001001001001000, euclidean_pattern(16, 5));
    TEST_ASSERT_EQUAL_UINT16(0b10111111, euclidean_pattern(8, 7));
    TEST_ASSERT_EQUAL_UINT16(0b101101011010, euclidean_pattern(12, 7));
    TEST_ASSERT_EQUAL_UINT16(0b1001010100101010, euclidean_pattern(16, 7));
    TEST_ASSERT_EQUAL_UINT16(0b1011010101101010, euclidean_pattern(16, 9));
}

void test_rotate(void) {
    for (uint8_t offset = 0; offset < 16; offset++) {
        uint16_t expected = 0x01 << (15 - offset);
        TEST_ASSERT_EQUAL_UINT16(expected, pattern_rotate(0b1000000000000000, 16, offset));
    }
}

void test_rotate_beyond_max(void) {
    TEST_ASSERT_EQUAL_UINT16(0b000011, pattern_rotate(0b000011, 6, 19));
}

void test_euclidean_rotate_smoke(void) {
    // Just a bunch of strings
    TEST_ASSERT_EQUAL_UINT16(0b1010, euclidean_pattern_rotate(4, 2, 0));
    TEST_ASSERT_EQUAL_UINT16(0b0101, euclidean_pattern_rotate(4, 2, 1));
    TEST_ASSERT_EQUAL_UINT16(0b1111100, euclidean_pattern_rotate(7, 5, 5));
    TEST_ASSERT_EQUAL_UINT16(0b01001001, euclidean_pattern_rotate(8, 3, 1));
    TEST_ASSERT_EQUAL_UINT16(0b10100100, euclidean_pattern_rotate(8, 3, 2));
    TEST_ASSERT_EQUAL_UINT16(0b0100101001001, euclidean_pattern_rotate(13, 5, 9));
    TEST_ASSERT_EQUAL_UINT16(0b1110111111111111, euclidean_pattern_rotate(16, 15, 4));

    // 16 rotations of every other beat with a rhythm of 16 steps
    for (uint8_t offset = 0; offset < 16; offset++) {
        uint16_t r = euclidean_pattern_rotate(16, 8, offset);
        if ((offset % 2) == 0) {
            TEST_ASSERT_EQUAL_UINT16(0b1010101010101010, r);
        } else {
            TEST_ASSERT_EQUAL_UINT16(0b0101010101010101, r);
        }
    }
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_euclid_length_zero);
    RUN_TEST(test_euclid_length_one);
    RUN_TEST(test_euclid_density_zero);
    RUN_TEST(test_euclid_density_max);
    RUN_TEST(test_euclid_smoke);
    RUN_TEST(test_rotate);
    RUN_TEST(test_rotate_beyond_max);
    RUN_TEST(test_euclidean_rotate_smoke);

    UNITY_END();
}