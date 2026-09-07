#include "socket_utils.h"
#include "protocol.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <iostream>

namespace RAT
{
    bool send_exact(int sock, const void *data, size_t size)
    {
        // Kiểm tra socket và dữ liệu hợp lệ
        if (sock < 0 || data == nullptr)
        {
            return false;
        }

        const char *ptr = static_cast<const char *>(data);
        size_t total_sent = 0;

        // Đảm bảo dữ liệu được gửi chính xác `size` byte
        while (total_sent < size)
        {
            ssize_t bytes_sent = ::send(sock, ptr + total_sent, size - total_sent, MSG_NOSIGNAL);
            if (bytes_sent <= 0)
            {
                return false;
            }
            total_sent += static_cast<size_t>(bytes_sent);
        }
        return true;
    }

    bool recv_exact(int sock, void *data, size_t size)
    {
        // Kiểm tra socket và dữ liệu hợp lệ
        if (sock < 0 || data == nullptr)
        {
            return false;
        }

        char *ptr = static_cast<char *>(data);
        size_t total_received = 0;

        // Đảm bảo dữ liệu được nhận chính xác `size` byte
        while (total_received < size)
        {
            ssize_t bytes_received = ::recv(sock, ptr + total_received, size - total_received, 0);
            if (bytes_received <= 0)
            {
                return false;
            }
            total_received += static_cast<size_t>(bytes_received);
        }
        return true;
    }

    bool send_message(int sock, const std::string &msg)
    {
        // Kiểm tra socket
        if (sock < 0)
        {
            return false;
        }

        uint32_t len = static_cast<uint32_t>(msg.size());

        // Kiểm tra kích thước thông điệp
        if (len > MAX_MESSAGE_SIZE)
        {
            std::cerr << "[!] Error: Message size exceeds MAX_MESSAGE_SIZE (" << len << " bytes)\n";
            return false;
        }

        // Chuyển đổi độ dài thông điệp sang định dạng mạng
        uint32_t net_len = htonl(len);

        // Gửi header (độ dài thông điệp)
        if (!send_exact(sock, &net_len, sizeof(net_len)))
        {
            return false;
        }

        // Gửi nội dung tin nhắn nếu có
        if (len > 0)
        {
            if (!send_exact(sock, msg.data(), len))
            {
                return false;
            }
        }
        return true;
    }

    bool recv_message(int sock, std::string &msg)
    {
        if (sock < 0)
        {
            return false;
        }

        // Nhận header (độ dài thông điệp)
        uint32_t net_len = 0;
        if (!recv_exact(sock, &net_len, sizeof(net_len)))
        {
            return false;
        }

        // Chuyển đổi độ dài thông điệp từ định dạng mạng sang định dạng máy
        uint32_t len = ntohl(net_len);

        // Kiểm tra kích thước thông điệp
        if (len > MAX_MESSAGE_SIZE)
        {
            std::cerr << "[!] Error: Message size exceeds MAX_MESSAGE_SIZE (" << len << " bytes)\n";
            return false;
        }
        if (len == 0)
        {
            msg.clear();
            return true;
        }

        // Nhận nội dung thông điệp
        msg.resize(len);
        if (!recv_exact(sock, msg.data(), len))
        {
            msg.clear();
            return false;
        }
        return true;
    }

    // Đóng socket và đặt giá trị socket về -1
    void close_socket(int &sock)
    {
        if (sock >= 0)
        {
            ::close(sock);
            sock = -1;
        }
    }
}