#include <libcli.hpp>
#include <iostream>
#include <optional>

int main(int argc, char** argv) {
    libcli::cli cli;

    bool flag_a = false;
    cli.add_option(flag_a, "--flag-a", "-a");

    bool flag_b = false;
    cli.add_option(flag_b, "--flag-b", "-b");

    std::optional<int> value;
    cli.add_option(value, "--value", "-v");

    std::vector<std::string> files;
    cli.add_argument(libcli::multi, files);

    int x;
    cli.add_argument(x);

    try { cli.parse(argc, argv); }
    catch (const std::exception& ex) {
        std::cout << ex.what() << '\n' << std::endl;
        return 1;
    }

    std::cout << "passed arguments:\n";
    for (const auto& file : files) {
        std::cout << '\t' << file << '\n';
    }

    std::cout << "\npassed options:\n";
    if (flag_a) { std::cout << "\t--flag-a\n"; }
    if (flag_b) { std::cout << "\t--flag-b\n"; }
    if (value) {
        std::cout << "\t--value, " << *value << '\n';
    }
    std::cout << std::endl;
}
