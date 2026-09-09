#include <gtest/gtest.h>

#include "socket_utils.h"
#include "protocol.h"

#include <sys/socket.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <thread>

namespace fs = std::filesystem;

class FileTransferTest : public ::testing::Test {
protected:
    int sv[2];
    fs::path src_file;
    fs::path dst_file;

    void SetUp() override {
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

        src_file = fs::temp_directory_path() / "rat_test_src_5mb.bin";
        dst_file = fs::temp_directory_path() / "rat_test_dst_5mb.bin";

        // Tạo file mẫu 5MB (chứa dữ liệu byte lặp lại)
        std::ofstream ofs(src_file, std::ios::binary);
        std::vector<char> dummy_chunk(64 * 1024, 'A'); // 64 KB
        for (int i = 0; i < 80; ++i) { // 80 * 64KB = 5.12 MB
            ofs.write(dummy_chunk.data(), dummy_chunk.size());
        }
        ofs.close();
    }

    void TearDown() override {
        RAT::close_socket(sv[0]);
        RAT::close_socket(sv[1]);
        fs::remove(src_file);
        fs::remove(dst_file);
    }
};

TEST_F(FileTransferTest, Stream5MBFileMatchesExactly) {
    // Luồng gửi chạy nền mô phỏng Client
    std::thread sender([this]() {
        EXPECT_TRUE(RAT::send_file_stream(sv[0], src_file.string()));
    });

    // Luồng nhận đóng vai Server
    EXPECT_TRUE(RAT::recv_file_stream(sv[1], dst_file.string()));

    sender.join();

    // Kiểm tra kích thước file nhận được có bằng file nguồn 
    ASSERT_TRUE(fs::exists(dst_file));
    EXPECT_EQ(fs::file_size(src_file), fs::file_size(dst_file));
}