#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <span>
#include <string>

#include "parser_utils.hpp"

namespace libcli {

template <typename T>
struct parser_t;

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
    static auto parse(std::span<std::string_view, parser_num_args<T>> args)
        -> std::optional<T>
    {
        return {parser_t<T>::parse(args)};
    }
};

template <typename T, typename U>
struct parser_t<std::pair<T, U>> {
    static auto parse(
        std::span<std::string_view, parser_num_args<T> + parser_num_args<U>>
            args) -> std::pair<T, U>
    {
        return {
            parser_t<T>::parse(args.template subspan<0, parser_num_args<T>>()),
            parser_t<U>::parse(args.template subspan<
                               parser_num_args<T>,
                               parser_num_args<U>>())};
    }
};

}  // namespace libcli

#endif  // LIBCLI_PARSE_HPP