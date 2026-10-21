#include <zpew/data/bar.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/snapshot_csv.hpp>
#include <zpew/error.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <memory>
#include <string_view>
#include <system_error>
#include <vector>

namespace zpew::data {

namespace {

constexpr std::string_view kByteOrderMark = "\xEF\xBB\xBF";
constexpr std::size_t kFieldCount = 6;
constexpr std::array<std::string_view, kFieldCount> kColumnNames{"date", "open",  "high",
                                                                 "low",  "close", "volume"};

struct Line {
    std::size_t number;
    std::string_view text;
};

class ParseContext {
public:
    explicit ParseContext(std::string_view source_name) : source_name_{source_name} {}

    [[noreturn]] void fail(std::size_t line_number, std::string_view message) const {
        throw DataError{std::format("{}:{}: {}", source_name_, line_number, message)};
    }

    [[noreturn]] void fail(std::string_view message) const {
        throw DataError{std::format("{}: {}", source_name_, message)};
    }

private:
    std::string_view source_name_;
};

std::vector<Line> split_lines(std::string_view text) {
    std::vector<Line> lines;
    std::size_t start = 0;
    std::size_t number = 1;
    while (start < text.size()) {
        const std::size_t end = text.find('\n', start);
        std::string_view line =
            end == std::string_view::npos ? text.substr(start) : text.substr(start, end - start);
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }
        lines.push_back({.number = number, .text = line});
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
        ++number;
    }
    return lines;
}

std::array<std::string_view, kFieldCount> split_fields(const Line& line,
                                                       const ParseContext& context) {
    std::array<std::string_view, kFieldCount> fields{};
    std::string_view rest = line.text;
    for (std::size_t i = 0; i < kFieldCount; ++i) {
        const std::size_t comma = rest.find(',');
        const bool is_last = i + 1 == kFieldCount;
        if (is_last != (comma == std::string_view::npos)) {
            context.fail(line.number,
                         std::format("expected {} comma-separated fields", kFieldCount));
        }
        fields.at(i) = is_last ? rest : rest.substr(0, comma);
        rest = is_last ? std::string_view{} : rest.substr(comma + 1);
    }
    return fields;
}

double parse_number(std::string_view field, std::size_t column, const Line& line,
                    const ParseContext& context) {
    double value = 0.0;
    const char* const end = std::to_address(field.end());
    const auto [parsed_end, error] = std::from_chars(std::to_address(field.begin()), end, value);
    if (field.empty() || error != std::errc{} || parsed_end != end) {
        context.fail(line.number, std::format("invalid number '{}' in column {}", field,
                                              kColumnNames.at(column)));
    }
    return value;
}

Bar parse_row(const Line& line, const ParseContext& context) {
    const auto fields = split_fields(line, context);
    const auto date = parse_iso_date(fields[0]);
    if (!date) {
        context.fail(line.number, std::format("invalid date '{}', expected YYYY-MM-DD", fields[0]));
    }
    return Bar{.date = *date,
               .open = parse_number(fields[1], 1, line, context),
               .high = parse_number(fields[2], 2, line, context),
               .low = parse_number(fields[3], 3, line, context),
               .close = parse_number(fields[4], 4, line, context),
               .volume = parse_number(fields[5], 5, line, context)};
}

}

std::vector<Bar> parse_snapshot_csv(std::string_view text, std::string_view source_name) {
    const ParseContext context{source_name};
    if (text.starts_with(kByteOrderMark)) {
        context.fail("byte-order mark is not allowed");
    }
    const std::vector<Line> lines = split_lines(text);
    if (lines.empty()) {
        context.fail("missing header");
    }
    if (lines.front().text != kSnapshotCsvHeader) {
        context.fail(1, std::format("expected header '{}'", kSnapshotCsvHeader));
    }

    std::vector<Bar> bars;
    bars.reserve(lines.size() - 1);
    for (std::size_t i = 1; i < lines.size(); ++i) {
        const Line& line = lines[i];
        if (line.text.empty()) {
            context.fail(line.number, "blank line");
        }
        bars.push_back(parse_row(line, context));
    }
    return bars;
}

}
