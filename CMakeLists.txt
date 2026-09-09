cmake_minimum_required(VERSION 3.16)
project(RAT CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Bật cảnh báo trình biên dịch để rà soát lỗi code
add_compile_options(-Wall -Wextra)

# Tìm kiếm thư viện luồng (pthread) cần cho std::this_thread::sleep_for ở Client
find_package(Threads REQUIRED)

# 1. TẠO THƯ VIỆN DÙNG CHUNG (rat_common)
add_library(rat_common STATIC
    common/socket_utils.cpp
)
# Công khai thư mục common để Server và Client tự động include được
target_include_directories(rat_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# 2. BIÊN DỊCH SERVER (bin_server)
add_executable(bin_server
    server/server.cpp
    server/main_server.cpp
)
target_include_directories(bin_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/server
)
target_link_libraries(bin_server PRIVATE
    rat_common
)

# 3. BIÊN DỊCH CLIENT (bin_client)
add_executable(bin_client
    client/executor.cpp
    client/client.cpp
    client/main_client.cpp
)
target_include_directories(bin_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/client
)
target_link_libraries(bin_client PRIVATE
    rat_common
    Threads::Threads
)