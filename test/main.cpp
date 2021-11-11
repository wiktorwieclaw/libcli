#include <catch2/catch_test_macros.hpp>
#include <complex>
#include <iostream>
#include <libcli.hpp>

// TODO: -a -b -c written as -abc or -bac, etc. \
         https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html

using namespace std::string_literals;

template <libcli::streamable T>
    requires std::default_initializable<T>
auto operator>>(std::istream& stream, std::optional<T>& o) -> std::istream&
{
    o = T{};
    stream >> *o;
    return stream;
}

TEST_CASE("empty argument list")
{
    auto const input = std::array<char const*, 0>{};

    auto cli = libcli::cli{};
    REQUIRE_THROWS(cli.parse(input.size(), input.data())); // TODO THROWS_AS
}

TEST_CASE("missing positional argument")
{
    auto const input = std::array{"program"};
    auto arg1 = ""s;

    auto cli = libcli::cli{};
    cli.add_argument(arg1);
    REQUIRE_THROWS(cli.parse(input.size(), input.data())); // TODO THROWS_AS
}

TEST_CASE("unknown option")
{
    SECTION("by name")
    {
        auto const input = std::array{"program", "--unspecified"};
        auto cli = libcli::cli{};
        REQUIRE_THROWS(cli.parse(input.size(), input.data())); // TODO THROWS_AS
    }

    SECTION("by shorthand")
    {
        auto const input = std::array{"program", "-u"};
        auto cli = libcli::cli{};
        REQUIRE_THROWS(cli.parse(input.size(), input.data())); // TODO THROWS_AS
    }
}

TEST_CASE("main test")
{
    auto argv = std::array{
        "test",
        "--label",
        "coords",
        "--number",
        "123",
        "first positional",
        "-p",
        "second positional",
        "third positional",
        "fourth positional"};

    auto label = std::string{};
    auto number = std::optional<int>{};
    auto is_pretty_print = false;

    auto pre_multiarg = std::string{};
    auto multiarg = std::vector<std::string>{};
    auto post_multiarg = std::string{};

    auto cli = libcli::cli{};
    cli.add_option(label, "--label", "-l");
    cli.add_option(number, "--number", "-n");
    cli.add_option(is_pretty_print, "--pretty-print", "-p");
    cli.add_argument(pre_multiarg);
    cli.add_argument(multiarg);
    cli.add_argument(post_multiarg);
    cli.parse(argv.size(), argv.data());

    REQUIRE(label == "coords");
    REQUIRE(number.has_value());
    REQUIRE(*number == 123);
    REQUIRE(is_pretty_print == true);
    REQUIRE(pre_multiarg == "first positional");
    REQUIRE(multiarg == std::vector{"second positional"s, "third positional"s});
    REQUIRE(post_multiarg == "fourth positional");
}