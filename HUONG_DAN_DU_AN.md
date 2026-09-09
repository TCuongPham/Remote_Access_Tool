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

---

# PHẦN II: HƯỚNG DẪN NÂNG CẤP HỆ THỐNG NÂNG CAO
*(Truyền File Kích Thước Lớn - Quản Lý Đa Client & Broadcast - Tích Hợp Unit Test)*

---

## 7. Các Kiến Thức Nền Tảng Cần Có Cho Phần Nâng Cấp

Để mở rộng hệ thống đáp ứng 3 yêu cầu nâng cao, bạn cần trang bị thêm 3 nhóm kiến thức chuyên sâu sau:

### 7.1. Đa Luồng và Đồng Bộ Hóa trong C++ (Multithreading & Synchronization)
* **Khái niệm Luồng (`std::thread`):** 
  * Cho phép chương trình thực thi nhiều luồng công việc đồng thời trong cùng một tiến trình.
  * Trong Server RAT, cần tách biệt **Luồng lắng nghe kết nối (Acceptor Thread)** chạy ngầm liên tục và **Luồng giao diện điều khiển (CLI Thread)** nhận lệnh từ quản trị viên.
* **Tranh chấp dữ liệu (Race Condition) và Khóa tương hỗ (`std::mutex`):**
  * Khi luồng Acceptor thêm client mới và luồng CLI duyệt danh sách client để gửi lệnh, việc truy cập đồng thời vào tài nguyên dùng chung (danh sách phiên kết nối) sẽ gây lỗi bộ nhớ hoặc làm sập ứng dụng.
  * Áp dụng nguyên tắc quản lý tài nguyên RAII với `std::lock_guard<std::mutex>` để tự động khóa và giải phóng tài nguyên an toàn kể cả khi có ngoại lệ xảy ra.
* **Thực thi Bất đồng bộ (`std::async`, `std::future`):**
  * Giải pháp gửi lệnh phát sóng đồng thời (`BROADCAST`) tới toàn bộ máy đích song song mà không bị nghẽn Server khi một máy bất kỳ phản hồi chậm hoặc đứt mạng.

### 7.2. Giao Thức Phân Mảnh Truyền File Lớn (Chunked Streaming Protocol)
* **Hạn chế của phương pháp đọc toàn bộ file:** Đọc toàn bộ nội dung file vào bộ nhớ RAM (`std::string`) sẽ gây tràn bộ nhớ (Out Of Memory) khi gặp file hàng trăm MB hoặc GB, đồng thời dễ làm hỏng dữ liệu của các file nhị phân (`.zip`, `.iso`, `.png`, `.bin`).
* **Nguyên lý Stream I/O với bộ đệm cố định ($O(1)$ RAM):**
  * Giữ mức tiêu thụ RAM cố định ở một kích thước nhỏ (ví dụ 64 KB = 65,536 bytes).
  * Phía gửi đọc file tuần tự từng khối bằng luồng nhị phân `std::ifstream` và gửi ngay qua mạng.
  * Phía nhận tiếp nhận từng khối và ghi trực tiếp xuống ổ cứng qua luồng nhị phân `std::ofstream`.
  * Bộ nhớ RAM luôn giữ ổn định ở mức vài chục KB bất kể dung lượng file lớn bao nhiêu.
* **Cơ chế Bắt tay (Metadata Handshake) và Báo hiệu Hoàn tất (EOF):**
  * Trước khi gửi dữ liệu, hai bên trao đổi gói thông tin ban đầu gồm kích thước file thực tế (chuẩn hóa dạng 64-bit Big-Endian) và mã trạng thái file.
  * Dữ liệu truyền tải được phân mảnh thành các khối có độ dài xác định. Khối dữ liệu có độ dài bằng 0 đóng vai trò là điểm kết thúc truyền file.

