#include "check_data_command.hpp"
#include "exit_code.hpp"

#include <zpew/error.hpp>
#include <zpew/version.hpp>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kUsage = "usage: zpew-cli --version\n"
                                    "       zpew-cli check-data [--manifest <path>]\n";
constexpr std::string_view kDefaultManifest = "data/manifest.json";

int usage_error() {
    std::cerr << kUsage;
    return zpew::cli::kExitUsageError;
}

int check_data(std::span<const std::string_view> options) {
    std::filesystem::path manifest{kDefaultManifest};
    if (options.size() == 2 && options.front() == "--manifest") {
        manifest = std::filesystem::path{options.back()};
    } else if (!options.empty()) {
        return usage_error();
    }
    return zpew::cli::run_check_data(manifest, std::cout, std::cerr);
}

int run(std::span<const std::string_view> args) {
    if (args.size() == 1 && args.front() == "--version") {
        std::cout << "zpew-cli " << zpew::version() << '\n';
        return zpew::cli::kExitSuccess;
    }
    if (!args.empty() && args.front() == "check-data") {
        return check_data(args.subspan(1));
    }
    return usage_error();
}

}

int main(int argc, char* argv[]) {
    try {
        const std::span<char*> raw{argv, static_cast<std::size_t>(argc)};
        const std::vector<std::string_view> args(raw.begin() + 1, raw.end());
        return run(args);
    } catch (const zpew::ConvergenceError& error) {
        std::cerr << "zpew-cli: " << error.what() << '\n';
        return zpew::cli::kExitConvergenceError;
    } catch (const zpew::Error& error) {
        std::cerr << "zpew-cli: " << error.what() << '\n';
        return zpew::cli::kExitDataError;
    } catch (const std::exception& error) {
        std::cerr << "zpew-cli: unexpected error: " << error.what() << '\n';
        return zpew::cli::kExitUnexpectedError;
    } catch (...) {
        std::cerr << "zpew-cli: unexpected non-standard exception\n";
        return zpew::cli::kExitUnexpectedError;
    }
}
