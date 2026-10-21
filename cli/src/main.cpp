#include <zpew/version.hpp>

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

namespace {

constexpr std::string_view kUsage = "usage: zpew-cli --version\n";

}

int main(int argc, char* argv[]) {
    const std::span<char*> args{argv, static_cast<std::size_t>(argc)};

    if (args.size() == 2 && std::string_view{args.back()} == "--version") {
        std::cout << "zpew-cli " << zpew::version() << '\n';
        return EXIT_SUCCESS;
    }

    std::cerr << kUsage;
    return EXIT_FAILURE;
}
