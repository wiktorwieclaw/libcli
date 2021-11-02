#ifndef LIBCLI_OPTION_HPP
#define LIBCLI_OPTION_HPP

#include <cassert>
#include <memory>
#include <string>

#include "parse.hpp"

namespace libcli {

namespace detail {

struct option_base_t {  // NOLINT(cppcoreguidelines-special-member-functions)
    virtual ~option_base_t() = default;
    virtual auto num_args() const -> std::size_t = 0;
    virtual void parse(std::span<std::string_view> args) const = 0;
};

template <typename T>
struct option_impl_t : option_base_t {
    explicit option_impl_t(T& result) : result{&result} {}

    auto num_args() const -> std::size_t override { return parser_num_args<T>; }

    void parse(std::span<std::string_view> args) const override
    {
        assert(args.size() == num_args());
        *result =
            parser_t<T>::parse(args.template subspan<0, parser_num_args<T>>());
    }

   private:
    T* result;
};

}  // namespace detail

struct option_t {
    template <typename T>
    option_t(std::string_view long_name, std::string_view short_name, T& result)
        : long_name_{long_name.data(), long_name.size()},
          short_name_{short_name.data(), short_name.size()},
          impl{std::make_unique<detail::option_impl_t<T>>(result)}
    {
    }

    auto long_name() const -> std::string_view { return long_name_; }
    auto short_name() const -> std::string_view { return short_name_; }
    auto num_args() const { return impl->num_args(); }

   private:
    void parse(std::span<std::string_view> args) const
    {
        return impl->parse(args);
    }

    friend class cli_t;

    std::string long_name_;
    std::string short_name_;
    std::unique_ptr<detail::option_base_t> impl;
};

}  // namespace libcli

#endif  // LIBCLI_OPTION_HPP
