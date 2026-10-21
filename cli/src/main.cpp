#include "check_data_command.hpp"
#include "exit_code.hpp"
#include "fit_command.hpp"
#include "forecast_command.hpp"
#include "manifest_path.hpp"
#include "options.hpp"

#include <zpew/error.hpp>
#include <zpew/version.hpp>

#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kUsage =
    "usage: zpew-cli --version\n"
    "       zpew-cli check-data [--manifest <path>]\n"
    "       zpew-cli forecast --symbol <symbol> --model ewma [--lambda 0.94] [--seed 21]\n"
    "                         [--manifest <path>]\n"
    "       zpew-cli forecast --symbol <symbol> --model realised [--window 21]\n"
    "                         [--manifest <path>]\n"
    "       zpew-cli forecast --symbol <symbol> --model garch [--fit-window 1000]\n"
    "                         [--targeting on|off] [--manifest <path>]\n"
    "       zpew-cli fit --symbol <symbol> --model garch [--targeting on|off] [--manifest "
    "<path>]\n";

int run(std::span<const std::string_view> args) {
    if (args.size() == 1 && args.front() == "--version") {
        std::cout << "zpew-cli " << zpew::version() << '\n';
        return zpew::cli::kExitSuccess;
    }
    if (args.empty()) {
        throw zpew::cli::UsageError{"missing command"};
    }
    const std::string_view command = args.front();
    const auto rest = args.subspan(1);
    if (command == "check-data") {
        const zpew::cli::Options options{rest, {"manifest"}};
        return zpew::cli::run_check_data(zpew::cli::resolve_manifest(options.get("manifest")),
                                         std::cout, std::cerr);
    }
    if (command == "forecast") {
        const zpew::cli::Options options{
            rest,
            {"symbol", "model", "lambda", "seed", "window", "fit-window", "targeting", "manifest"}};
        return zpew::cli::run_forecast(options, std::cout);
    }
    if (command == "fit") {
        const zpew::cli::Options options{rest, {"symbol", "model", "targeting", "manifest"}};
        return zpew::cli::run_fit(options, std::cout);
    }
    throw zpew::cli::UsageError{std::string{"unknown command '"} + std::string{command} + "'"};
}

}

int main(int argc, char* argv[]) {
    try {
        const std::span<char*> raw{argv, static_cast<std::size_t>(argc)};
        const std::vector<std::string_view> args(raw.begin() + 1, raw.end());
        return run(args);
    } catch (const zpew::cli::UsageError& error) {
        std::cerr << "zpew-cli: " << error.what() << '\n' << kUsage;
        return zpew::cli::kExitUsageError;
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
