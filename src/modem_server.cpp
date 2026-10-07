#include "modem_server.hpp"

#include <cerrno>
#include <cstring>
#include <csignal>
#include <iostream>

namespace atmodem {
namespace {

const std::size_t MAX_COMMAND_LENGTH = 4096U;
volatile std::sig_atomic_t stop_requested = 0;

void signal_handler(int) {

    stop_requested = 1;
}

std::string strip_line_endings(const std::string& value) {

    std::string result = value;

    while (!result.empty() && (result[result.size() - 1U] == '\r' || result[result.size() - 1U] == '\n')) {result.erase(result.size() - 1U);}

    return result;
}

}

ModemServer::ModemServer(const std::string& device,
                         unsigned int baud,
                         const std::string& config_file)
    : device_(device),
      baud_(baud),
      config_file_(config_file),
      serial_(),
      table_(),
      echo_enabled_(true),
      command_buffer_() {

}

void ModemServer::reset_command_buffer() {command_buffer_.clear();}

bool ModemServer::send_response(const std::string& response) {

    std::string error;

    if (!serial_.write_all(response, error)) {
        std::cerr << error << std::endl;
        return false;
    }

    return true;
}

bool ModemServer::handle_command(const std::string& raw_command) {

    const std::string command = strip_line_endings(raw_command);
    if (command.empty()) return true;

    const bool enable_echo_command = (command == "ATE1");

    if (echo_enabled_ || enable_echo_command) {
        if (!send_response(command + "\r\n")) return false;
    }

    if (command == "ATE0") {
        echo_enabled_ = false;
        return send_response("OK\r\n");
    }

    if (command == "ATE1") {
        echo_enabled_ = true;
        return send_response("OK\r\n");
    }

    const ExpectRule* rule = table_.find(command);
    if (rule == NULL) return send_response("ERROR\r\n");

    return send_response(rule->answer);
}

int ModemServer::run() {

    std::string error;

    if (!table_.load(config_file_, error)) {
        std::cerr << error << std::endl;
        return 2;
    }

    if (!serial_.open(device_, baud_, error)) {
        std::cerr << error << std::endl;
        return 3;
    }

    stop_requested = 0;

    struct sigaction action = {};
    action.sa_handler = signal_handler;

    if (sigemptyset(&action.sa_mask) != 0) {
        std::cerr << "Failed to initialize signal mask" << std::endl;
        serial_.close();
        return 4;
    }

    if (sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        std::cerr << "Failed to install signal handler: " << std::strerror(errno) << std::endl;
        serial_.close();
        return 4;
    }

    std::cout << "AT modem emulator listening on " << device_ << " at " << baud_ << " 8N1" << std::endl;

    reset_command_buffer();
    bool command_overflowed = false;

    while (!stop_requested) {
        char value = '\0';

        if (!serial_.read_char(value, error)) {
            if (stop_requested || errno == EINTR) break;

            std::cerr << error << std::endl;
            serial_.close();
            return 5;
        }

        if (value == '\r' || value == '\n') {
            if (command_overflowed) {
                command_overflowed = false;
                reset_command_buffer();
                continue;
            }

            if (command_buffer_.empty()) continue;

            const std::string command = command_buffer_;
            reset_command_buffer();

            if (!handle_command(command)) {
                serial_.close();
                return 6;
            }

            continue;
        }

        if (command_overflowed) continue;

        if (command_buffer_.size() >= MAX_COMMAND_LENGTH) {
            reset_command_buffer();
            command_overflowed = true;

            if (!send_response("ERROR\r\n")) {
                serial_.close();
                return 7;
            }

            continue;
        }

        command_buffer_ += value;
    }

    serial_.close();
    return 0;
}

}
