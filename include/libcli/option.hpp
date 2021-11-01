#ifndef LIBCLI_OPTION_HPP
#define LIBCLI_OPTION_HPP

#include <cassert>
#include <memory>
#include <string>

#include "parse.hpp"

struct option_base_t {  // NOLINT(cppcoreguidelines-special-member-functions)
    virtual ~option_base_t() = default;
    virtual auto num_args() const -> std::size_t = 0;
    virtual void parse(std::span<std::string_view> args) const = 0;
};

template <typename T>
struct option_impl_t : option_base_t {
    explicit option_impl_t(T& result) : result{&result} {}

    auto num_args() const -> std::size_t override { return ::num_args<T>; }

    void parse(std::span<std::string_view> args) const override
    {
        assert(args.size() == num_args());
        *result = parser_t<T>::parse(args.template subspan<0, ::num_args<T>>());
    }

   private:
    T* result;
};

struct option_t {
    template <typename T>
    option_t(std::string_view long_name, std::string_view short_name, T& result)
        : long_name_{long_name.data(), long_name.size()},
          short_name_{short_name.data(), short_name.size()},
          impl{std::make_unique<option_impl_t<T>>(result)}
    {
    }

    auto long_name() const -> std::string_view { return long_name_; }
    auto short_name() const -> std::string_view { return short_name_; }
    void parse(std::span<std::string_view> args) const
    {
        return impl->parse(args);
    }
    auto num_args() const { return impl->num_args(); }

   private:
    std::string long_name_;
    std::string short_name_;
    std::unique_ptr<option_base_t> impl;
};

#endif  // LIBCLI_OPTION_HPP
