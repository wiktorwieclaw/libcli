#include <iostream>

#include "cli.hpp"

// TODO: support std::optional \
         implement svtoi \
         multiple args for option

auto main(int argc, char* argv[]) -> int
try {
    for (auto i = 1; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;

    auto label = std::string{};
    auto x = INT_MIN;
    auto y = INT_MIN;
    auto is_pretty = false;

    auto cli = cli_t{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(x, "--x", "-x");
    cli.add_option(y, "--y", "-y");
    cli.add_option(is_pretty, "--pretty", "-p");
    cli.parse(argc, argv);

    if (is_pretty) { std::cout << "[PRETTY] "; }
    if (!label.empty()) { std::cout << label << ": "; }
    auto sum = 0;
    if (x != INT_MIN) { sum += x; }
    if (y != INT_MIN) { sum += y; }
    if (x != INT_MIN || y != INT_MIN) { std::cout << sum; }
    std::cout << '\n';
}
catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
}