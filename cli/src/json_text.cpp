#include "json_text.hpp"

#include <format>
#include <string>
#include <string_view>

namespace zpew::cli {

std::string json_string(std::string_view text) {
    constexpr unsigned char kFirstPrintable = 0x20;
    std::string escaped = "\"";
    for (const char c : text) {
        switch (c) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < kFirstPrintable) {
                escaped += std::format("\\u{:04x}", static_cast<unsigned>(c));
            } else {
                escaped += c;
            }
        }
    }
    escaped += '"';
    return escaped;
}

}
