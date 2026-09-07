#pragma once

#include "protocol.h"
#include "socket_utils.h"
#include <string>

namespace RAT
{
    class Server
    {
    public:
        // Khởi tạo server với port 8888
        explicit Server(int port = DEFAULT_PORT);
        ~Server();

        // Khởi tạo Socket, bind và listen
        bool start();

        // Kết nối client
        bool wait_for_client();

        // RAT shell
        void run_shell();

        // Đóng socket
        void stop();

    private:
        // Kiểm tra tính hợp lệ của command trước khi gửi qua mạng
        bool validate_and_process_command(const std::string &line, std::string &cmd_name, std::string &cmd_args);

        int server_fd_;         // Socket lắng nghe của Server
        int client_fd_;         // Socket kết nối trực tiếp với Client
        int port_;              
        std::string client_ip_; // Địa chỉ IP của Client đang kết nối
        int client_port_;       // Port của Client
        bool is_running_;       // Trạng thái hoạt động của Shell
    };
}
