#include <unity.h>

#include <euclidean.h>

// required on Windows
void setUp(void) { }

// required on Windows
void tearDown(void) { }

void test_euclid_length_zero(void) {
    for (uint8_t density = 0; density < 16; density++) {
        TEST_ASSERT_EQUAL_UINT16(0, euclid(0, density));
    }
}

void test_euclid_length_one(void) {
    TEST_ASSERT_EQUAL_UINT16(0, euclid(1, 0));
    TEST_ASSERT_EQUAL_UINT16(1, euclid(1, 1));
}

void test_euclid_density_zero(void) {
    for (uint8_t length = 0; length < 16; length++) {
        TEST_ASSERT_EQUAL_UINT16(0, euclid(length, 0));
    }
}

void test_euclid_density_max(void) {
    for (uint8_t length = 1; length < 16; length++) {
        uint16_t expected = (1 << length) - 1;
        TEST_ASSERT_EQUAL_UINT16(expected, euclid(length, 16));
    }
}

void test_offset(void) {
    for (uint8_t offset = 0; offset < 16; offset++) {
        uint16_t expected = 0x01 << (15 - offset);
        TEST_ASSERT_EQUAL_UINT16(expected, pattern_offset(0b1000000000000000, 16, offset));
    }
}

void test_smoke(void) {
    // Just a bunch of patterns
    TEST_ASSERT_EQUAL_UINT16(0b1010, euclidean_rhythm_gen(4, 2, 0));
    TEST_ASSERT_EQUAL_UINT16(0b0101, euclidean_rhythm_gen(4, 2, 1));
    TEST_ASSERT_EQUAL_UINT16(0b1111100, euclidean_rhythm_gen(7, 5, 5));
    TEST_ASSERT_EQUAL_UINT16(0b01001001, euclidean_rhythm_gen(8, 3, 1));
    TEST_ASSERT_EQUAL_UINT16(0b10100100, euclidean_rhythm_gen(8, 3, 2));
    TEST_ASSERT_EQUAL_UINT16(0b0100101001001, euclidean_rhythm_gen(13, 5, 9));
    TEST_ASSERT_EQUAL_UINT16(0b1110111111111111, euclidean_rhythm_gen(16, 15, 4));

    // 16 rotations of every other beat with a rhythm of 16 steps
    for (uint8_t offset = 0; offset < 16; offset++) {
        uint16_t r = euclidean_rhythm_gen(16, 8, offset);
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
    RUN_TEST(test_offset);
    RUN_TEST(test_smoke);

    UNITY_END();
}