### 7.3. Kiểm Thử Tự Động Với Google Test và Kỹ Thuật Mocking Socket (Unit Testing)
* **Khung kiểm thử Google Test (GTest) & CTest:**
  * Khung kiểm thử đơn vị tiêu chuẩn trong C++, cung cấp các macro kiểm tra điều kiện (`EXPECT_EQ`, `ASSERT_TRUE`,...).
  * Tích hợp trực tiếp qua module `FetchContent` của CMake giúp tự động tải và biên dịch bộ thư viện kiểm thử mà không yêu cầu cài đặt thêm gói ngoài hệ điều hành.
* **Mô phỏng mạng bằng `socketpair()` của POSIX Linux:**
  * Tạo cặp socket kết nối trực tiếp trong không gian bộ nhớ của hệ điều hành.
  * Cho phép kiểm thử toàn bộ các hàm truyền nhận mạng một cách độc lập, tin cậy, không cần mở cổng mạng thật, tránh xung đột cổng và chạy kiểm thử tức thì.
* **Kiểm thử Hệ thống File trong Môi trường Cô lập:**
  * Sử dụng thư mục tạm của hệ thống (`std::filesystem::temp_directory_path`) để tạo các tệp thử nghiệm, kiểm tra các trường hợp đọc tệp, duyệt thư mục và tự động thu dọn sau khi kết thúc.

---

## 8. Mở Rộng Cấu Trúc Cây Thư Mục & Giao Thức Nâng Cao

### 8.1. Cấu trúc Cây Thư mục Mở rộng
```text
RAT/
├── CMakeLists.txt              # Cập nhật: Tích hợp FetchContent GTest & CTest
├── HUONG_DAN_DU_AN.md          # Tài liệu hướng dẫn hoàn chỉnh
│
├── common/                     # Thư viện dùng chung
│   ├── protocol.h              # Bổ sung mã lệnh DOWNLOAD_FILE, định nghĩa kích thước chunk
│   ├── socket_utils.h          # Bổ sung khai báo hàm stream file (send/recv_file_stream)
│   └── socket_utils.cpp        # Cài đặt logic truyền nhận file theo khối
│
├── server/                     # Mã nguồn Server
│   ├── session_manager.h       # Khai báo lớp quản lý phiên đa luồng
│   ├── session_manager.cpp     # Cài đặt danh sách phiên an toàn luồng với mutex
│   ├── server.h                # Cập nhật: Kiến trúc tách biệt Acceptor Thread và CLI Thread
│   ├── server.cpp              # Cài đặt quản lý phiên, lệnh tương tác và broadcast
│   └── main_server.cpp         # Điểm khởi chạy Server
│
├── client/                     # Mã nguồn Client
│   ├── executor.h              # Khai báo các hàm thực thi hệ thống
│   ├── executor.cpp            # Cài đặt xử lý stream dữ liệu file
│   ├── client.h                # Cập nhật xử lý lệnh tải file
│   ├── client.cpp              # Tiếp nhận và điều phối lệnh tải file
│   └── main_client.cpp         # Điểm khởi chạy Client
│
└── tests/                      # Thư mục kiểm thử tự động
    ├── test_main.cpp           # Điểm khởi chạy bộ kiểm thử Google Test
    ├── test_protocol.cpp       # Kiểm thử định dạng gói tin và endian
    ├── test_socket_utils.cpp   # Kiểm thử truyền nhận qua socketpair
    └── test_executor.cpp       # Kiểm thử logic hệ thống và tệp tin
```

### 8.2. Thiết Kế Giao Thức Lệnh Mới

| Lệnh tại Shell Server | Tham số | Mô tả hành động thực hiện | Kết quả trả về |
| :--- | :--- | :--- | :--- |
| `SESSIONS` | *(không có)* | Liệt kê danh sách tất cả Client đang kết nối trực tuyến | Bảng danh sách gồm Session ID, IP, Port và trạng thái |
| `INTERACT` | `<session_id>` | Chuyển Server sang chế độ tương tác 1-1 trực tiếp với Client chỉ định | Chuyển đổi dấu nhắc sang shell của riêng Client đó |
| `BACK` | *(không có)* | Rời khỏi chế độ tương tác 1-1 với một Client để quay về Menu chính | Quay lại dấu nhắc quản trị `RAT-Manager>` |
| `BROADCAST` | `<command>` | Phát sóng một lệnh tới tất cả Client đang trực tuyến cùng lúc | Tổng hợp và hiển thị kết quả từ toàn bộ Client theo ID |
| `DOWNLOAD_FILE` | `<remote_path> <local_path>` | Tải file kích thước lớn từ máy Client về lưu trên máy Server | Quá trình truyền dữ liệu theo khối kèm tiến độ % |

