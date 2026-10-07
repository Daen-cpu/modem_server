#include "command_matcher.hpp"

#include <cstddef>

namespace atmodem {
namespace {

bool char_class_matches(const std::string& pattern,
                        std::size_t& pattern_pos,
                        char value) {

    const std::size_t open = pattern_pos;
    std::size_t pos = open + 1U;
    bool matched = false;
    bool has_items = false;

    while (pos < pattern.size() && pattern[pos] != ']') {
        has_items = true;

        if (pos + 2U < pattern.size() &&
            pattern[pos + 1U] == '-' &&
            pattern[pos + 2U] != ']') {
            const char first = pattern[pos];
            const char last = pattern[pos + 2U];

            if (first <= value && value <= last) matched = true;
            pos += 3U;
        }
        else {
            if (pattern[pos] == value) matched = true;
            ++pos;
        }
    }

    if (pos >= pattern.size() || pattern[pos] != ']' || !has_items) {
        pattern_pos = open;
        return pattern[open] == value;
    }

    pattern_pos = pos;
    return matched;
}

bool match_impl(const std::string& pattern,
                std::size_t pattern_pos,
                const std::string& text,
                std::size_t text_pos) {

    while (pattern_pos < pattern.size()) {
        const char token = pattern[pattern_pos];

        if (token == '*') {
            ++pattern_pos;
            if (pattern_pos == pattern.size()) return true;

            for (std::size_t i = text_pos; i <= text.size(); ++i) {
                if (match_impl(pattern, pattern_pos, text, i)) return true;
            }

            return false;
        }

        if (text_pos >= text.size()) return false;

        if (token == '.') {
            ++pattern_pos;
            ++text_pos;
            continue;
        }

        if (token == '[') {
            std::size_t class_end = pattern_pos;
            const bool matched = char_class_matches(
                pattern, class_end, text[text_pos]);

            if (class_end != pattern_pos) {
                if (!matched) return false;

                pattern_pos = class_end + 1U;
                ++text_pos;
                continue;
            }
        }

        if (token != text[text_pos]) return false;

        ++pattern_pos;
        ++text_pos;
    }

    return text_pos == text.size();
}

}

bool CommandMatcher::matches(const std::string& pattern,
                             const std::string& text) {

    return match_impl(pattern, 0U, text, 0U);
}

}
