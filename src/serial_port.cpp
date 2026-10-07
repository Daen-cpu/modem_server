#include "serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace atmodem {
namespace {

speed_t baud_to_termios(unsigned int baud) {

    switch (baud) {
    case 50: return B50;
    case 75: return B75;
    case 110: return B110;
    case 134: return B134;
    case 150: return B150;
    case 200: return B200;
    case 300: return B300;
    case 600: return B600;
    case 1200: return B1200;
    case 1800: return B1800;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    default: return 0;
    }
}

}

SerialPort::SerialPort()
    : fd_(-1) {

}

SerialPort::~SerialPort() {

    close();
}

bool SerialPort::open(const std::string& device,
                      unsigned int baud,
                      std::string& error) {

    close();

    const speed_t speed = baud_to_termios(baud);
    if (speed == 0) {
        error = "unsupported baud rate";
        return false;
    }

    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0) {
        error = "cannot open " + device + ": " + std::strerror(errno);
        return false;
    }

    struct termios tty;
    if (tcgetattr(fd_, &tty) != 0) {
        error = "tcgetattr failed: ";
        error += std::strerror(errno);
        close();
        return false;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        error = "tcsetattr failed: ";
        error += std::strerror(errno);
        close();
        return false;
    }

    if (tcflush(fd_, TCIOFLUSH) != 0) {
        error = "tcflush failed: ";
        error += std::strerror(errno);
        close();
        return false;
    }

    return true;
}

void SerialPort::close() {

    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool SerialPort::read_char(char& value, std::string& error) {

    const ssize_t result = ::read(fd_, &value, 1U);

    if (result == 1) return true;

    if (result < 0 && errno == EINTR) {
        error.clear();
        return false;
    }

    if (result == 0) {
        error = "read returned end-of-file";
        return false;
    }

    error = "read failed: ";
    error += std::strerror(errno);
    return false;
}

bool SerialPort::write_all(const std::string& data, std::string& error) {

    std::size_t offset = 0U;

    while (offset < data.size()) {
        const ssize_t result = ::write(fd_, data.data() + offset,
                                       data.size() - offset);

        if (result > 0) {
            offset += static_cast<std::size_t>(result);
            continue;
        }

        if (result < 0 && errno == EINTR) continue;

        if (result == 0) {
            error = "write returned zero";
            return false;
        }

        error = "write failed: ";
        error += std::strerror(errno);
        return false;
    }

    return true;
}

bool SerialPort::is_open() const
{
    return fd_ >= 0;
}

}