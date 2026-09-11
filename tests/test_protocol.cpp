#include "protocol.h"
#include "platform.h"

#include <gtest/gtest.h>
#include <cstring>

// Test 1: Kiểm tra các hằng số kích thước và cấu hình mặc định trong protocol
TEST(ProtocolTest, ProtocolConstants) {
    EXPECT_EQ(RAT::DEFAULT_PORT, 8888);
    EXPECT_STREQ(RAT::DEFAULT_HOST, "127.0.0.1");
    EXPECT_EQ(RAT::HEADER_SIZE, sizeof(uint32_t));
    EXPECT_EQ(RAT::MAX_MESSAGE_SIZE, 10 * 1024 * 1024);
    EXPECT_EQ(RAT::FILE_CHUNK_SIZE, 64 * 1024);
}

// Test 2: Kiểm tra hằng số chuỗi các lệnh (Commands) và trạng thái (Status)
TEST(ProtocolTest, CommandAndStatusStrings) {
    EXPECT_EQ(RAT::Command::LIST_DIR, "LIST_DIR");
    EXPECT_EQ(RAT::Command::READ_FILE, "READ_FILE");
    EXPECT_EQ(RAT::Command::LIST_PROC, "LIST_PROC");
    EXPECT_EQ(RAT::Command::KILL_PROC, "KILL_PROC");
    EXPECT_EQ(RAT::Command::HELP, "HELP");
    EXPECT_EQ(RAT::Command::EXIT, "EXIT");
    EXPECT_EQ(RAT::Command::DOWNLOAD_FILE, "DOWNLOAD_FILE");

    EXPECT_EQ(RAT::Status::OK, "[+] ");
    EXPECT_EQ(RAT::Status::ERR, "[!] ");
    EXPECT_EQ(RAT::Status::INFO, "[*] ");
}

// Test 3: Kiểm tra cấu trúc header truyền file FileTransferHeader
TEST(ProtocolTest, FileTransferHeaderLayout) {
    RAT::FileTransferHeader header;
    header.status_code = 0;
    header.file_size = 104857600ULL; // 100MB

    EXPECT_EQ(header.status_code, 0);
    EXPECT_EQ(header.file_size, 104857600ULL);
    EXPECT_GE(sizeof(RAT::FileTransferHeader), sizeof(uint8_t) + sizeof(uint64_t));
}

// Test 4: Kiểm thử chuyển đổi Endian 64-bit và 32-bit (htobe64, be64toh, htonl, ntohl)
TEST(ProtocolTest, EndianConversionIntegrity) {
    // 64-bit (cho kích thước file hỗ trợ > 4GB)
    uint64_t original_64 = 0x123456789ABCDEF0ULL;
    uint64_t be_64 = rat_htobe64(original_64);
    uint64_t recovered_64 = rat_be64toh(be_64);
    EXPECT_EQ(recovered_64, original_64);

    uint64_t large_file_size = 5ULL * 1024 * 1024 * 1024; // 5 GB
    EXPECT_EQ(rat_be64toh(rat_htobe64(large_file_size)), large_file_size);

    // 32-bit (cho độ dài gói tin length-prefixed)
    uint32_t original_32 = 0xA1B2C3D4;
    uint32_t net_32 = htonl(original_32);
    uint32_t host_32 = ntohl(net_32);
    EXPECT_EQ(host_32, original_32);
}
