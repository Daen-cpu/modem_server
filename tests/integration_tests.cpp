#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error(message) {}
};

void check(bool condition, const std::string& message) {

    if (!condition) throw TestFailure(message);
}

std::string executable_path() {

    const char* value = std::getenv("AT_MODEM_SERVER");
    if (value != NULL) return std::string(value);

    char buffer[4096];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1U);
    check(length > 0, "cannot determine integration test executable path");

    buffer[length] = '\0';
    const std::string path(buffer);
    const std::size_t separator = path.find_last_of('/');
    check(separator != std::string::npos, "cannot determine executable directory");

    return path.substr(0U, separator + 1U) + "at_modem_server";
}

std::string config_path() {

    const char* value = std::getenv("AT_MODEM_CONFIG");
    if (value != NULL) return std::string(value);

    char buffer[4096];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1U);
    check(length > 0, "cannot determine integration test executable path");

    buffer[length] = '\0';
    const std::string path(buffer);
    const std::size_t separator = path.find_last_of('/');
    check(separator != std::string::npos, "cannot determine executable directory");

    return path.substr(0U, separator) + "/../config/expect.txt";
}

class PtyPair {
public:
    PtyPair()
        : master_fd_(-1),
          slave_fd_(-1),
          slave_name_() {
    
        master_fd_ = posix_openpt(O_RDWR | O_NOCTTY);
        check(master_fd_ >= 0, "posix_openpt() failed");

        check(grantpt(master_fd_) == 0, "grantpt() failed");
        check(unlockpt(master_fd_) == 0, "unlockpt() failed");

        const char* name = ptsname(master_fd_);
        check(name != NULL, "ptsname() failed");

        slave_name_ = name;
        slave_fd_ = open(slave_name_.c_str(), O_RDWR | O_NOCTTY);
        check(slave_fd_ >= 0, "cannot open PTY slave");
    }

    ~PtyPair() {
    
        close_fds();
    }

    int master_fd() const
    {
        return master_fd_;
    }

    const std::string& slave_name() const
    {
        return slave_name_;
    }

private:
    void close_fds() {
    
        if (slave_fd_ >= 0) {
            close(slave_fd_);
            slave_fd_ = -1;
        }

        if (master_fd_ >= 0) {
            close(master_fd_);
            master_fd_ = -1;
        }
    }

    int master_fd_;
    int slave_fd_;
    std::string slave_name_;
};

bool wait_for_data(int fd, int timeout_ms) {

    struct pollfd descriptor;
    descriptor.fd = fd;
    descriptor.events = POLLIN;
    descriptor.revents = 0;

    while (true) {
        const int result = poll(&descriptor, 1, timeout_ms);

        if (result > 0) return (descriptor.revents & POLLIN) != 0;
        if (result == 0) return false;
        if (errno == EINTR) continue;

        return false;
    }
}

std::string read_until(int fd,
                       const std::string& expected,
                       int timeout_ms) {

    std::string result;
    char buffer[256];

    while (result.find(expected) == std::string::npos) {
        if (!wait_for_data(fd, timeout_ms)) break;

        const ssize_t count = read(fd, buffer, sizeof(buffer));

        if (count > 0) {
            result.append(buffer, static_cast<std::size_t>(count));
            continue;
        }

        if (count < 0 && errno == EINTR) continue;
        break;
    }

    return result;
}

void write_all(int fd, const std::string& data) {

    std::size_t offset = 0U;

    while (offset < data.size()) {
        const ssize_t written =
            write(fd, data.data() + offset, data.size() - offset);

        if (written > 0) {
            offset += static_cast<std::size_t>(written);
            continue;
        }

        if (written < 0 && errno == EINTR) continue;

        if (written == 0) {
            throw TestFailure("write() returned zero");
        }

        throw TestFailure(
            "write() failed: " + std::string(std::strerror(errno)));
    }
}

void flush_input(int fd) {

    char buffer[256];

    while (wait_for_data(fd, 20)) {
        const ssize_t count = read(fd, buffer, sizeof(buffer));

        if (count > 0) continue;
        if (count < 0 && errno == EINTR) continue;

        break;
    }
}

void send_command(int fd,
                  const std::string& command,
                  const std::string& expected) {

    write_all(fd, command + "\r");

    const std::string response = read_until(fd, expected, 1000);

    if (response.find(expected) == std::string::npos) {
        throw TestFailure(
            "unexpected response for command '" + command + "': [" + response + "]");
    }
}

