#include <gtest/gtest.h>
#include "session_manager.h"
#include "server.h"
#include "protocol.h"
#include "socket_utils.h"

#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <vector>

namespace RAT
{
    // 1. KIỂM THỬ SESSION MANAGER
    class SessionManagerTest : public ::testing::Test
    {
    protected:
        SessionManager manager;
    };

    // Thêm session mới
    TEST_F(SessionManagerTest, AddAndGetSessionSuccess)
    {
        int fds[2];
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

        int id = manager.add_session(fds[0], "192.168.1.50", 9999);
        EXPECT_GT(id, 0);
        EXPECT_EQ(manager.count(), 1);

        auto session = manager.get_session(id);
        ASSERT_NE(session, nullptr);
        EXPECT_EQ(session->id, id);
        EXPECT_EQ(session->ip, "192.168.1.50");
        EXPECT_EQ(session->port, 9999);
        EXPECT_EQ(session->socket_fd, fds[0]);

        ::close(fds[1]);
    }

    // Lấy session non
    TEST_F(SessionManagerTest, GetNonExistentSessionReturnsNull)
    {
        auto session = manager.get_session(9999);
        EXPECT_EQ(session, nullptr);
    }

    // Xóa session
    TEST_F(SessionManagerTest, RemoveSessionCleansUp)
    {
        int fds[2];
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

        int id = manager.add_session(fds[0], "127.0.0.1", 8888);
        EXPECT_EQ(manager.count(), 1);

        manager.remove_session(id);
        EXPECT_EQ(manager.count(), 0);
        EXPECT_EQ(manager.get_session(id), nullptr);

        ::close(fds[1]);
    }

