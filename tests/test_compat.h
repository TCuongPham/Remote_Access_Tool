#pragma once

#include "platform.h"

// Giả lập socketpair trên Windows bằng kết nối loopback 127.0.0.1
template <typename T>
inline bool create_test_socketpair(T sv[2])
{
// Linux    
#ifndef _WIN32
    int raw_sv[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, raw_sv) != 0) {
        return false;
    }
    sv[0] = static_cast<T>(raw_sv[0]);
    sv[1] = static_cast<T>(raw_sv[1]);
    return true;

// Windows
#else
    RAT::init_networking();
    SOCKET listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // Cổng ngẫu nhiên trống

    if (::bind(listener, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        ::closesocket(listener);
        return false;
    }
    int len = sizeof(addr);
    if (::getsockname(listener, reinterpret_cast<struct sockaddr*>(&addr), &len) == SOCKET_ERROR) {
        ::closesocket(listener);
        return false;
    }
    if (::listen(listener, 1) == SOCKET_ERROR) {
        ::closesocket(listener);
        return false;
    }

    SOCKET client = ::socket(AF_INET, SOCK_STREAM, 0);
    if (client == INVALID_SOCKET) {
        ::closesocket(listener);
        return false;
    }
    if (::connect(client, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        ::closesocket(listener);
        ::closesocket(client);
        return false;
    }
    SOCKET server = ::accept(listener, NULL, NULL);
    ::closesocket(listener);
    if (server == INVALID_SOCKET) {
        ::closesocket(client);
        return false;
    }
    sv[0] = static_cast<T>(client);
    sv[1] = static_cast<T>(server);
    return true;
#endif
}