pid_t start_server(const PtyPair& pty, const std::string& config) {

    int exec_pipe[2];
    check(pipe(exec_pipe) == 0, "pipe() failed");

    const int flags = fcntl(exec_pipe[1], F_GETFD);
    check(flags >= 0, "fcntl(F_GETFD) failed");
    check(fcntl(exec_pipe[1], F_SETFD, flags | FD_CLOEXEC) == 0,
          "fcntl(F_SETFD) failed");

    const pid_t pid = fork();
    check(pid >= 0, "fork() failed");

    if (pid == 0) {
        close(exec_pipe[0]);

        const std::string executable = executable_path();
        execl(executable.c_str(),
              executable.c_str(),
              "--device",
              pty.slave_name().c_str(),
              "--config",
              config.c_str(),
              "--baud",
              "115200",
              static_cast<char*>(NULL));

        const int error_code = errno;
        (void)write(exec_pipe[1], &error_code, sizeof(error_code));
        _exit(127);
    }

    close(exec_pipe[1]);

    int error_code = 0;
    const ssize_t result = read(exec_pipe[0], &error_code, sizeof(error_code));
    const int saved_errno = errno;
    close(exec_pipe[0]);

    if (result > 0) {
        waitpid(pid, NULL, 0);
        throw TestFailure(
            "exec failed: " + std::string(std::strerror(error_code)));
    }

    if (result < 0) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        throw TestFailure(
            "read() from exec status pipe failed: " +
            std::string(std::strerror(saved_errno)));
    }

    return pid;
}

bool wait_for_process(pid_t pid, int timeout_ms) {

    const int step_ms = 10;
    int elapsed = 0;

    while (elapsed < timeout_ms) {
        int status = 0;
        const pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == pid) return true;
        if (result < 0) return false;

        usleep(static_cast<useconds_t>(step_ms * 1000));
        elapsed += step_ms;
    }

    return false;
}

void stop_server(pid_t pid) {

    if (pid <= 0) return;

    kill(pid, SIGTERM);

    if (wait_for_process(pid, 1000)) return;

    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
}

void test_basic_commands(int fd) {

    std::cout << "  AT..." << std::endl;
    send_command(fd, "AT", "OK\r\n");

    std::cout << "  ATI..." << std::endl;
    send_command(fd, "ATI", "OK\r\n");

    std::cout << "  AT+COPS..." << std::endl;
    send_command(fd, "AT+COPS", "OK\r\n");

    std::cout << "  AT+CPIN..." << std::endl;
    send_command(fd, "AT+CPIN", "OK\r\n");
}

void test_echo(int fd) {

    std::cout << "  ATE0..." << std::endl;
    write_all(fd, "ATE0\r");

    std::string response = read_until(fd, "OK\r\n", 1000);
    check(response.find("ATE0\r\n") != std::string::npos,
          "ATE0 should be echoed before echo mode is disabled");
    check(response.find("OK\r\n") != std::string::npos,
          "ATE0 should return OK");

    flush_input(fd);

    std::cout << "  AT with echo disabled..." << std::endl;
    write_all(fd, "AT\r");
    response = read_until(fd, "OK\r\n", 1000);
    check(response == "OK\r\n",
          "AT should return only OK when echo is disabled");

    std::cout << "  ATE1..." << std::endl;
    write_all(fd, "ATE1\r");
    response = read_until(fd, "OK\r\n", 1000);
    check(response.find("ATE1\r\n") != std::string::npos,
          "ATE1 should be echoed before echo mode is enabled");
    check(response.find("OK\r\n") != std::string::npos,
          "ATE1 should return OK");

    flush_input(fd);

    std::cout << "  AT with echo enabled..." << std::endl;
    write_all(fd, "AT\r");
    response = read_until(fd, "OK\r\n", 1000);
    check(response.find("AT\r\n") != std::string::npos,
          "AT should be echoed after ATE1");
    check(response.find("OK\r\n") != std::string::npos,
          "AT should return OK");
}

void test_unknown_command(int fd) {

    std::cout << "  unknown command..." << std::endl;
    write_all(fd, "AT+UNKNOWN\r");

    const std::string response = read_until(fd, "ERROR\r\n", 1000);
    check(response.find("ERROR\r\n") != std::string::npos,
          "unknown command should return ERROR");
}

void test_overflow(int fd) {

    std::cout << "  oversized command..." << std::endl;

    write_all(fd, std::string(4097U, 'X') + "\r");
    const std::string response = read_until(fd, "ERROR\r\n", 1000);

    check(response == "ERROR\r\n",
          "oversized command should return exactly one ERROR");
}

void run_tests() {

    PtyPair pty;
    const pid_t server_pid = start_server(pty, config_path());

    try {
        usleep(100000);
        check(kill(server_pid, 0) == 0,
              "server process terminated unexpectedly");

        test_basic_commands(pty.master_fd());
        test_echo(pty.master_fd());
        test_unknown_command(pty.master_fd());
        test_overflow(pty.master_fd());

        stop_server(server_pid);

        int status = 0;
        const pid_t result = waitpid(server_pid, &status, WNOHANG);
        check(result == -1 || result == server_pid,
              "server process was not terminated");

        std::cout << "Integration tests passed." << std::endl;
    } catch (...) {
        stop_server(server_pid);
        throw;
    }
}

}

int main() {

    try {
        run_tests();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Integration test failed: " << error.what() << std::endl;
        return 1;
    }
}
