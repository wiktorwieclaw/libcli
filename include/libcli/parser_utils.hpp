#ifndef LIBCLI_PARSER_UTILS_HPP
#define LIBCLI_PARSER_UTILS_HPP

namespace libcli {

template <typename T>
struct parser_t;

namespace detail {

template <typename T, std::size_t size>
constexpr auto parser_num_args_impl(
    [[maybe_unused]] T (*parse)(std::span<std::string_view, size>))
{
    return size;
}

}  // namespace detail

template <typename T>
constexpr auto parser_num_args =
    detail::parser_num_args_impl(parser_t<T>::parse);

}  // namespace libcli

#endif  // LIBCLI_PARSER_UTILS_HPP
