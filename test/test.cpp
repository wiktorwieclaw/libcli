#include <catch2/catch_test_macros.hpp>
#include <libcli.hpp>

// TODO: https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html

using namespace std::string_literals;

TEST_CASE("empty argument list")
{
    auto cli = libcli::cli{};
    REQUIRE_THROWS_AS(cli.parse({}), std::logic_error);
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
        cli.parse({"app_name", "--flag"});
        REQUIRE(flag == true);
    }

    SECTION("--flag untouched")
    {
        flag = false;
        cli.parse({"app_name", "--flag", "untouched"});
        REQUIRE(flag == true);
    }

    SECTION("--flag=1")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "--flag=1"}),
            libcli::invalid_input);
    }
}

TEST_CASE("parse option")
{
    auto option = std::optional<int>{};
    auto cli = libcli::cli{};
    cli.add_option(option, "--option", "-o");

    SECTION("--option=1")
    {
        cli.parse({"app_name", "--option=1"});
        REQUIRE(option.value() == 1);
    }

    SECTION("--option 1")
    {
        option = std::nullopt;
        cli.parse({"app_name", "--option", "1"});
        REQUIRE(option.value() == 1);
    }

    SECTION("--option")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "--option"}),
            libcli::invalid_input);
    }

    SECTION("-option")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "--option"}),
            libcli::invalid_input);
    }

    SECTION("-o 1")
    {
        option = std::nullopt;
        cli.parse({"app_name", "-o", "1"});
        REQUIRE(option.value() == 1);
    }

    SECTION("-o1")
    {
        option = std::nullopt;
        cli.parse({"app_name", "-o1"});
        REQUIRE(option.value() == 1);
    }

    SECTION("-o")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "-o"}),
            libcli::invalid_input);
    }

    SECTION("--o")
    {
        REQUIRE_THROWS_AS(
            cli.parse({"app_name", "--option"}),
            libcli::invalid_input);
    }
}

TEST_CASE("options terminator")
{
    auto option = false;
    auto arg = ""s;

    auto cli = libcli::cli{};
    cli.add_option(option, "--option", "-o");
    cli.add_argument(arg);

    SECTION("-o arg --")
    {
        cli.parse({"app_name", "-o", "arg", "--"});
        REQUIRE(option == true);
        REQUIRE(arg == "arg");
    }

    SECTION("-o -- -arg")
    {
        option = false;
        arg.clear();

        cli.parse({"app_name", "-o", "--", "-arg"});
        REQUIRE(option == true);
        REQUIRE(arg == "-arg");
    }
}

TEST_CASE("multi-argument")
{
    SECTION("more than one")
    {
        auto multi_arg_1 = std::vector<std::string>{};
        auto multi_arg_2 = std::vector<int>{};

        auto cli = libcli::cli{};
        cli.add_argument(multi_arg_1);
        REQUIRE_THROWS_AS(
            cli.add_argument(multi_arg_2),
            libcli::invalid_cli_definition);
    }

    // TODO
}

TEST_CASE("connected flags")
{
    auto a = false;
    auto b = false;

    auto cli = libcli::cli{};
    cli.add_option(a, "--a", "-a");
    cli.add_option(b, "--b", "-b");
    cli.parse({"app_name", "-ab"});

    REQUIRE(a == true);
    REQUIRE(b == true);

    // TODO add case where one of the joined options is not a flag
}

TEST_CASE("main test")
{
    auto argv = std::array{
        "test",
        "--label",
        "coords",
        "--number=123",
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