# HƯỚNG DẪN TỪNG BƯỚC XÂY DỰNG REMOTE ADMINISTRATION TOOL (RAT) BẰNG C++ TRÊN LINUX

Tài liệu này hướng dẫn chi tiết từ kiến thức nền tảng, thiết kế kiến trúc, cây thư mục cho đến từng bước thực hiện xây dựng một công cụ quản trị từ xa (Remote Administration Tool) đơn giản, hoạt động trên Linux sử dụng C++ và các thư viện chuẩn/POSIX APIs.

---

## 1. Mục tiêu và Phạm vi Dự án

* **Mục tiêu:** Xây dựng mô hình Client - Server cơ bản bằng C++ trên Linux.
* **Server (Controller):** Đóng vai trò máy điều khiển, chạy giao diện dòng lệnh (CLI), gửi lệnh cho Client và hiển thị kết quả.
* **Client (Agent):** Chạy trên máy đích (chạy nền), lắng nghe lệnh từ Server, thực thi các tác vụ hệ thống và gửi kết quả trả về.
* **Các chức năng chính:**
  1. Liệt kê file và thư mục (`LIST_DIR <path>`)
  2. Đọc nội dung file (`READ_FILE <path>`)
  3. Liệt kê các tiến trình đang chạy (`LIST_PROC`)
  4. Dừng/Tắt tiến trình theo PID (`KILL_PROC <pid>`)

---

## 2. Kiến thức Nền tảng Cần Dùng

Để hoàn thành dự án này bằng C++ trên Linux, bạn cần nắm vững 4 nhóm kiến thức cơ bản sau:

### 2.1. Lập trình Mạng Socket TCP trên Linux (POSIX Sockets)
* **Khái niệm Socket:** Kênh giao tiếp hai chiều giữa 2 tiến trình thông qua mạng IP/Port.
* **Các thư viện chuẩn POSIX:**
  * `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`, `<unistd.h>`
* **Luồng hoạt động của Server:**
  * `socket()`: Tạo socket endpoint.
  * `bind()`: Gán socket với một địa chỉ IP và số Port cụ thể.
  * `listen()`: Đặt socket vào trạng thái lắng nghe kết nối đến.
  * `accept()`: Chấp nhận một kết nối mới từ Client (hàm này sẽ block cho đến khi có Client kết nối).
  * `send()` / `recv()`: Gửi và nhận dữ liệu byte stream.
  * `close()`: Đóng socket khi hoàn tất.
* **Luồng hoạt động của Client:**
  * `socket()`: Tạo socket.
  * `connect()`: Kết nối chủ động tới IP và Port của Server.
  * `recv()` / `send()`: Nhận lệnh và trả lời kết quả.

### 2.2. Giao thức Trao đổi Dữ liệu (Message Framing Protocol)
* TCP là luồng byte liên tục (stream-based), không có khái niệm "kết thúc một tin nhắn".
* **Giải pháp đơn giản:** Đính kèm độ dài thông điệp ở đầu gói tin (Length-prefixed message):
  * Cấu trúc gói tin: `[4 bytes: Độ dài dữ liệu (uint32_t)] + [N bytes: Nội dung văn bản]`.
  * Nhờ vậy, cả Server và Client luôn biết chính xác cần đọc bao nhiêu bytes từ socket để nhận trọn vẹn một thông điệp mà không bị cắt vụn hay dính lệnh.

### 2.3. Tương tác Hệ thống File (Filesystem in C++17)
* Thư viện chuẩn C++17: `<filesystem>` (namespace `std::filesystem`).
* **Duyệt thư mục:** Sử dụng `std::filesystem::directory_iterator` để quét file và thư mục con một cách an toàn, lấy được kích thước, định dạng và thời gian sửa đổi.
* **Đọc file:** Sử dụng `<fstream>` (`std::ifstream`) để đọc luồng byte hoặc văn bản từ file.

### 2.4. Quản lý Tiến trình trên Linux (Process Management)
* **Hệ thống file ảo `/proc`:**
  * Trong Linux, mọi thông tin về tiến trình đang chạy đều được nhân (Kernel) phản ánh qua thư mục `/proc/[PID]/`.
  * Liệt kê tiến trình: Quét các thư mục có tên là chữ số bên trong `/proc`, đọc file `/proc/[PID]/comm` hoặc `/proc/[PID]/cmdline` để lấy tên chương trình.
* **Kill tiến trình:**
  * Thư viện `<signal.h>`.
  * Sử dụng hàm `kill(pid_t pid, int sig)` (ví dụ `sig = SIGTERM` hoặc `sig = SIGKILL`).

---

## 3. Cấu trúc Cây Thư mục Dự án

