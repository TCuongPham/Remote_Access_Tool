#include <gtest/gtest.h>

// Khởi tạo và chạy các test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}