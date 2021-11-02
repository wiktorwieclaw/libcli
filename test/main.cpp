#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <optional>

#include "libcli/cli.hpp"

// TODO: implement svtoi \
         multiple args for option

TEST_CASE("main test")
{
    auto argv = std::array{
        "test",
        "--label",
        "coords",
        "discardable",
        "--number",
        "1",
        "-p"};

    auto label = std::string{};
    auto number = INT_MIN;
    auto is_pretty_print = false;

    auto cli = libcli::Cli{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(number, "--number", "-n");
    cli.add_option(is_pretty_print, "--pretty-print", "-p");
    cli.parse(argv.size(), argv.data());

    REQUIRE(label == "coords");
    REQUIRE(number == 1);
    REQUIRE(is_pretty_print == true);
}