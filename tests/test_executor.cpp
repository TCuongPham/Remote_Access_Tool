#include "executor.h"
#include "protocol.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Test 1: Kiểm tra list_directory với đường dẫn không tồn tại -> bắt đầu bằng tiền tố báo lỗi [!]
TEST(ExecutorTest, ListDirectoryNonExistentPath) {
    std::string result = RAT::Executor::list_directory("/duong_dan_khong_ton_tai");
    EXPECT_EQ(result.rfind(RAT::Status::ERR, 0), 0); // Bắt đầu bằng "[!] "
}

// Test 2: Kiểm tra list_directory với thư mục hiện tại -> bắt đầu bằng tiền tố thành công [+] và chứa bảng danh sách file
TEST(ExecutorTest, ListDirectoryCurrentPathSuccess) {
    std::string result = RAT::Executor::list_directory(".");
    EXPECT_EQ(result.rfind(RAT::Status::OK, 0), 0); // Bắt đầu bằng "[+] "
    EXPECT_NE(result.find("TYPE"), std::string::npos);
    EXPECT_NE(result.find("SIZE (Bytes)"), std::string::npos);
    EXPECT_NE(result.find("NAME"), std::string::npos);
}

// Test 3: Kiểm tra đọc file tạm thời trong temp_directory_path() và dọn dẹp sau khi đọc
TEST(ExecutorTest, ReadFileContentSuccess) {
    fs::path temp_file = fs::temp_directory_path() / "test_rat_sample.txt";
    
    // Tạo nội dung file mẫu
    std::ofstream ofs(temp_file);
    ofs << "Hello Unit Test Content";
    ofs.close();

    // Đọc file
    std::string content = RAT::Executor::read_file_content(temp_file.string());
    EXPECT_EQ(content.rfind(RAT::Status::OK, 0), 0); // Bắt đầu bằng "[+] "
    EXPECT_NE(content.find("Hello Unit Test Content"), std::string::npos);

    // Dọn dẹp file tạm
    fs::remove(temp_file);
    EXPECT_FALSE(fs::exists(temp_file));
}

// Test 4: Kiểm tra đọc file không tồn tại
TEST(ExecutorTest, ReadFileContentNonExistentPath) {
    std::string result = RAT::Executor::read_file_content("/duong_dan_khong_ton_tai_12345.txt");
    EXPECT_EQ(result.rfind(RAT::Status::ERR, 0), 0);
}

// Test 5: Kiểm tra dừng tiến trình với PID không hợp lệ (số âm -999)
TEST(ExecutorTest, KillProcessInvalidPid) {
    std::string result = RAT::Executor::kill_process(-999);
    EXPECT_EQ(result.rfind(RAT::Status::ERR, 0), 0); // Báo lỗi tiền tố [!]
}

// Test 6: Kiểm tra liệt kê danh sách tiến trình trên Linux (/proc)
TEST(ExecutorTest, ListProcessesSuccess) {
    std::string result = RAT::Executor::list_processes();
    EXPECT_EQ(result.rfind(RAT::Status::OK, 0), 0);
    EXPECT_NE(result.find("PID"), std::string::npos);
    EXPECT_NE(result.find("COMMAND / NAME"), std::string::npos);
}