    // Tái sử dụng ID nhỏ nhất còn trống khi client ngắt kết nối
    TEST_F(SessionManagerTest, ReusesLowestAvailableIdAfterRemoval)
    {
        int fds1[2], fds2[2], fds3[2], fds_new[2];
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds1), 0);
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds2), 0);
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds3), 0);

        int id1 = manager.add_session(fds1[0], "127.0.0.1", 10001);
        int id2 = manager.add_session(fds2[0], "127.0.0.1", 10002);
        int id3 = manager.add_session(fds3[0], "127.0.0.1", 10003);

        EXPECT_EQ(id1, 1);
        EXPECT_EQ(id2, 2);
        EXPECT_EQ(id3, 3);

        // Ngắt kết nối client ID 2
        manager.remove_session(2);
        EXPECT_EQ(manager.get_session(2), nullptr);

        // Client mới kết nối -> Phải nhận ID 2 (ID nhỏ nhất còn trống)
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds_new), 0);
        int id_reused = manager.add_session(fds_new[0], "127.0.0.1", 10004);
        EXPECT_EQ(id_reused, 2);

        // Client kế tiếp kết nối -> Phải nhận ID 4
        int fds_next[2];
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds_next), 0);
        int id_next = manager.add_session(fds_next[0], "127.0.0.1", 10005);
        EXPECT_EQ(id_next, 4);

        ::close(fds1[1]);
        ::close(fds2[1]);
        ::close(fds3[1]);
        ::close(fds_new[1]);
        ::close(fds_next[1]);
    }

    //  Lấy bản sao ds client
    TEST_F(SessionManagerTest, GetAllSessionsReturnsAllEntries)
    {
        std::vector<int> client_ends;
        for (int i = 1; i <= 3; ++i)
        {
            int fds[2];
            ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
            manager.add_session(fds[0], "10.0.0." + std::to_string(i), 8000 + i);
            client_ends.push_back(fds[1]);
        }

        EXPECT_EQ(manager.count(), 3);
        auto all_sessions = manager.get_all_sessions();
        EXPECT_EQ(all_sessions.size(), 3);

        for (int i = 1; i <= 3; ++i)
        {
            EXPECT_NE(all_sessions.find(i), all_sessions.end());
        }

        for (int fd : client_ends)
        {
            ::close(fd);
        }
    }

    // Đóng toàn bộ kết nối
    TEST_F(SessionManagerTest, CloseAllTerminatesAndClears)
    {
        std::vector<int> client_ends;
        for (int i = 0; i < 5; ++i)
        {
            int fds[2];
            ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
            manager.add_session(fds[0], "127.0.0.1", 7000 + i);
            client_ends.push_back(fds[1]);
        }
        EXPECT_EQ(manager.count(), 5);

        manager.close_all();
        EXPECT_EQ(manager.count(), 0);
        EXPECT_TRUE(manager.get_all_sessions().empty());

        for (int fd : client_ends)
        {
            ::close(fd);
        }
    }

    // Tao 10 luồng ghi data
    TEST_F(SessionManagerTest, ConcurrentMultiThreadedAccess)
    {
        constexpr int NUM_THREADS = 10;
        std::vector<std::thread> threads;
        std::vector<int> client_ends(NUM_THREADS);

        for (int i = 0; i < NUM_THREADS; ++i)
        {
            threads.emplace_back([this, i, &client_ends]() {
                int fds[2];
                if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0)
                {
                    client_ends[i] = fds[1];
                    manager.add_session(fds[0], "172.16.0." + std::to_string(i), 9000 + i);
                }
            });
        }

        for (auto &t : threads)
        {
            t.join();
        }

        EXPECT_EQ(manager.count(), NUM_THREADS);

        for (int fd : client_ends)
        {
            ::close(fd);
        }
    }


    // 2. KIỂM THỬ PHÂN TÍCH LỆNH SERVER
    class ServerCommandTest : public ::testing::Test
    {
    protected:
        Server server;
    };

    TEST_F(ServerCommandTest, EmptyCommandReturnsFalse)
    {
        std::string cmd, args;
        EXPECT_FALSE(server.validate_and_process_command("", cmd, args));
        EXPECT_FALSE(server.validate_and_process_command("    ", cmd, args));
    }

    TEST_F(ServerCommandTest, HelpCommandHandledLocally)
    {
        std::string cmd, args;
        // Lệnh HELP hợp lệ nhưng không gửi sang client nên trả về false
        EXPECT_FALSE(server.validate_and_process_command("HELP", cmd, args));
        EXPECT_EQ(cmd, "HELP");
    }

    TEST_F(ServerCommandTest, ListDirWithoutArgsDefaultsToCurrentDir)
    {
        std::string cmd, args;
        EXPECT_TRUE(server.validate_and_process_command("LIST_DIR", cmd, args));
        EXPECT_EQ(cmd, Command::LIST_DIR);
        EXPECT_EQ(args, ".");
    }

    TEST_F(ServerCommandTest, ListDirWithArgsPreservesPath)
    {
        std::string cmd, args;
        EXPECT_TRUE(server.validate_and_process_command("LIST_DIR /var/log", cmd, args));
        EXPECT_EQ(cmd, Command::LIST_DIR);
        EXPECT_EQ(args, "/var/log");
    }

    TEST_F(ServerCommandTest, ReadFileValidation)
    {
        std::string cmd, args;
        // Thiếu tham số -> false
        EXPECT_FALSE(server.validate_and_process_command("READ_FILE", cmd, args));
        // Đủ tham số -> true
        EXPECT_TRUE(server.validate_and_process_command("READ_FILE /etc/os-release", cmd, args));
        EXPECT_EQ(cmd, Command::READ_FILE);
        EXPECT_EQ(args, "/etc/os-release");
    }

    TEST_F(ServerCommandTest, ListProcValidation)
    {
        std::string cmd, args;
        EXPECT_TRUE(server.validate_and_process_command("LIST_PROC", cmd, args));
        EXPECT_EQ(cmd, Command::LIST_PROC);
    }

    TEST_F(ServerCommandTest, KillProcValidation)
    {
        std::string cmd, args;
        EXPECT_FALSE(server.validate_and_process_command("KILL_PROC", cmd, args));
        EXPECT_TRUE(server.validate_and_process_command("KILL_PROC 1234", cmd, args));
        EXPECT_EQ(cmd, Command::KILL_PROC);
        EXPECT_EQ(args, "1234");
    }

    TEST_F(ServerCommandTest, DownloadFileValidation)
    {
        std::string cmd, args;
        // Thiếu tham số
        EXPECT_FALSE(server.validate_and_process_command("DOWNLOAD_FILE", cmd, args));
        EXPECT_FALSE(server.validate_and_process_command("DOWNLOAD_FILE /remote/path", cmd, args));
        // Đủ cả 2 tham số
        EXPECT_TRUE(server.validate_and_process_command("DOWNLOAD_FILE /remote/path ./local/path", cmd, args));
        EXPECT_EQ(cmd, Command::DOWNLOAD_FILE);
    }

    TEST_F(ServerCommandTest, UnknownCommandReturnsFalse)
    {
        std::string cmd, args;
        EXPECT_FALSE(server.validate_and_process_command("UNKNOWN_INVALID_CMD 123", cmd, args));
    }

    
    // 3. KIỂM THỬ ĐIỀU PHỐI ĐA CLIENT (MOCK SOCKETPAIR)
    TEST_F(ServerCommandTest, MultiClientSessionDispatchMock)
    {
        // Giả lập 3 client kết nối qua socketpair
        constexpr int NUM_CLIENTS = 3;
        int client_fds[NUM_CLIENTS];

        for (int i = 0; i < NUM_CLIENTS; ++i)
        {
            int sv[2];
            ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
            server.get_session_manager().add_session(sv[0], "192.168.1." + std::to_string(10 + i), 5000 + i);
            client_fds[i] = sv[1]; // Đầu phía Client
        }

        EXPECT_EQ(server.get_session_manager().count(), NUM_CLIENTS);

        // Server gửi tin nhắn riêng cho từng Client và Client phản hồi
        for (int i = 0; i < NUM_CLIENTS; ++i)
        {
            auto session = server.get_session_manager().get_session(i + 1);
            ASSERT_NE(session, nullptr);

            // Server gửi lệnh
            EXPECT_TRUE(send_message(session->socket_fd, "LIST_PROC"));

            // Client ảo nhận lệnh
            std::string received_cmd;
            EXPECT_TRUE(recv_message(client_fds[i], received_cmd));
            EXPECT_EQ(received_cmd, "LIST_PROC");

            // Client ảo phản hồi
            std::string client_resp = "PROC_OK_FROM_CLIENT_" + std::to_string(i + 1);
            EXPECT_TRUE(send_message(client_fds[i], client_resp));

            // Server nhận phản hồi
            std::string server_received;
            EXPECT_TRUE(recv_message(session->socket_fd, server_received));
            EXPECT_EQ(server_received, client_resp);
        }

        for (int i = 0; i < NUM_CLIENTS; ++i)
        {
            ::close(client_fds[i]);
        }
    }

    // 4. KIỂM THỬ KHỞI ĐỘNG VÀ DỪNG SERVER KHÔNG BỊ TREO (NON-BLOCKING GRACEFUL STOP)
    TEST(ServerLifecycleTest, StartAndStopGracefullyWithoutHang)
    {
        Server test_server(19876);
        EXPECT_TRUE(test_server.start());
        
        // Dừng server (phải kết thúc ngay lập tức và join được acceptor_thread_)
        test_server.stop();
        SUCCEED();
    }
}
