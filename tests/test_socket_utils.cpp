#include "socket_utils.h"
#include "test_compat.h"
#include <gtest/gtest.h>

#include <string>
#include <vector>

// Fixture tự động tạo trước test và đóng cặp socket sau test
class SocketUtilsTest : public ::testing::Test {
protected:
    socket_t sv[2]; // sv[0] là đầu gửi, sv[1] là đầu nhận
    void SetUp() override {
        // Tạo cặp socket ảo trong kernel
        ASSERT_TRUE(create_test_socketpair(sv));
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

// Test 3: Tính toàn vẹn của phân mảnh: Kiểm tra hàm send_exact và recv_exact
TEST_F(SocketUtilsTest, SendAndRecvExactIntegrity) {
    // Chuẩn bị buffer nhị phân 1024 bytes với dữ liệu mẫu
    std::vector<uint8_t> send_buffer(1024);
    for (size_t i = 0; i < send_buffer.size(); ++i) {
        send_buffer[i] = static_cast<uint8_t>(i % 256);
    }

    std::vector<uint8_t> recv_buffer(1024, 0);

    // Gửi chính xác 1024 bytes từ sv[0] và nhận chính xác ở sv[1]
    EXPECT_TRUE(RAT::send_exact(sv[0], send_buffer.data(), send_buffer.size()));
    EXPECT_TRUE(RAT::recv_exact(sv[1], recv_buffer.data(), recv_buffer.size()));

    EXPECT_EQ(send_buffer, recv_buffer);

    // Kiểm tra trường hợp tham số không hợp lệ (socket âm hoặc con trỏ null)
    EXPECT_FALSE(RAT::send_exact(-1, send_buffer.data(), send_buffer.size()));
    EXPECT_FALSE(RAT::send_exact(sv[0], nullptr, send_buffer.size()));
    EXPECT_FALSE(RAT::recv_exact(-1, recv_buffer.data(), recv_buffer.size()));
    EXPECT_FALSE(RAT::recv_exact(sv[1], nullptr, recv_buffer.size()));
}