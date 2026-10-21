#pragma once

#include <zpew/data/trading_calendar.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace zpew::data {

inline constexpr int kManifestSchemaVersion = 1;

struct ManifestAsset {
    std::string symbol;
    std::string name;
    std::string asset_class;
    TradingCalendar calendar{TradingCalendar::kWeekdays};
    std::string source;
    std::string source_url;
    std::string terms_url;
    std::string fetched_at;
    std::chrono::sys_days first_date;
    std::chrono::sys_days last_date;
    std::size_t rows{};
    std::filesystem::path file;
    std::string sha256;
};

class Manifest {
public:
    Manifest(std::filesystem::path directory, std::vector<ManifestAsset> assets);

    [[nodiscard]] const std::filesystem::path& directory() const noexcept;
    [[nodiscard]] const std::vector<ManifestAsset>& assets() const noexcept;
    [[nodiscard]] const ManifestAsset& find(std::string_view symbol) const;
    [[nodiscard]] std::filesystem::path snapshot_path(const ManifestAsset& asset) const;

private:
    std::filesystem::path directory_;
    std::vector<ManifestAsset> assets_;
};

[[nodiscard]] Manifest parse_manifest(std::string_view json_text,
                                      const std::filesystem::path& directory,
                                      std::string_view source_name);
[[nodiscard]] Manifest load_manifest(const std::filesystem::path& manifest_path);
[[nodiscard]] std::optional<std::filesystem::path>
find_manifest(const std::filesystem::path& start);

}
