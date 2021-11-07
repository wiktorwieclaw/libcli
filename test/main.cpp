#include <catch2/catch_test_macros.hpp>
#include <complex>
#include <iostream>

#include "libcli/cli.hpp"

// TODO: -a -b -c written as -abc or -bac, etc. \
         https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html

template <typename T>
auto operator>>(std::istream& stream, std::optional<T>& o) -> std::istream&
{
    o = T{};
    stream >> *o;
    return stream;
}

TEST_CASE("main test")
{
    auto argv = std::array{
        "test",
        "--label",
        "coords",
        "--number",
        "123",
        "aaa",
        "-p",
        "bbb",
        "ccc"};

    auto label = std::string{};
    auto number = std::optional<int>{};
    auto is_pretty_print = false;

    auto before1 = std::string{};
    auto before2 = std::string{};
    auto from = std::vector<std::string>{};
    auto to = std::string{};

    auto cli = libcli::Cli{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(number, "--number", "-n");
    cli.add_option(is_pretty_print, "--pretty-print", "-p");
    cli.add_argument(before1);
    cli.add_argument(before2);
    cli.add_argument(from);
    cli.add_argument(to);
    cli.parse(argv.size(), argv.data());

    std::cout << before1 << std::endl << std::endl;
    std::cout << before2 << std::endl << std::endl;
    for (auto path : from) {
        std::cout << path << std::endl;
    }
    std::cout << std::endl << to << std::endl;

    REQUIRE(label == "coords");
    REQUIRE(number.has_value());
    REQUIRE(*number == 123);
    REQUIRE(is_pretty_print == true);
}