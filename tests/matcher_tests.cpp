#include "command_matcher.hpp"

#include <cstdlib>
#include <iostream>

using atmodem::CommandMatcher;

void expect_match(const std::string& pattern, const std::string& text) {
    if (!CommandMatcher::matches(pattern, text)) {
        std::cerr << "Expected match: [" << pattern << "] vs [" << text << "]\n";
        std::exit(1);
    }
}

void expect_no_match(const std::string& pattern, const std::string& text) {
    if (CommandMatcher::matches(pattern, text)) {
        std::cerr << "Expected no match: [" << pattern << "] vs [" << text << "]\n";
        std::exit(1);
    }
}

int main() {
    expect_match("AT", "AT");
    expect_no_match("AT", "ATI");

    expect_match("A.E", "ABE");
    expect_match("A.E", "A0E");
    expect_no_match("A.E", "AE");

    expect_match("A*E", "AE");
    expect_match("A*E", "AbE");
    expect_match("A*E", "AbcdE");
    expect_match("A*E", "AbcdxyzE");
    expect_no_match("A*E", "Abcd");

    expect_match("ATE[01]", "ATE0");
    expect_match("ATE[01]", "ATE1");
    expect_no_match("ATE[01]", "ATE2");

    expect_match("AT+CPIN=[0-9]", "AT+CPIN=5");
    expect_match("AT+CPIN=[A-Z]", "AT+CPIN=R");
    expect_no_match("AT+CPIN=[0-9]", "AT+CPIN=A");

    expect_match("AT+C*", "AT+COPS");
    expect_match("AT+C*", "AT+CPIN");
    expect_no_match("AT+C*", "AT");

    expect_match("[ab]*", "abcXYZ");
    expect_match("[ab]*", "bbb");
    expect_no_match("[ab]*", "c");

    std::cout << "All matcher tests passed\n";
    return 0;
}
