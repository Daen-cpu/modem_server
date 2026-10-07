#ifndef AT_MODEM_SERIAL_PORT_HPP
#define AT_MODEM_SERIAL_PORT_HPP

#include <string>
#include <termios.h>

namespace atmodem {

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    bool open(const std::string& device,
              unsigned int baud,
              std::string& error);
    void close();

    bool read_char(char& value, std::string& error);
    bool write_all(const std::string& data, std::string& error);

    bool is_open() const;

private:
    int fd_;

    SerialPort(const SerialPort&);
    SerialPort& operator=(const SerialPort&);
};

}

#endif
