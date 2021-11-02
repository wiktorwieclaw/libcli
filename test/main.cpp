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
        "--coords",
        "1",
        "2",
        "-p"};

    auto label = std::string{};
    auto coords = std::pair<std::optional<int>, std::optional<int>>{
        std::nullopt,
        std::nullopt};
    auto is_pretty = false;

    auto cli = libcli::cli_t{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(coords, "--coords", "-c");
    cli.add_option(is_pretty, "--pretty", "-p");
    cli.parse(argv.size(), argv.data());

    REQUIRE(label == "coords");
    REQUIRE(coords == std::pair{std::optional{1}, std::optional{2}});
    REQUIRE(is_pretty == true);
}