```text
RAT/
├── CMakeLists.txt              # File cấu hình build tự động (CMake)
├── Makefile                    # Hoặc file Makefile truyền thống
├── README.md                   # Giới thiệu dự án
├── HUONG_DAN_DU_AN.md          # Tài liệu hướng dẫn này
│
├── common/                     # Thư viện và định nghĩa dùng chung giữa Server và Client
│   ├── protocol.h              # Định nghĩa cấu trúc gói tin, mã lệnh (Command Type)
│   ├── socket_utils.h          # Hàm hỗ trợ gửi/nhận dữ liệu nguyên vẹn (send_all, recv_all)
│   └── socket_utils.cpp
│
├── server/                     # Mã nguồn phía Server (Controller)
│   ├── server.h                # Khai báo lớp Server quản lý kết nối và CLI
│   ├── server.cpp              # Xử lý logic mạng của Server
│   └── main_server.cpp         # Điểm khởi chạy của Server
│
└── client/                     # Mã nguồn phía Client (Agent trên máy đích)
    ├── client.h                # Khai báo lớp Client
    ├── client.cpp              # Xử lý kết nối, nhận lệnh và điều phối
    ├── executor.h              # Khai báo các hàm thực thi lệnh OS
    ├── executor.cpp            # Cài đặt: list_dir, read_file, list_proc, kill_proc
    └── main_client.cpp         # Điểm khởi chạy của Client
```

---

## 4. Thiết kế Giao thức Lệnh (Command Protocol)

Để Server và Client hiểu nhau, chúng ta quy ước các định dạng lệnh dạng chuỗi văn bản (hoặc mã hóa nhị phân):

| Lệnh gửi từ Server | Tham số | Mô tả hành động Client thực hiện | Kết quả trả về |
| :--- | :--- | :--- | :--- |
| `LIST_DIR` | `<đường_dẫn>` | Duyệt danh sách file/thư mục tại vị trí chỉ định | Chuỗi danh sách file kèm loại (File/Dir) và kích thước |
| `READ_FILE` | `<đường_dẫn>` | Đọc toàn bộ nội dung file | Nội dung văn bản của file (hoặc thông báo lỗi nếu không có quyền) |
| `LIST_PROC` | *(không có)* | Đọc danh sách PID và tên tiến trình từ `/proc` | Danh sách dạng bảng: `PID | Name` |
| `KILL_PROC` | `<pid>` | Gửi tín hiệu `SIGKILL` tới PID chỉ định | Thông báo thành công hoặc mã lỗi |
| `HELP` | *(không có)* | In ra danh sách các lệnh hỗ trợ | Cẩm nang lệnh hỗ trợ |
| `EXIT` | *(không có)* | Yêu cầu đóng phiên làm việc | Xác nhận ngắt kết nối |

---

## 5. Quy trình 6 Bước Triển khai Dự án

### Bước 1: Chuẩn bị Môi trường trên Linux
Mở terminal trên máy ảo Linux và cài đặt bộ công cụ biên dịch C++ chuẩn:
```bash
sudo apt update
sudo apt install -y build-essential cmake gdb
```
Kiểm tra phiên bản `g++` hỗ trợ chuẩn C++17:
```bash
g++ --version
```

---

### Bước 2: Xây dựng Module Mạng Dùng Chung (`common/`)
1. **File `protocol.h`:**
   * Quy ước port mặc định (ví dụ: `8888`).
   * Quy ước kích thước header (4 bytes độ dài).
2. **File `socket_utils.cpp`:**
   * Cài đặt hàm `send_exact(int sock, const void* data, size_t size)`: Đảm bảo gửi đủ số bytes, lặp lại nếu mạng bị nghẽn.
   * Cài đặt hàm `recv_exact(int sock, void* data, size_t size)`: Đọc đủ số bytes trước khi trả về.
   * Cài đặt hàm gửi chuỗi: `send_message(int sock, const std::string& msg)`.
   * Cài đặt hàm nhận chuỗi: `recv_message(int sock, std::string& msg)`.

---

### Bước 3: Cài đặt Module Thực thi Hệ thống cho Client (`client/executor.cpp`)
Đây là trái tim của Client, phụ trách 4 tác vụ chính:

1. **Hàm `list_directory(const std::string& path)`:**
   * Sử dụng `std::filesystem::directory_iterator(path)`.
   * Kiểm tra `entry.is_directory()`, `entry.file_size()`, `entry.path().filename()`.
   * Xử lý ngoại lệ `std::filesystem::filesystem_error` khi không có quyền truy cập.

2. **Hàm `read_file_content(const std::string& filepath)`:**
   * Mở file bằng `std::ifstream file(filepath, std::ios::in | std::ios::binary)`.
   * Đọc nội dung vào chuỗi `std::string` (giới hạn dung lượng tối đa khoảng 1-2 MB cho mỗi lần đọc để tránh tràn bộ nhớ).

