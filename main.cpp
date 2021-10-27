#include <iostream>
#include <optional>

#include "cli.hpp"

// TODO: implement svtoi \
         multiple args for option

auto main(int argc, char* argv[]) -> int
try {
    for (auto i = 1; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;

    auto label = std::string{};
    auto coords = std::pair<std::optional<int>, std::optional<int>>{
        std::nullopt,
        std::nullopt};
    auto is_pretty = false;

    auto cli = cli_t{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(coords, "--coords", "-c");
    cli.add_option(is_pretty, "--pretty", "-p");
    cli.parse(argc, argv);

    if (is_pretty) {
        std::cout << "[PRETTY] ";
    }
    if (!label.empty()) {
        std::cout << label << ": ";
    }
    auto sum = 0;
    auto [x, y] = coords;
    if (x) {
        sum += *x;
    }
    if (y) {
        sum += *y;
    }
    if (x || y) {
        std::cout << sum;
    }
    std::cout << '\n';
}
catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
}