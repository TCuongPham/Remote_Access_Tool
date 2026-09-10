#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>

namespace RAT
{
    // Thông tin của 1 client đang connect
    struct ClientSession
    {
        int id;         // ID
        int socket_fd;  // Socket kết nối với Client này
        std::string ip; // Địa chỉ IP của Client
        int port;       // Cổng kết nối của Client
        std::chrono::system_clock::time_point connect_time;
        std::mutex socket_mtx; // Đảm bảo an toàn khi gửi/nhận đồng thời trên socket này
    };

    // Quản lý phiên danh sách các client đang kết nối
    class SessionManager
    {
    public:
        SessionManager() = default;
        ~SessionManager();

        // Thêm một client mới, trả về Session ID
        int add_session(int sock, const std::string &ip, int port);

        // Xóa một client theo ID (và đóng socket)
        void remove_session(int id);

        // Lấy thông tin session theo ID
        std::shared_ptr<ClientSession> get_session(int id);

        // Lấy danh sách toàn bộ các session đang online
        std::unordered_map<int, std::shared_ptr<ClientSession>> get_all_sessions();

        // Đếm số lượng client đang online
        size_t count();

        // Đóng toàn bộ các kết nối
        void close_all();

    private:
        // Danh sách các session đang online
        std::unordered_map<int, std::shared_ptr<ClientSession>> sessions_;

        std::mutex map_mtx_; // Khóa bảo vệ danh sách sessions_
    };
}