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
    Server::Server(int port)
        : server_fd_(-1), client_fd_(-1), port_(port), client_port_(0), is_running_(false) {}
    Server::~Server()
    {
        stop();
    }

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
    }

}