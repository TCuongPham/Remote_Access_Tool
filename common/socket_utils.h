#pragma once

#include <string>
#include <cstddef>
#include <cstdint>

namespace RAT{
    //Gửi dữ liệu chính xác `size` byte qua socket
    bool send_exact(int sock, const void* data, size_t size);   
    // Nhận dữ liệu chính xác `size` byte từ socket
    bool recv_exact(int sock, void* data, size_t size);

    // Gửi một thông điệp (message) có đính kèm header qua socket
    bool send_message(int sock, const std::string& msg);
    // Nhận một thông điệp (message) có đính kèm header từ socket
    bool recv_message(int sock, std::string& msg);

    // Gửi file từ đĩa qua socket theo từng khối 64KB (Client)
    bool send_file_stream(int sock, const std::string &filepath);
    // Nhận từng khối 64KB từ socket và ghi xuống đĩa (Server)
    bool recv_file_stream(int sock, const std::string &save_path);

    // Đóng socket và đặt giá trị socket về -1
    void close_socket(int& sock);
}
