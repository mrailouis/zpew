#include "eval_config.hpp"

#include "parallel.hpp"

#include <zpew/error.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/io/binary_file.hpp>
#include <zpew/io/sha256.hpp>
#include <zpew/models/model_spec.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace zpew::cli {

namespace {

class ConfigReader {
public:
    explicit ConfigReader(std::string source) : source_{std::move(source)} {}

    [[noreturn]] void fail(std::string_view message) const {
        throw DataError{std::format("{}: {}", source_, message)};
    }

    void require_keys(const toml::table& table, std::string_view where,
                      std::initializer_list<std::string_view> allowed) const {
        for (const auto& [key, value] : table) {
            if (std::ranges::find(allowed, key.str()) == allowed.end()) {
                fail(std::format("unknown key '{}' in {}", key.str(), where));
            }
        }
    }

    [[nodiscard]] std::string text(const toml::table& table, std::string_view key) const {
        const auto value = table[key].value<std::string>();
        if (!value || value->empty()) {
            fail(std::format("'{}' must be a non-empty string", key));
        }
        return *value;
    }

    [[nodiscard]] std::size_t count(const toml::table& table, std::string_view key,
                                    std::int64_t minimum) const {
        const auto value = table[key].value<std::int64_t>();
        if (!value || *value < minimum) {
            fail(std::format("'{}' must be an integer of at least {}", key, minimum));
        }
        return static_cast<std::size_t>(*value);
    }

    [[nodiscard]] const toml::table& table(const toml::table& parent, std::string_view key) const {
        const toml::table* child = parent[key].as_table();
        if (child == nullptr) {
            fail(std::format("missing table [{}]", key));
        }
        return *child;
    }

    [[nodiscard]] std::vector<std::string> strings(const toml::table& table,
                                                   std::string_view key) const {
        const toml::array* array = table[key].as_array();
        if (array == nullptr) {
            fail(std::format("'{}' must be an array of strings", key));
        }
        std::vector<std::string> values;
        for (const toml::node& node : *array) {
            const auto value = node.value<std::string>();
            if (!value) {
                fail(std::format("'{}' must contain only strings", key));
            }
            values.push_back(*value);
        }
        return values;
    }

private:
    std::string source_;
};

models::ModelSpec parse_model(const toml::table& table, const ConfigReader& reader) {
    const std::string name = reader.text(table, "name");
    const std::string type = reader.text(table, "type");
    const std::string where = std::format("model '{}'", name);
    if (type == "realised") {
        reader.require_keys(table, where, {"name", "type", "window"});
        return {.name = name,
                .parameters = models::RealisedSpec{.window = reader.count(table, "window", 1)}};
    }
    if (type == "ewma") {
        reader.require_keys(table, where, {"name", "type", "lambda", "seed_window"});
        const auto lambda = table["lambda"].value<double>();
        if (!lambda) {
            reader.fail(std::format("{}: 'lambda' must be a number", where));
        }
        return {.name = name,
                .parameters = models::EwmaSpec{
                    .lambda = *lambda, .seed_window = reader.count(table, "seed_window", 1)}};
    }
    if (type == "garch") {
        reader.require_keys(table, where, {"name", "type", "variance_targeting"});
        const auto targeting = table["variance_targeting"].value<bool>();
        if (!targeting) {
            reader.fail(std::format("{}: 'variance_targeting' must be true or false", where));
        }
        return {.name = name, .parameters = models::GarchSpec{.variance_targeting = *targeting}};
    }
    reader.fail(
        std::format("{}: unknown type '{}'; expected realised, ewma or garch", where, type));
}

std::filesystem::path resolve(const std::filesystem::path& base, const std::string& value) {
    const std::filesystem::path path{value};
    return path.is_absolute() ? path : (base / path).lexically_normal();
}

void fill_evaluation(EvalConfig& config, const toml::table& root, const ConfigReader& reader) {
    const toml::table& evaluation = reader.table(root, "evaluation");
    reader.require_keys(evaluation, "[evaluation]", {"window", "refit_every", "window_modes"});
    config.window = reader.count(evaluation, "window", 1);
    config.refit_every = reader.count(evaluation, "refit_every", 1);
    for (const std::string& mode : reader.strings(evaluation, "window_modes")) {
        const auto parsed = evaluation::parse_window_mode(mode);
        if (!parsed) {
            reader.fail(
                std::format("unknown window mode '{}'; expected sliding or expanding", mode));
        }
        config.window_modes.push_back(*parsed);
    }
    if (config.window_modes.empty()) {
        reader.fail("'window_modes' must name at least one mode");
    }
}

void fill_models(EvalConfig& config, const toml::table& root, const ConfigReader& reader) {
    const toml::array* models = root["models"].as_array();
    if (models == nullptr || models->empty()) {
        reader.fail("at least one [[models]] entry is required");
    }
    std::set<std::string, std::less<>> names;
    for (const toml::node& node : *models) {
        const toml::table* table = node.as_table();
        if (table == nullptr) {
            reader.fail("each [[models]] entry must be a table");
        }
        models::ModelSpec spec = parse_model(*table, reader);
        if (!names.insert(spec.name).second) {
            reader.fail(std::format("duplicate model name '{}'", spec.name));
        }
        config.models.push_back(std::move(spec));
    }
    if (!names.contains(config.baseline)) {
        reader.fail(std::format("baseline '{}' is not one of the models", config.baseline));
    }
}

}

EvalConfig load_eval_config(const std::filesystem::path& path) {
    const std::string bytes = io::read_binary_file(path);
    const ConfigReader reader{path.generic_string()};
    toml::table root;
    try {
        root = toml::parse(bytes, path.generic_string());
    } catch (const toml::parse_error& error) {
        reader.fail(std::format("invalid TOML: {}", error.description()));
    }
    reader.require_keys(
        root, "the top level",
        {"name", "manifest", "output", "assets", "threads", "baseline", "evaluation", "models"});

    const std::filesystem::path base = path.parent_path();
    EvalConfig config;
    config.path = path;
    config.sha256 = io::to_hex(io::sha256(bytes));
    config.name = reader.text(root, "name");
    config.manifest = resolve(base, reader.text(root, "manifest"));
    config.output = resolve(base, reader.text(root, "output"));
    config.assets = reader.strings(root, "assets");
    const std::size_t threads = reader.count(root, "threads", 0);
    config.threads = threads == 0 ? default_thread_count() : threads;
    config.baseline = reader.text(root, "baseline");
    fill_evaluation(config, root, reader);
    fill_models(config, root, reader);
    return config;
}

}