---

## 9. Hướng Dẫn Chi Tiết Triển Khai Từng Hạng Mục

---

### HẠNG MỤC 1: TÍCH HỢP UNIT TEST (GOOGLE TEST & CTEST)

Đây là bước nền tảng cần làm trước tiên để tạo lưới an toàn cho mã nguồn hiện có, đảm bảo các tính năng cốt lõi không bị lỗi phát sinh khi mở rộng hệ thống.

#### Bước 1.1: Cấu hình `CMakeLists.txt` tự động tích hợp Google Test
* Sử dụng module `FetchContent` của CMake khai báo tải mã nguồn Google Test phiên bản ổn định (v1.14.0) từ kho GitHub.
* Thiết lập cờ `gtest_force_shared_crt` và gọi `FetchContent_MakeAvailable(googletest)` để biên dịch GTest cùng dự án mà không cần cài đặt gói thư viện bên ngoài.
* Bật tính năng kiểm thử của CMake bằng lệnh `enable_testing()`.
* Tạo target thực thi mới cho kiểm thử (ví dụ: `rat_unit_tests`), bao gồm các file kiểm thử trong thư mục `tests/` cùng các module cần kiểm tra.
* Liên kết target này với thư viện dùng chung `rat_common`, `GTest::gtest` và thư viện luồng `Threads::Threads`.
* Khai báo `include(GoogleTest)` và gọi `gtest_discover_tests(rat_unit_tests)` để tự động đăng ký các ca kiểm thử vào hệ thống CTest.

#### Bước 1.2: Cài đặt điểm chạy kiểm thử (`tests/test_main.cpp`)
* Khởi tạo framework Google Test thông qua hàm `::testing::InitGoogleTest`.
* Gọi `RUN_ALL_TESTS()` để kích hoạt việc chạy toàn bộ các ca kiểm thử đã đăng ký và trả về mã lỗi tổng thể.

#### Bước 1.3: Cài đặt kiểm thử Socket với Virtual Socket (`tests/test_socket_utils.cpp`)
* **Nguyên lý kiểm thử:** Sử dụng hàm POSIX `socketpair(AF_UNIX, SOCK_STREAM, 0, sv)` tạo ra 2 socket nối thẳng với nhau trong kernel để kiểm thử toàn diện các hàm mạng mà không cần kết nối qua card mạng thật.
* **Cấu hình Fixture (`SetUp` và `TearDown`):**
  * Hàm `SetUp`: Tạo cặp socket liên kết trực tiếp trước mỗi bài test.
  * Hàm `TearDown`: Đóng an toàn cả 2 socket sau khi bài test hoàn tất.
* **Các ca kiểm thử cần có:**
  * *Ca 1 - Gửi và nhận tin nhắn văn bản thông thường:* Một đầu gọi `send_message`, đầu kia gọi `recv_message` và đối chiếu chuỗi nhận được phải trùng khớp 100% với chuỗi ban đầu.
  * *Ca 2 - Gửi và nhận chuỗi rỗng:* Kiểm tra hệ thống xử lý gói tin có độ dài bằng 0 một cách an toàn mà không gây treo kết nối.
  * *Ca 3 - Tính toàn vẹn của phân mảnh:* Kiểm tra hàm `send_exact` và `recv_exact` nhận đầy đủ số bytes đã yêu cầu.

#### Bước 1.4: Cài đặt kiểm thử Thực thi Hệ thống (`tests/test_executor.cpp`)
* **Kiểm thử duyệt thư mục (`list_directory`):**
  * Kiểm thử với một đường dẫn không tồn tại: Đảm bảo kết quả trả về bắt đầu bằng tiền tố báo lỗi `[!]`.
  * Kiểm thử với thư mục hiện tại: Đảm bảo kết quả trả về bắt đầu bằng tiền tố thành công `[+]` và chứa thông tin các file.
