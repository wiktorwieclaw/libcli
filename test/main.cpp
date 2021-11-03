#include <catch2/catch_test_macros.hpp>

#include <complex>

#include "libcli/cli.hpp"

// TODO: implement svtoi \
         try to use operator>> overloading for custom types

template <typename T>
struct libcli::Parser<std::complex<T>> {
    void operator()(std::string_view arg, std::complex<T>& result)
    {
        auto is = std::istringstream{'(' + std::string{arg} + ')'};
        is >> result;
    }
};

TEST_CASE("main test")
{
    auto argv = std::array{
        "test",
        "--label",
        "coords",
        "discardable",
        "--number",
        "123,5",
        "-p"};

    auto label = std::string{};
    auto number = std::optional<std::complex<int>>{};
    auto is_pretty_print = false;

    auto cli = libcli::Cli{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(number, "--number", "-n");
    cli.add_option(is_pretty_print, "--pretty-print", "-p");
    cli.parse(argv.size(), argv.data());

    REQUIRE(label == "coords");
    REQUIRE(number.has_value());
    REQUIRE(*number == std::complex{123, 5});
    REQUIRE(is_pretty_print == true);
}