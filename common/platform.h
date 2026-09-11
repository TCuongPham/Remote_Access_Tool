#pragma once

#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

// Kiểu dữ liệu socket trên Windows
using socket_t = SOCKET;
constexpr socket_t INVALID_SOCKET_VAL = INVALID_SOCKET;
constexpr int SOCKET_ERROR_VAL = SOCKET_ERROR;

// Định nghĩa CLOSE_SOCKET dùng chung
#define CLOSE_SOCKET(s) ::closesocket(s)

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// Hàm Polling (Kiểm tra trạng thái nhiều socket cùng lúc)
#define POLL_STRUCT WSAPOLLFD
#define POLL_FUNC WSAPoll

// Endianness 64-bit trên Windows (dùng htonll/ntohll của winsock2.h)
inline uint64_t rat_htobe64(uint64_t val) { return htonll(val); }
inline uint64_t rat_be64toh(uint64_t val) { return ntohll(val); }

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <endian.h>

// Kiểu dữ liệu socket trên Linux
using socket_t = int;
constexpr socket_t INVALID_SOCKET_VAL = -1;
constexpr int SOCKET_ERROR_VAL = -1;

// Định nghĩa CLOSE_SOCKET dùng chung
#define CLOSE_SOCKET(s) ::close(s)

// Hàm Polling (Kiểm tra trạng thái nhiều socket cùng lúc)
#define POLL_STRUCT struct pollfd
#define POLL_FUNC ::poll

// Endianness 64-bit trên Linux
inline uint64_t rat_htobe64(uint64_t val) { return htobe64(val); }
inline uint64_t rat_be64toh(uint64_t val) { return be64toh(val); }
#endif

namespace RAT
{
    // Khởi tạo mạng
    inline bool init_networking()
    {
#ifdef _WIN32
        WSADATA wsaData;
        return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
#else
        return true;
#endif
    }

    // Dọn dẹp mạng khi kết thúc
    inline void cleanup_networking()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }
}