* **Kiểm thử đọc file (`read_file_content`):**
  * Tạo một file thử nghiệm tạm thời trong thư mục `temp_directory_path()` của hệ điều hành, ghi vào một chuỗi dữ liệu mẫu.
  * Gọi hàm đọc file và kiểm tra nội dung trả về có chứa chuỗi mẫu hay không.
  * Tự động xóa file tạm sau khi kiểm thử kết thúc.
* **Kiểm thử dừng tiến trình (`kill_process`):**
  * Kiểm thử với PID không hợp lệ (ví dụ: số âm `-999`): Xác nhận hệ thống trả về thông báo lỗi thay vì gửi tín hiệu sai.

* Chạy test: ctest --output-on-failure
---

### HẠNG MỤC 2: TRUYỀN FILE KÍCH THƯỚC LỚN (CHUNK-BASED STREAMING)

#### Bước 2.1: Quy ước Giao thức Stream File trong `common/protocol.h`
* Khai báo mã lệnh mới `DOWNLOAD_FILE` dành cho tác vụ tải file.
* Xác định kích thước khối cố định `FILE_CHUNK_SIZE` là 64 KB (65,536 bytes) - mức kích thước cân bằng tối ưu giữa lưu lượng TCP và bộ nhớ cache CPU.
* Định nghĩa cấu trúc Header bắt tay ban đầu (`FileTransferHeader`):
  * `status_code` (1 byte): Mã phản hồi từ Client (0: Sẵn sàng gửi, 1: File không tồn tại, 2: Không có quyền đọc).
  * `file_size` (8 bytes - số nguyên không dấu 64-bit `uint64_t`): Kích thước thực tế của file, đảm bảo hỗ trợ các file dung lượng trên 4GB. Sử dụng hàm chuyển đổi chuẩn mạng `htobe64` và `be64toh` để tương thích giữa các kiến trúc vi xử lý khác nhau.

#### Bước 2.2: Xây dựng hàm gửi file theo luồng phía Client (`send_file_stream`)
1. **Kiểm tra file:** Kiểm tra đường dẫn có tồn tại và có phải là tệp tin thông thường không. Nếu không hợp lệ, gửi gói Header chứa mã lỗi tương ứng về Server rồi dừng lại.
2. **Gửi thông tin ban đầu:** Lấy kích thước tệp tin thực tế, chuẩn hóa sang dạng Big-Endian và gửi gói Header bắt tay qua socket bằng `send_exact`.
3. **Đọc và phân mảnh dữ liệu:**
   * Mở file ở chế độ nhị phân (`std::ios::binary`).
   * Sử dụng một mảng bộ đệm 64 KB trên stack.
   * Chạy vòng lặp đọc từng khối dữ liệu từ file qua `file.read()`.
   * Lấy số byte thực tế vừa đọc được thông qua `file.gcount()`.
   * Gửi 4 bytes độ dài của khối hiện tại (đã chuyển `htonl`), theo sau là toàn bộ khối dữ liệu thực tế bằng `send_exact`.
4. **Báo hiệu kết thúc (EOF Marker):** Sau khi đọc hết nội dung file, gửi một giá trị độ dài bằng 0 (4 bytes `0`) để báo cho Server biết quá trình truyền tải đã kết thúc.

#### Bước 2.3: Xây dựng hàm nhận file và ghi trực tiếp phía Server (`recv_file_stream`)
1. **Nhận Header bắt tay:** Nhận gói Header ban đầu từ Client bằng `recv_exact`. Nếu mã trạng thái khác 0, thông báo lỗi cho người dùng và dừng lại.
2. **Khởi tạo file ghi:** Chuyển đổi kích thước file sang định dạng của máy và mở file tại đường dẫn lưu trữ trên Server ở chế độ nhị phân để ghi trực tiếp xuống ổ cứng.
3. **Vòng lặp tiếp nhận khối:**
   * Nhận 4 bytes độ dài khối tiếp theo và chuyển đổi bằng `ntohl`.
   * Nếu độ dài bằng 0, nhận biết đây là điểm kết thúc tệp và thoát khỏi vòng lặp.
   * Nếu độ dài lớn hơn 0, tiếp tục gọi `recv_exact` để nhận đủ số bytes dữ liệu và dùng `outfile.write()` ghi ngay vào file trên ổ đĩa.
   * Tích lũy số byte đã nhận, tính toán tỷ lệ phần trăm so với tổng kích thước file và cập nhật tiến độ tải trực tiếp trên cùng một dòng terminal.
