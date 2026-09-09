#include "executor.h"
#include "protocol.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Test 1: Kiểm tra list_directory với đường dẫn không tồn tại -> phải báo lỗi
TEST(ExecutorTest, ListDirectoryNonExistentPath) {
    std::string result = RAT::Executor::list_directory("/duong_dan_khong_ton_tai");
    EXPECT_NE(result.find(RAT::Status::ERR), std::string::npos);
}

// Test 2: Kiểm tra đọc file 
TEST(ExecutorTest, ReadFileContentSuccess) {
    fs::path temp_file = fs::temp_directory_path() / "test_rat_sample.txt";
    
    // Tạo nd file mẫu
    std::ofstream ofs(temp_file);
    ofs << "Hello Unit Test Content";
    ofs.close();

    // Đọc file
    std::string content = RAT::Executor::read_file_content(temp_file.string());
    EXPECT_NE(content.find("Hello Unit Test Content"), std::string::npos);
    // Dọn dẹp file tạm

    fs::remove(temp_file);
}

// Test 3: Kiểm tra dừng tiến trình với PID không hợp lệ
TEST(ExecutorTest, KillProcessInvalidPid) {
    std::string result = RAT::Executor::kill_process(-999);
    EXPECT_NE(result.find(RAT::Status::ERR), std::string::npos);
}

