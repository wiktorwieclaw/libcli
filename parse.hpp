#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <span>
#include <string>
#include <optional>
#include <utility>

template <typename T>
struct parser_t {
    static constexpr std::size_t num_args = 0;

    static auto parse(std::span<std::string_view> args) -> T = delete;
};

template <>
struct parser_t<bool> {
    static constexpr std::size_t num_args = 0;

    static auto parse(std::span<std::string_view> args) -> bool { return true; }
};

template <>
struct parser_t<int> {
    static constexpr std::size_t num_args = 1;

    static auto parse(std::span<std::string_view> args) -> int
    {
        return std::stoi({args[0].data(), args[0].size()});
    }
};

template <>
struct parser_t<std::string> {
    static constexpr std::size_t num_args = 1;

    static auto parse(std::span<std::string_view> args) -> std::string
    {
        return {args.front().data(), args.front().size()};
    }
};

template <typename T>
struct parser_t<std::optional<T>> {
    static constexpr std::size_t num_args = parser_t<T>::num_args;

    static auto parse(std::span<std::string_view> args) -> std::optional<T>
    {
        return {parser_t<T>::parse(args)};
    }
};

template <typename T, typename U>
struct parser_t<std::pair<T, U>> {
    static constexpr std::size_t num_args =
        parser_t<T>::num_args + parser_t<U>::num_args;

    static auto parse(std::span<std::string_view> args) -> std::pair<T, U>
    {
        return {
            parser_t<T>::parse({args.begin(), args.begin() + 1}),
            parser_t<U>::parse({args.begin() + 1, args.end()})};
    }
};

#endif  // LIBCLI_PARSE_HPP
