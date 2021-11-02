#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <span>
#include <string>

namespace libcli {

template <typename T>
auto parse(std::string_view arg) -> T = delete;

template <>
auto parse<bool>(std::string_view arg) -> bool
{
    return true;
}

template <>
auto parse<int>(std::string_view arg) -> int
{
    // TODO svtoi
    return std::stoi({arg.data(), arg.size()});
}

template <>
auto parse<std::string>(std::string_view arg) -> std::string
{
    return {arg.data(), arg.size()};
}

//template <typename T>
//auto parse<std::optional<T>>(std::string_view arg) -> std::optional<T>
//{
//    return {parse<T>(arg)};
//}

}  // namespace libcli

#endif  // LIBCLI_PARSE_HPP