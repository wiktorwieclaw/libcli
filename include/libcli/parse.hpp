#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <span>
#include <string>
#include <sstream>

namespace libcli {

template <typename T>
struct Parser {
    void operator()(std::string_view arg, T& result)
    {
        auto ss = std::stringstream{};
        ss << arg;
        ss >> result;
    }
};

template <typename T>
struct Parser<std::optional<T>> {
    void operator()(std::string_view arg, std::optional<T>& result)
    {
        result = T{};
        Parser<T>{}(arg, *result);
    }
};
}  // namespace libcli

#endif  // LIBCLI_PARSE_HPP