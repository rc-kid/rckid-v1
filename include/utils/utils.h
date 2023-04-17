#pragma once

#include <cassert>
#include <sstream>

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()

#define UNREACHABLE throw std::runtime_error{STR("Unreachable code triggered at " << __FILE__ << ":" << __LINE__)};
#define UNIMPLEMENTED throw std::runtime_error{STR("Unimplemented code triggered at " << __FILE__ << ":" << __LINE__)};

#define ASSERT(...) assert(__VA_ARGS__)

inline bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

namespace str {

    inline void trimLeft(std::string &s) {
        size_t i = 0;
        while (i < s.size() && isWhitespace(s[i]))
            ++i;
        s.erase(s.begin(), s.begin() + i);
    }

    inline void trimRight(std::string &s) {
        size_t i = s.size();
        while (i > 0 && isWhitespace(s[i -1]))
            --i;
        s.erase(s.begin() + i, s.end());
    }

    inline void trim(std::string & s) {
        trimLeft(s);
        trimRight(s);
    }

    inline std::string escape(std::string const & str) {
        std::stringstream s;
        for (char c : str) {
            switch (c) {
                case '"':
                case '\'':
                case '\\':
                    s << '\\' << c;
                    break;
                case '\n':
                    s << "\\n";
                    break;
                case '\t':
                    s << "\\t";
                    break;
                case '\r':
                    s << "\\r";
                    break;
                case '\0':
                    s << "\\0";
                    break;
                default:
                    s << c;
                    break;
            }
        }
        return s.str();
    }

} // namespace str