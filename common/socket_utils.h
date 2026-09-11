#pragma once

#include "platform.h"

#include <string>
#include <cstddef>
#include <cstdint>

namespace RAT{
    //Gửi dữ liệu chính xác `size` byte qua socket
    bool send_exact(socket_t sock, const void* data, size_t size);   
    // Nhận dữ liệu chính xác `size` byte từ socket
    bool recv_exact(socket_t sock, void* data, size_t size);

    // Gửi một thông điệp (message) có đính kèm header qua socket
    bool send_message(socket_t sock, const std::string& msg);
    // Nhận một thông điệp (message) có đính kèm header từ socket
    bool recv_message(socket_t sock, std::string& msg);

    // Gửi file từ đĩa qua socket theo từng khối 64KB (Client)
    bool send_file_stream(socket_t sock, const std::string &filepath);
    // Nhận từng khối 64KB từ socket và ghi xuống đĩa (Server)
    bool recv_file_stream(socket_t sock, const std::string &save_path);

    // Đóng socket và đặt giá trị socket về -1
    void close_socket(socket_t& sock);
}
