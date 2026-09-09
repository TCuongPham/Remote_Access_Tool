#include "socket_utils.h"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>
#include <string>

// Fixture tự động tạo trước test và đóng cặp socket sau test
class SocketUtilsTest : public ::testing::Test {
protected:
    int sv[2]; // sv[0] là đầu gửi, sv[1] là đầu nhận
    void SetUp() override {
        // Tạo cặp socket ảo trong kernel
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
    }
    void TearDown() override {
        RAT::close_socket(sv[0]);
        RAT::close_socket(sv[1]);
    }
};

// Test 1: Gửi và nhận thông điệp msg
TEST_F(SocketUtilsTest, SendAndReceiveStringMessage) {
    std::string test_msg = "PING_TEST_MESSAGE";
    EXPECT_TRUE(RAT::send_message(sv[0], test_msg));
    std::string received_msg;
    EXPECT_TRUE(RAT::recv_message(sv[1], received_msg));
    EXPECT_EQ(received_msg, test_msg);
}

// Test 2: Gửi và nhận thông điệp rỗng
TEST_F(SocketUtilsTest, SendAndReceiveEmptyMessage) {
    std::string empty_msg = "";
    EXPECT_TRUE(RAT::send_message(sv[0], empty_msg));
    std::string received_msg;
    EXPECT_TRUE(RAT::recv_message(sv[1], received_msg));
    EXPECT_TRUE(received_msg.empty());
}