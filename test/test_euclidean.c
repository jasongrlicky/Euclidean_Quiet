#include <unity.h>

#include <euclidean.h>

// required on Windows
void setUp(void) { }

// required on Windows
void tearDown(void) { }

void test_zero_length(void) {
    for (uint8_t density = 0; density < 16; density++) {
        for (uint8_t offset = 0; offset < 16; offset++) {
            TEST_ASSERT_EQUAL_UINT16(0, euclid(0, density, offset));
        }
    }
}

void test_zero_density(void) {
    for (uint8_t length = 0; length < 16; length++) {
        for (uint8_t offset = 0; offset < 16; offset++) {
            TEST_ASSERT_EQUAL_UINT16(0, euclid(length, 0, offset));
        }
    }
}

void test_max_density(void) {
    for (uint8_t length = 1; length < 16; length++) {
        for (uint8_t offset = 0; offset < length; offset++) {
            TEST_ASSERT_EQUAL_UINT16((1 << length) - 1, euclid(length, 16, offset));
        }
    }
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_zero_length);
    RUN_TEST(test_zero_density);
    RUN_TEST(test_max_density);

    UNITY_END();
}