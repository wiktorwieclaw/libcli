#ifndef LIBCLI_OPTION_HPP
#define LIBCLI_OPTION_HPP

#include <cassert>
#include <memory>
#include <string>

#include "parse.hpp"

struct var_base_t {  // NOLINT(cppcoreguidelines-special-member-functions)
    virtual ~var_base_t() = default;
    virtual auto num() -> std::size_t = 0;
    virtual void parse(std::span<std::string_view> args) = 0;
};

template <typename T>
struct var_t : var_base_t {
    explicit var_t(T& var) : var{&var} {}

    auto num() -> std::size_t override { return 1; }

    void parse(std::span<std::string_view> args) override
    {
        assert(args.size() == num());
        *var = ::parse<T>(args.front());
    }

   private:
    T* var;
};

template <>
struct var_t<bool> : var_base_t {
    explicit var_t(bool& var) : var{&var} {}

    auto num() -> std::size_t override { return 0; }

    void parse(std::span<std::string_view> args) override
    {
        assert(args.size() == num());
        *var = true;
    }

   private:
    bool* var;
};

struct option_t {
    std::string long_name;
    std::string short_name;
    std::unique_ptr<var_base_t> var;

    template <typename T>
    option_t(std::string_view long_name, std::string_view short_name, T& var)
        : long_name{long_name.data(), long_name.size()},
          short_name{short_name.data(), short_name.size()},
          var{std::make_unique<var_t<T>>(var)}
    {
    }
};

#endif  // LIBCLI_OPTION_HPP
