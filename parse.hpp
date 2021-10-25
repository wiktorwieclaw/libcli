#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <string>
#include <span>

template <typename T>
auto parse(std::string_view str) -> T = delete;

template <>
auto parse<int>(std::string_view str) -> int
{
    return std::stoi({str.data(), str.size()});
}

template <>
auto parse<std::string>(std::string_view str) -> std::string
{
    return {str.data(), str.size()};
}

#endif  // LIBCLI_PARSE_HPP
