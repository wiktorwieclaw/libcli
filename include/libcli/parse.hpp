#ifndef LIBCLI_PARSE_HPP
#define LIBCLI_PARSE_HPP

#include <span>
#include <string>
#include <sstream>

namespace libcli {

template <typename T>
void parse(std::string_view arg, T& result) {
    auto ss = std::stringstream{};
    ss << arg;
    ss >> result;
}

}  // namespace libcli

#endif  // LIBCLI_PARSE_HPP