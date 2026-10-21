#pragma once

#include <zpew/data/bar.hpp>

#include <string_view>
#include <vector>

namespace zpew::data {

inline constexpr std::string_view kSnapshotCsvHeader = "date,open,high,low,close,volume";

[[nodiscard]] std::vector<Bar> parse_snapshot_csv(std::string_view text,
                                                  std::string_view source_name);

}
