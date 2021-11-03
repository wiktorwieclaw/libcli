#ifndef LIBCLI_OPTION_HPP
#define LIBCLI_OPTION_HPP

#include <cassert>
#include <memory>
#include <string>
#include <variant>

#include "parse.hpp"

namespace libcli {

namespace detail {

struct BoundFlag {
    explicit BoundFlag(bool& result) : result{&result} {}
    void assign(bool flag) const { *result = flag; }

   private:
    bool* result;
};

struct
    BoundValueStorageBase {  // NOLINT(cppcoreguidelines-special-member-functions)
    virtual ~BoundValueStorageBase() = default;
    virtual void assign(std::string_view arg) const = 0;
};

template <typename T>
struct BoundValueStorage : BoundValueStorageBase {
    explicit BoundValueStorage(T& result) : result{&result} {}

    void assign(std::string_view arg) const override
    {
        *result = libcli::parse<T>(arg);
    }

   private:
    T* result;
};

struct BoundValue {
    template <typename T>
    explicit BoundValue(T& result)
        : value_storage{std::make_unique<BoundValueStorage<T>>(result)}
    {
    }

    void assign(std::string_view arg) const { value_storage->assign(arg); }

   private:
    std::unique_ptr<BoundValueStorageBase> value_storage;
};

using BoundVariable = std::variant<BoundFlag, BoundValue>;

template <typename T>
auto make_bound_variable(T& result) -> BoundValue
{
    return BoundValue{result};
}

auto make_bound_variable(bool& result) -> BoundFlag
{
    return BoundFlag{result};
}

}  // namespace detail

struct Option {
    template <typename T>
    Option(std::string_view long_name, std::string_view short_name, T& result)
        : long_name_{long_name.data(), long_name.size()},
          short_name_{short_name.data(), short_name.size()},
          bound_variable{detail::make_bound_variable(result)}
    {
    }

    auto long_name() const -> std::string_view { return long_name_; }
    auto short_name() const -> std::string_view { return short_name_; }

   private:
    friend class Cli;

    std::string long_name_;
    std::string short_name_;
    detail::BoundVariable bound_variable;
};

}  // namespace libcli

#endif  // LIBCLI_OPTION_HPP
