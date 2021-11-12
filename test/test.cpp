#include <catch2/catch_test_macros.hpp>
#include <libcli.hpp>

// TODO: -a -b -c written as -abc or -bac, etc. \
         https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html

using namespace std::string_literals;

TEST_CASE("empty argument list")
{
    auto cli = libcli::cli{};
    REQUIRE_THROWS(cli.parse({}));  // TODO THROWS_AS
}

TEST_CASE("missing positional argument")
{
    auto arg1 = ""s;
    auto cli = libcli::cli{};
    cli.add_argument(arg1);
    REQUIRE_THROWS_AS(cli.parse({"app_name"}), libcli::invalid_input);
}

TEST_CASE("unknown option")
{
    auto cli = libcli::cli{};
    SECTION("by name")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "--unspecified"}),
            libcli::invalid_input);
    }

    SECTION("by shorthand")
    {
        REQUIRE_THROWS_AS(cli.parse({"app_name", "-u"}), libcli::invalid_input);
    }
}

TEST_CASE("parse flag")
{
    auto flag = false;
    auto cli = libcli::cli{};
    cli.add_option(flag, "--flag", "-f");

    SECTION("--flag")
    {
        flag = false;
        cli.parse({"app_name", "--flag"});
        REQUIRE(flag == true);
    }

    SECTION("--flag unrelated")
    {
        flag = false;
        cli.parse({"app_name", "--flag", "unrelated"});
        REQUIRE(flag == true);
    }

    SECTION("--flag=true")
    {
        flag = false;
        cli.parse({"app_name", "--flag=true"});
        REQUIRE(flag == true);
    }

    SECTION("--flag=1")
    {
        flag = false;
        cli.parse({"app_name", "--flag=1"});
        REQUIRE(flag == true);
    }

    SECTION("--flag=false")
    {
        flag = false;
        cli.parse({"app_name", "--flag=false"});
        REQUIRE(flag == false);
    }

    SECTION("--flag=0")
    {
        flag = false;
        cli.parse({"app_name", "--flag=0"});
        REQUIRE(flag == false);
    }

    SECTION("--flag=2")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "--flag=2"}),
            libcli::invalid_input);
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