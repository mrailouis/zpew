#include <zpew/data/iso_date.hpp>
#include <zpew/data/manifest.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/error.hpp>
#include <zpew/io/binary_file.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace zpew::data {

namespace {

using Json = nlohmann::json;

constexpr std::array<std::string_view, 2> kTopLevelKeys{"schema_version", "assets"};
constexpr std::array<std::string_view, 13> kAssetKeys{
    "symbol",     "name",       "asset_class", "calendar", "source", "source_url", "terms_url",
    "fetched_at", "first_date", "last_date",   "rows",     "file",   "sha256"};
constexpr std::size_t kSha256HexLength = 64;
constexpr std::string_view kTimestampPattern = "YYYY-MM-DDTHH:MM:SSZ";

class ManifestContext {
public:
    explicit ManifestContext(std::string_view source_name) : source_name_{source_name} {}

    [[noreturn]] void fail(std::string_view message) const {
        throw DataError{std::format("{}: {}", source_name_, message)};
    }

    [[noreturn]] void fail(std::size_t asset_index, std::string_view message) const {
        throw DataError{std::format("{}: assets[{}]: {}", source_name_, asset_index, message)};
    }

    [[noreturn]] void operator()(std::string_view message) const { fail(message); }

private:
    std::string_view source_name_;
};

class AssetContext {
public:
    AssetContext(const ManifestContext& manifest, std::size_t index)
        : manifest_{&manifest}, index_{index} {}

    [[noreturn]] void operator()(std::string_view message) const {
        manifest_->fail(index_, message);
    }

private:
    const ManifestContext* manifest_;
    std::size_t index_;
};

template <std::size_t N>
void require_exact_keys(const Json& object, const std::array<std::string_view, N>& expected,
                        const auto& fail) {
    for (const std::string_view key : expected) {
        if (!object.contains(key)) {
            fail(std::format("missing key '{}'", key));
        }
    }
    for (const auto& [key, value] : object.items()) {
        if (std::ranges::find(expected, key) == expected.end()) {
            fail(std::format("unexpected key '{}'", key));
        }
    }
}

bool is_lower_hex(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

bool is_ascii_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

int two_digits(std::string_view text, std::size_t offset) {
    return ((text[offset] - '0') * 10) + (text[offset + 1] - '0');
}

bool is_utc_timestamp(std::string_view text) {
    if (text.size() != kTimestampPattern.size()) {
        return false;
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char expected = kTimestampPattern[i];
        const bool is_placeholder =
            std::string_view{"YMDHS"}.find(expected) != std::string_view::npos;
        if (is_placeholder ? !is_ascii_digit(text[i]) : text[i] != expected) {
            return false;
        }
    }
    constexpr int kHoursPerDay = 24;
    constexpr int kMinutesPerHour = 60;
    constexpr int kSecondsPerMinute = 60;
    return parse_iso_date(text.substr(0, 10)).has_value() && two_digits(text, 11) < kHoursPerDay &&
           two_digits(text, 14) < kMinutesPerHour && two_digits(text, 17) < kSecondsPerMinute;
}

bool stays_inside(const std::filesystem::path& relative) {
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() ||
        relative.has_root_directory()) {
        return false;
    }
    return std::ranges::none_of(
        relative, [](const std::filesystem::path& part) { return part == ".." || part == "."; });
}

std::string required_string(const Json& asset, std::string_view key, const auto& fail) {
    const Json& value = asset.at(key);
    if (!value.is_string() || value.get_ref<const std::string&>().empty()) {
        fail(std::format("'{}' must be a non-empty string", key));
    }
    return value.get<std::string>();
}

std::chrono::sys_days required_date(const Json& asset, std::string_view key, const auto& fail) {
    const std::string text = required_string(asset, key, fail);
    const auto date = parse_iso_date(text);
    if (!date) {
        fail(std::format("'{}' must be a YYYY-MM-DD date, got '{}'", key, text));
    }
    return *date;
}

ManifestAsset parse_asset(const Json& json, std::size_t index, const ManifestContext& context) {
    const AssetContext fail{context, index};
    if (!json.is_object()) {
        fail("must be an object");
    }
    require_exact_keys(json, kAssetKeys, fail);

    ManifestAsset asset;
    asset.symbol = required_string(json, "symbol", fail);
    asset.name = required_string(json, "name", fail);
    asset.asset_class = required_string(json, "asset_class", fail);
    asset.source = required_string(json, "source", fail);
    asset.source_url = required_string(json, "source_url", fail);
    asset.terms_url = required_string(json, "terms_url", fail);

    const std::string calendar = required_string(json, "calendar", fail);
    const auto parsed_calendar = parse_trading_calendar(calendar);
    if (!parsed_calendar) {
        fail(std::format("'calendar' must be 'weekdays' or 'daily', got '{}'", calendar));
    }
    asset.calendar = *parsed_calendar;

    asset.fetched_at = required_string(json, "fetched_at", fail);
    if (!is_utc_timestamp(asset.fetched_at)) {
        fail(std::format("'fetched_at' must look like {}, got '{}'", kTimestampPattern,
                         asset.fetched_at));
    }

    asset.first_date = required_date(json, "first_date", fail);
    asset.last_date = required_date(json, "last_date", fail);
    if (asset.last_date <= asset.first_date) {
        fail("'last_date' must be after 'first_date'");
    }

    const Json& rows = json.at("rows");
    if (!rows.is_number_unsigned() || rows.get<std::size_t>() < 2) {
        fail("'rows' must be an integer of at least 2");
    }
    asset.rows = rows.get<std::size_t>();

    asset.file = std::filesystem::path{required_string(json, "file", fail)};
    if (!stays_inside(asset.file)) {
        fail(std::format("'file' must be a relative path inside the manifest directory, got '{}'",
                         asset.file.generic_string()));
    }

    asset.sha256 = required_string(json, "sha256", fail);
    if (asset.sha256.size() != kSha256HexLength ||
        !std::ranges::all_of(asset.sha256, is_lower_hex)) {
        fail("'sha256' must be 64 lower-case hexadecimal characters");
    }
    return asset;
}

Json parse_json(std::string_view json_text, const ManifestContext& context) {
    try {
        return Json::parse(json_text);
    } catch (const Json::parse_error& error) {
        context.fail(std::format("invalid JSON: {}", error.what()));
    }
}

}

