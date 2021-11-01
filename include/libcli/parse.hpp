#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <span>
#include <string>

template <typename T>
struct parser_t {
    static auto parse(std::span<std::string_view, 0> args) -> T = delete;
};

template <typename T, std::size_t size>
constexpr auto num_args_impl(T (* /**/)(std::span<std::string_view, size>))
{
    return size;
}

template <typename T>
constexpr auto num_args = num_args_impl(parser_t<T>::parse);

template <>
struct parser_t<bool> {
    static auto parse(std::span<std::string_view, 0> args) { return true; }
};

template <>
struct parser_t<int> {
    static auto parse(std::span<std::string_view, 1> args)
    {
        return std::stoi({args.front().data(), args.front().size()});
    }
};

template <>
struct parser_t<std::string> {
    static auto parse(std::span<std::string_view, 1> args) -> std::string
    {
        return {args.front().data(), args.front().size()};
    }
};

template <typename T>
struct parser_t<std::optional<T>> {
    static auto parse(std::span<std::string_view, num_args<T>> args)
        -> std::optional<T>
    {
        return {parser_t<T>::parse(args)};
    }
};

template <typename T, typename U>
struct parser_t<std::pair<T, U>> {
    static auto parse(
        std::span<std::string_view, num_args<T> + num_args<U>> args)
        -> std::pair<T, U>
    {
        return {
            parser_t<T>::parse(args.template subspan<0, num_args<T>>()),
            parser_t<U>::parse(
                args.template subspan<num_args<T>, num_args<U>>())};
    }
};

#endif  // LIBCLI_PARSE_HPP
