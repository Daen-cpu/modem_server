#ifndef AT_MODEM_EXPECT_TABLE_HPP
#define AT_MODEM_EXPECT_TABLE_HPP

#include <string>
#include <vector>

namespace atmodem {

struct ExpectRule {
    std::string pattern;
    std::string answer;
};

class ExpectTable {
public:
    bool load(const std::string& filename, std::string& error);
    const ExpectRule* find(const std::string& command) const;

private:
    std::vector<ExpectRule> rules_;
};

std::string decode_escapes(const std::string& value);

}

#endif