4. **Đóng file và xác nhận:** Đóng luồng file và in thông báo tải hoàn tất kèm đường dẫn file đã lưu.

---

### HẠNG MỤC 3: SERVER ĐIỀU KHIỂN ĐA CLIENT & BROADCAST

#### Bước 3.1: Xây dựng Lớp Quản lý Phiên (`server/session_manager.h` & `session_manager.cpp`)
* **Cấu trúc `ClientSession`:** Đại diện cho mỗi Client đang duy trì kết nối tới Server, bao gồm:
  * Mã định danh phiên (`id`): Số nguyên tự tăng duy nhất cho mỗi máy.
  * Socket descriptor (`socket_fd`): Kênh giao tiếp với Client đó.
  * Thông tin mạng: Địa chỉ IP và số hiệu Port của Client.
  * Thời điểm kết nối: Dùng để theo dõi thời gian hoạt động trực tuyến.
  * Khóa tương hỗ riêng (`std::mutex`): Đảm bảo các tác vụ gửi/nhận trên socket này luôn an toàn, tránh việc nhiều luồng cùng ghi đè dữ liệu lên một kênh truyền.
* **Lớp `SessionManager`:** Quản lý tập hợp các phiên thông qua bảng băm `std::unordered_map<int, std::shared_ptr<ClientSession>>`. Cung cấp các phương thức được bảo vệ bởi `std::mutex`:
  * `add_session`: Nhận socket mới, sinh ID tự tăng, lưu vào danh sách và trả về ID.
  * `remove_session`: Xóa phiên khỏi danh sách khi Client ngắt kết nối và đóng socket liên quan.
  * `get_session`: Tìm kiếm và trả về con trỏ phiên làm việc theo ID.
  * `get_all_sessions`: Trả về bản sao danh sách toàn bộ các phiên đang hoạt động để phục vụ tác vụ duyệt danh sách hoặc phát sóng lệnh.

#### Bước 3.2: Thiết kế Kiến trúc Server Đa Luồng (`server/server.cpp`)
* **Luồng Lắng nghe Kết nối (Acceptor Thread):**
  * Khởi chạy dưới dạng một luồng chạy ngầm độc lập (`std::thread`).
  * Thực hiện vòng lặp liên tục gọi hàm `accept()` để chờ kết nối mới từ các Client.
  * Mỗi khi có kết nối mới thành công: Trích xuất địa chỉ IP và Port của Client, đưa vào `SessionManager`, cấp phát Session ID và hiển thị thông báo kết nối mới trên màn hình quản trị.
* **Luồng Giao diện Điều khiển Chính (CLI Main Thread):**
  * Đảm nhiệm việc tương tác với người quản trị qua dòng lệnh, không bao giờ bị nghẽn bởi các kết nối mạng mới.

#### Bước 3.3: Điều phối Lệnh Phân Tầng và Cơ chế Phát sóng (Broadcast)
* **Phân tầng Điều khiển:**
  * *Tầng Quản trị Tổng thể (`RAT-Manager>`):* Cung cấp các lệnh quản lý diện rộng gồm `SESSIONS` (xem danh sách toàn bộ máy online), `INTERACT <id>` (chọn một máy để điều khiển trực tiếp) và `BROADCAST <cmd>` (phát lệnh cho toàn bộ máy).
  * *Tầng Tương tác 1-1 (`RAT-Shell [Client ID]>`):* Tương tác riêng biệt với máy đích đã chọn để chạy các lệnh hệ thống hoặc truyền file. Hỗ trợ lệnh `BACK` để quay trở lại Tầng Quản trị Tổng thể mà vẫn giữ nguyên kết nối ngầm của Client.
