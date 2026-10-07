#ifndef AT_MODEM_SERVER_HPP
#define AT_MODEM_SERVER_HPP

#include "expect_table.hpp"
#include "serial_port.hpp"

#include <string>

namespace atmodem {

class ModemServer {
public:
    ModemServer(const std::string& device,
                unsigned int baud,
                const std::string& config_file);

    int run();

private:
    bool handle_command(const std::string& command);
    bool send_response(const std::string& response);
    void reset_command_buffer();

    std::string device_;
    unsigned int baud_;
    std::string config_file_;
    SerialPort serial_;
    ExpectTable table_;
    bool echo_enabled_;
    std::string command_buffer_;
};

}
#endif
