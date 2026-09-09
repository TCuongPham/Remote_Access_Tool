#include "executor.h"
#include "protocol.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <cctype>

namespace fs = std::filesystem;

namespace RAT
{
    namespace Executor
    {
        // Giới hạn đọc file tối đa 2MB để tránh nghẽn mạng và tràn RAM
        constexpr size_t MAX_READ_FILE_SIZE = 2 * 1024 * 1024;

        // 1. Liệt kê danh sách file/folder trong đường dẫn
        std::string list_directory(const std::string &path)
        {
            std::string target_path = path.empty() ? "." : path;
            std::ostringstream oss;

            try
            {
                fs::path p(target_path);

                // Kiểm tra đường dẫn
                if (!fs::exists(p))
                {
                    return Status::ERR + "Duong dan khong ton tai: " + target_path;
                }
                if (!fs::is_directory(p))
                {
                    return Status::ERR + "Duong dan khong phai la thu muc: " + target_path;
                }

                // In ra header danh sách
                oss << Status::OK << fs::canonical(p).string() << "\n";
                oss << std::left << std::setw(8) << "TYPE"
                    << std::setw(14) << "SIZE (Bytes)"
                    << "NAME\n";
                oss << std::string(55, '-') << "\n";

                // In chi tiết từng file/folder trong thư mục (bỏ qua các mục không có quyền)
                for (const auto &entry : fs::directory_iterator(p, fs::directory_options::skip_permission_denied))
                {
                    std::error_code ec;
                    bool is_dir = entry.is_directory(ec);
                    std::string type = is_dir ? "[DIR]" : "[FILE]";
                    std::string name = entry.path().filename().string();
                    std::string size_str = "-";
                    if (!is_dir)
                    {
                        auto size = entry.file_size(ec);
                        if (!ec)
                        {
                            size_str = std::to_string(size);
                        }
                    }
                    oss << std::left << std::setw(8) << type
                        << std::setw(14) << size_str
                        << name << "\n";
                }
            }

            catch (const fs::filesystem_error &e)
            {
                return Status::ERR + "Loi he thong file: " + e.what();
            }
            catch (const std::exception &e)
            {
                return Status::ERR + "Ngoai le: " + e.what();
            }
            return oss.str();
        }

        // 2. Đọc nội dung file
        std::string read_file_content(const std::string &filepath)
        {
            if (filepath.empty())
            {
                return Status::ERR + "Duong dan file trong!";
            }

            try
            {
                fs::path p(filepath);

                // Kiểm tra đường dẫn
                if (!fs::exists(p))
                {
                    return Status::ERR + "File khong ton tai: " + filepath;
                }
                if (fs::is_directory(p))
                {
                    return Status::ERR + "Day la thu muc, khong phai file: " + filepath;
                }

                // Kiểm tra kích thước file
                std::error_code ec;
                auto fsize = fs::file_size(p, ec);
                if (!ec && fsize > MAX_READ_FILE_SIZE)
                {
                    return Status::ERR + "File qua nang (" + std::to_string(fsize) + " bytes, gioi han 2MB)!";
                }

                // Mở file và đọc nội dung
                std::ifstream file(filepath, std::ios::in | std::ios::binary);
                // Kiểm tra mở file
                if (!file.is_open())
                {
                    return Status::ERR + "Loi hoac khong co quyen doc file: " + filepath;
                }
                // In nội dung file
                std::ostringstream ss;
                ss << file.rdbuf();
                return Status::OK + "Noi dung file [" + filepath + "]:\n" + ss.str();
            }

            catch (const std::exception &e)
            {
                return Status::ERR + "Loi khi doc file: " + e.what();
            }
        }

        // 3. Liệt kê danh sách tiến trình đang chạy
        std::string list_processes()
        {
            // In header danh sách
            std::ostringstream oss;
            oss << Status::OK << "Danh sach tien trinh dang chay:\n";
            oss << std::left << std::setw(10) << "PID" << "COMMAND / NAME\n";
            oss << std::string(45, '-') << "\n";

            // Tạo vector lưu thông tin tiến trình
            struct ProcessInfo
            {
                int pid;
                std::string name;
            };
            std::vector<ProcessInfo> procs;

            try
            {
                // Duyệt qua thư mục /proc để lấy danh sách tiến trình
                for (const auto &entry : fs::directory_iterator("/proc", fs::directory_options::skip_permission_denied))
                {
                    // Chỉ kiểm tra thư mục
                    if (!entry.is_directory())
                        continue;

                    // Lấy tên thư mục
                    std::string dirname = entry.path().filename().string();
                    // Chỉ lọc lấy tên thư mục là số (PID)
                    if (dirname.empty() || !std::all_of(dirname.begin(), dirname.end(), ::isdigit))
                    {
                        continue;
                    }

                    // Lấy PID ở dạng int
                    int pid = std::stoi(dirname);

                    // Lấy tên tiến trình từ file /proc/<PID>/comm
                    std::string proc_name = "unknown";
                    std::ifstream comm_file(entry.path() / "comm");
                    if (comm_file.is_open())
                    {
                        std::getline(comm_file, proc_name);
                    }

                    // Thêm vào vector
                    procs.push_back({pid, proc_name});
                }

                // Sắp xếp danh sách tiến trình theo PID tăng dần
                std::sort(procs.begin(), procs.end(), [](const ProcessInfo &a, const ProcessInfo &b)
                          { return a.pid < b.pid; });

                // In danh sách tiến trình
                for (const auto &p : procs)
                {
                    oss << std::left << std::setw(10) << p.pid << p.name << "\n";
                }
                oss << "\nTong so tien trinh: " << procs.size() << "\n";
            }
            catch (const std::exception &e)
            {
                return Status::ERR + "Loi liet ke tien trinh: " + e.what();
            }
            return oss.str();
        }

        // 4. Kết thúc tiến trình theo PID
        std::string kill_process(int pid)
        {
            // Kiểm tra PID hợp lệ
            if (pid <= 0)
            {
                return Status::ERR + "PID khong hop le: " + std::to_string(pid);
            }

            // Gửi tín hiệu SIGKILL (9) tới PID chỉ định
            if (::kill(pid, SIGKILL) == 0)
            {
                return Status::OK + "Da ket thuc tien trinh PID " + std::to_string(pid) + " thanh cong.";
            }

            // Nếu kill() trả về -1, kiểm tra errno để xác định lỗi
            switch (errno)
            {
            case ESRCH:
                return Status::ERR + "Khong tim thay tien trinh voi PID " + std::to_string(pid) + " (tien trinh co the da tat).";
            case EPERM:
                return Status::ERR + "Khong du quyen ket thuc PID " + std::to_string(pid) + " (Yeu cau quyen root/sudo).";
            default:
                return Status::ERR + "Loi khi kill PID " + std::to_string(pid) + ": " + std::strerror(errno);
            }
        }
    }
}