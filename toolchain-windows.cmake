# Tên hệ điều hành
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Chỉ định trình biên dịch C và C++ của MinGW 64-bit
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)

# Thư mục gốc chứa header và thư viện Windows trên Linux
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Cấu hình tìm kiếm gói: chỉ tìm thư viện cho Windows
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# LIÊN KẾT TĨNH (STATIC LINKING):
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++" CACHE STRING "" FORCE)