* **Cơ chế Phát sóng Đồng thời (Concurrent Broadcast):**
  * Khi người quản trị gọi lệnh `BROADCAST`:
  * Server lấy danh sách toàn bộ các phiên đang hoạt động từ `SessionManager`.
  * Sử dụng cơ chế bất đồng bộ `std::async(std::launch::async, ...)` để khởi tạo các luồng gửi lệnh song song tới tất cả Client.
  * Mỗi luồng con thực hiện khóa mutex riêng của Client đó, gửi lệnh qua `send_message`, chờ nhận phản hồi qua `recv_message` và đóng gói kết quả kèm ID máy.
  * Luồng chính duyệt danh sách các `std::future` để thu thập và in kết quả phản hồi của từng máy ra màn hình một cách tuần tự, rõ ràng, tránh hiện tượng nghẽn toàn bộ hệ thống khi có một Client bị lag.

---

## 10. Hướng Dẫn Biên Dịch & Chạy Thử Nghiệm Toàn Diện

### 10.1. Quy trình Biên dịch Toàn bộ Dự án
Mở terminal tại thư mục gốc của dự án và thực hiện tuần tự các bước:
1. Tạo thư mục build riêng biệt để giữ sạch mã nguồn: `mkdir -p build && cd build`.
2. Chạy CMake để sinh cấu hình Makefile và tự động tải bộ Google Test: `cmake ..`.
3. Biên dịch toàn bộ các mục tiêu (Server, Client và bộ Test) bằng trình biên dịch song song: `make -j$(nproc)`.

### 10.2. Quy trình Thực thi Kiểm thử Tự động (Unit Test)
* Chạy toàn bộ các ca kiểm thử thông qua công cụ kiểm thử tự động của CMake:
  ```bash
  ctest --output-on-failure
  ```
* Hoặc chạy trực tiếp file thực thi kiểm thử để xem chi tiết từng ca thử nghiệm:
  ```bash
  ./rat_unit_tests
  ```
* **Tiêu chí đánh giá:** Toàn bộ các ca kiểm thử thuộc `SocketUtilsTest` và `ExecutorTest` đều phải đạt trạng thái `PASSED`, đảm bảo các module mạng và logic hệ thống hoạt động chính xác trước khi đưa vào vận hành.

### 10.3. Kịch bản Thử nghiệm Thực tế Hệ thống Mới
Chuẩn bị 3 cửa sổ terminal trên máy Linux:
1. **Tại Terminal 1 (Khởi chạy Server):**
   * Khởi động chương trình quản trị: `./bin_server`.
   * Server thông báo đang lắng nghe trên cổng 8888 và hiển thị dấu nhắc `RAT-Manager>`.
2. **Tại Terminal 2 (Khởi chạy Client thứ nhất):**
   * Chạy: `./bin_client`.
   * Terminal 1 lập tức thông báo có Client mới kết nối với ID là 1.
3. **Tại Terminal 3 (Khởi chạy Client thứ hai):**
   * Chạy: `./bin_client`.
   * Terminal 1 tiếp tục thông báo có thêm Client mới với ID là 2.
4. **Thao tác kiểm tra tại Terminal 1:**
   * Nhập `SESSIONS`: Màn hình hiển thị bảng danh sách 2 Client đang trực tuyến kèm ID, địa chỉ IP và Port.
   * Nhập `BROADCAST LIST_PROC`: Server phát lệnh tới cả hai Client song song và in kết quả danh sách tiến trình của cả 2 máy trả về.
   * Nhập `INTERACT 1`: Dấu nhắc chuyển thành `RAT-Shell [Client 1]>`.
   * Nhập `DOWNLOAD_FILE /var/log/syslog ./syslog_client1.log`: Client 1 bắt đầu stream file theo từng khối 64 KB, Server hiển thị thanh tiến độ phần trăm đến khi hoàn tất mà không bị tràn bộ nhớ.
   * Nhập `BACK`: Dấu nhắc quay trở lại `RAT-Manager>` để tiếp tục quản lý các máy khác.