Manifest::Manifest(std::filesystem::path directory, std::vector<ManifestAsset> assets)
    : directory_{std::move(directory)}, assets_{std::move(assets)} {}

const std::filesystem::path& Manifest::directory() const noexcept {
    return directory_;
}

const std::vector<ManifestAsset>& Manifest::assets() const noexcept {
    return assets_;
}

const ManifestAsset& Manifest::find(std::string_view symbol) const {
    const auto it = std::ranges::find(assets_, symbol, &ManifestAsset::symbol);
    if (it == assets_.end()) {
        throw DataError{std::format("unknown symbol '{}'", symbol)};
    }
    return *it;
}

std::filesystem::path Manifest::snapshot_path(const ManifestAsset& asset) const {
    return directory_ / asset.file;
}

Manifest parse_manifest(std::string_view json_text, const std::filesystem::path& directory,
                        std::string_view source_name) {
    const ManifestContext context{source_name};
    const Json root = parse_json(json_text, context);
    if (!root.is_object()) {
        context.fail("top level must be an object");
    }
    require_exact_keys(root, kTopLevelKeys, context);

    const Json& version = root.at("schema_version");
    if (!version.is_number_integer() || version.get<int>() != kManifestSchemaVersion) {
        context.fail(std::format("'schema_version' must be {}", kManifestSchemaVersion));
    }
    const Json& assets_json = root.at("assets");
    if (!assets_json.is_array() || assets_json.empty()) {
        context.fail("'assets' must be a non-empty array");
    }

    std::vector<ManifestAsset> assets;
    assets.reserve(assets_json.size());
    std::set<std::string, std::less<>> symbols;
    std::set<std::string, std::less<>> files;
    for (std::size_t i = 0; i < assets_json.size(); ++i) {
        ManifestAsset asset = parse_asset(assets_json.at(i), i, context);
        if (!symbols.insert(asset.symbol).second) {
            context.fail(i, std::format("duplicate symbol '{}'", asset.symbol));
        }
        if (!files.insert(asset.file.generic_string()).second) {
            context.fail(i, std::format("duplicate file '{}'", asset.file.generic_string()));
        }
        assets.push_back(std::move(asset));
    }
    return Manifest{directory, std::move(assets)};
}

Manifest load_manifest(const std::filesystem::path& manifest_path) {
    const std::string text = io::read_binary_file(manifest_path);
    return parse_manifest(text, manifest_path.parent_path(), manifest_path.generic_string());
}

}
