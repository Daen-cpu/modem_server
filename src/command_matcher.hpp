#ifndef AT_MODEM_COMMAND_MATCHER_HPP
#define AT_MODEM_COMMAND_MATCHER_HPP

#include <string>

namespace atmodem {

class CommandMatcher {
public:
    static bool matches(const std::string& pattern,
                        const std::string& text);
};

}

#endif
