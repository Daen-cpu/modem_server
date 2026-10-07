#include "expect_table.hpp"
#include "command_matcher.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace atmodem {
namespace {

std::string trim(const std::string& value) {

    std::size_t first = 0U;

    while (first < value.size() &&
           std::isspace(static_cast<unsigned char>(value[first])) != 0) ++first;

    std::size_t last = value.size();

    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1U])) != 0) --last;

    return value.substr(first, last - first);
}

}

std::string decode_escapes(const std::string& value) {

    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0U; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1U >= value.size()) {
            result += value[i];
            continue;
        }

        ++i;

        switch (value[i]) {
        case 'r': result += '\r'; break;
        case 'n': result += '\n'; break;
        case 't': result += '\t'; break;
        case '\\': result += '\\'; break;
        default:
            result += '\\';
            result += value[i];
            break;
        }
    }

    return result;
}

bool ExpectTable::load(const std::string& filename, std::string& error) {

    std::ifstream input(filename.c_str());

    if (!input) {
        error = "cannot open configuration file: " + filename;
        return false;
    }

    rules_.clear();

    std::string line;
    std::size_t line_number = 0U;

    while (std::getline(input, line)) {
        ++line_number;
        line = trim(line);

        if (line.empty() || line[0] == '#') continue;

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            std::ostringstream message;
            message << "invalid config line " << line_number << ": expected pattern=answer";
            error = message.str();
            return false;
        }

        const std::string pattern = trim(line.substr(0U, separator));
        const std::string answer = line.substr(separator + 1U);

        if (pattern.empty()) {
            std::ostringstream message;
            message << "invalid config line " << line_number << ": empty pattern";
            error = message.str();
            return false;
        }

        ExpectRule rule;
        rule.pattern = decode_escapes(pattern);
        rule.answer = decode_escapes(answer);
        rules_.push_back(rule);
    }

    if (rules_.empty()) {
        error = "configuration file contains no rules";
        return false;
    }

    return true;
}

const ExpectRule* ExpectTable::find(const std::string& command) const
{
    for (std::vector<ExpectRule>::const_iterator it = rules_.begin(); it != rules_.end(); ++it) {
        if (CommandMatcher::matches(it->pattern, command)) return &(*it);
    }

    return NULL;
}

}