3. **Hàm `list_processes()`:**
   * Duyệt thư mục `/proc`.
   * Lọc ra các thư mục có tên chỉ chứa ký tự số (đó chính là các PID).
   * Mở file `/proc/[pid]/comm` để đọc tên ngắn gọn của tiến trình.
   * Ghép lại thành danh sách dạng: `[PID] Tên_tiến_trình`.

4. **Hàm `kill_process(int pid)`:**
   * Chuyển đổi tham số sang số nguyên.
   * Gọi hàm POSIX: `kill(pid, SIGKILL)`.
   * Kiểm tra giá trị trả về: nếu bằng 0 là thành công; nếu `-1` thì kiểm tra `errno` (ví dụ `EPERM` là thiếu quyền, `ESRCH` là không tìm thấy PID).

---

### Bước 4: Xây dựng Vòng lặp Chính của Client (`client/main_client.cpp`)
1. Tạo socket `socket(AF_INET, SOCK_STREAM, 0)`.
2. Khởi tạo địa chỉ IP Server (`127.0.0.1`) và Port (`8888`).
3. Gọi `connect()`. Nếu chưa kết nối được, ngủ 3 giây rồi thử lại (tính năng tự động kết nối lại - auto reconnect).
4. Khi đã kết nối:
   * Chờ `recv_message()` nhận chuỗi lệnh từ Server.
   * Tách từ đầu tiên của chuỗi làm tên lệnh (command parser).
   * Gọi hàm tương ứng trong `executor.cpp`.
   * Dùng `send_message()` gửi toàn bộ kết quả ngược lại cho Server.

---

### Bước 5: Xây dựng Giao diện Điều khiển của Server (`server/main_server.cpp`)
1. Tạo socket, gán địa chỉ `INADDR_ANY` và port `8888`.
2. Gọi `bind()` và `listen()`.
3. In ra thông báo: `[+] Server dang lang nghe tren port 8888...`.
4. Gọi `accept()` và in ra thông tin IP máy Client vừa kết nối.
5. Tạo vòng lặp điều khiển dòng lệnh:
   * In dấu nhắc: `RAT-Shell> `.
   * Nhập lệnh từ bàn phím (`std::getline(std::cin, cmd)`).
   * Kiểm tra tính hợp lệ của lệnh trước khi gửi.
   * Gửi lệnh qua `send_message()`.
   * Nhận kết quả từ `recv_message()` và in ra màn hình.

---

### Bước 6: Biên dịch và Thử nghiệm Thực tế

#### Biên dịch bằng `g++` trực tiếp:
```bash
# Biên dịch Server
g++ -std=c++17 -Wall -I./common common/socket_utils.cpp server/server.cpp server/main_server.cpp -o bin_server

# Biên dịch Client
g++ -std=c++17 -Wall -I./common common/socket_utils.cpp client/executor.cpp client/client.cpp client/main_client.cpp -o bin_client
```

#### Chạy thử nghiệm trên 2 Terminal song song:
1. **Terminal 1:**
   ```bash
   ./bin_server
   ```
2. **Terminal 2:**
   ```bash
   ./bin_client
   ```
3. **Thao tác mẫu trên Terminal 1:**
   ```text
   [+] Co ket noi tu Client: 127.0.0.1
   RAT-Shell> LIST_DIR /home
   [Kết quả in ra danh sách thư mục /home]

   RAT-Shell> LIST_PROC
   [Kết quả in ra danh sách các PID và tên tiến trình]

   RAT-Shell> KILL_PROC 1234
   [+] Da gui tin hieu kill toi process 1234
   ```

---

## 6. Các Điểm Cần Lưu Ý về Kỹ thuật & Tối ưu

1. **Tránh treo (Deadlock / Hang):**
   * Luôn kiểm tra giá trị trả về của `send` và `recv`. Nếu hàm trả về `<= 0`, tức là kết nối đã bị đứt, chương trình cần đóng socket và thoát hoặc chuyển sang chế độ đợi kết nối lại.
2. **Xử lý đường dẫn tương đối và tuyệt đối:**
   * Khi gọi `LIST_DIR` hoặc `READ_FILE`, nên dùng `std::filesystem::canonical` hoặc kiểm tra đường dẫn tồn tại trước khi mở để tránh crash chương trình.
3. **An toàn bộ nhớ:**
   * Tận dụng tối đa các lớp chuẩn của C++ như `std::string`, `std::vector`, `std::unique_ptr` để tránh rò rỉ bộ nhớ (memory leaks) hay lỗi phân đoạn (segmentation fault).
