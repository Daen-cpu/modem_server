#include "modem_server.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void print_usage(const char* program) {

    std::cout
        << "Usage: " << program << " --device DEVICE [options]\n"
        << "\nOptions:\n"
        << "  --device PATH       TTY device (required)\n"
        << "  --config PATH       expect=answer file (default: config/expect.txt)\n"
        << "  --baud RATE         baud rate (default: 115200)\n"
        << "  --help              show this help\n";
}

bool parse_unsigned(const std::string& value, unsigned int& result) {

    char* end = NULL;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);

    if (end == value.c_str() || *end != '\0') return false;
    if (parsed > 0xFFFFFFFFUL) return false;

    result = static_cast<unsigned int>(parsed);
    return true;
}

}

int main(int argc, char* argv[]) {

    std::string device;
    std::string config = "config/expect.txt";
    unsigned int baud = 115200U;

    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);

        if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            return 0;
        }

        if (argument == "--device" || argument == "-d") {
            if (i + 1 >= argc) {
                std::cerr << "--device requires an argument\n";
                return 1;
            }

            device = argv[++i];
            continue;
        }

        if (argument == "--config" || argument == "-c") {
            if (i + 1 >= argc) {
                std::cerr << "--config requires an argument\n";
                return 1;
            }

            config = argv[++i];
            continue;
        }

        if (argument == "--baud" || argument == "-b") {
            if (i + 1 >= argc || !parse_unsigned(argv[i + 1], baud)) {
                std::cerr << "--baud requires a valid integer\n";
                return 1;
            }

            ++i;
            continue;
        }

        std::cerr << "unknown argument: " << argument << "\n";
        print_usage(argv[0]);
        return 1;
    }

    if (device.empty()) {
        std::cerr << "--device is required\n";
        print_usage(argv[0]);
        return 1;
    }

    atmodem::ModemServer server(device, baud, config);
    return server.run();
}
