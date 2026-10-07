# AT Modem Emulator

Linux C++14 test assignment: a server that emulates a simple modem and responds to AT commands received through a TTY device.

## Requirements

- Linux
- C++14 compiler
- CMake >= 3.10
- POSIX `termios`
- No regular-expression library is used.

## Supported pattern syntax

The configuration file contains `expect=answer` rules.

- `.` — exactly one arbitrary character;
- `*` — zero or more arbitrary characters;
- `[abc]` — one character from the listed set;
- `[0-9]` — one character from a range;
- all other characters are matched literally.

For example, `A*E` matches `AE`, `AbE`, `AbcdE`, `AbcdxyzE`.

The matcher is implemented manually in `src/command_matcher.cpp` and does not use `std::regex` or any external regex library.

## Configuration

Default file: `config/expect.txt`.

Example:

```text
AT=OK\r\n
ATI=OpenAI AT Modem Emulator\r\nOK\r\n
AT+COPS=+COPS: 0,0,"TestOperator"\r\nOK\r\n
AT+CPIN=+CPIN: READY\r\nOK\r\n
```

The parser supports `\r`, `\n`, `\t` and `\\` escapes in both patterns and answers.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The build uses C++14 explicitly:

```cmake
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The tests cover exact matching, `.`, `*`, character classes, ranges, modem commands, echo mode, unknown commands, oversized commands and graceful server shutdown.

## Run

```bash
./build/at_modem_server \
    --device /dev/ttyUSB0 \
    --config config/expect.txt \
    --baud 115200
```

Serial parameters are:

- 8 data bits;
- no parity;
- 1 stop bit;
- no hardware flow control;
- raw input/output mode;
- 115200 baud by default.

The baud rate can be changed with `--baud`.

## Modem behaviour

The server supports:

- `AT` — responds according to the configuration;
- `ATE0` — disables command echo and returns `OK`;
- `ATE1` — enables command echo and returns `OK`;
- `ATI` — configured modem identification;
- `AT+COPS` — configured operator response;
- `AT+CPIN` — configured SIM state response;
- unknown commands — `ERROR`.

Commands are terminated by CR or LF. Responses use CRLF, as is typical for AT command interfaces.

## Testing without physical hardware

A pair of pseudo-terminals can be created with `socat`:

```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

It prints two PTY paths, for example `/dev/pts/3` and `/dev/pts/4`.

Run the emulator on one side:

```bash
./build/at_modem_server --device /dev/pts/3
```

Use another terminal connected to `/dev/pts/4` to send commands. For example:

```bash
printf 'AT\r' > /dev/pts/4
printf 'ATI\r' > /dev/pts/4
printf 'ATE0\r' > /dev/pts/4
printf 'AT+COPS\r' > /dev/pts/4
```

## Project structure

```text
at_modem_server/
├── CMakeLists.txt
├── README.md
├── config/
│   └── expect.txt
├── src/
│   ├── command_matcher.cpp
│   ├── command_matcher.hpp
│   ├── expect_table.cpp
│   ├── expect_table.hpp
│   ├── modem_server.cpp
│   ├── modem_server.hpp
│   ├── serial_port.cpp
│   ├── serial_port.hpp
│   └── main.cpp
└── tests/
    └── matcher_tests.cpp
```
