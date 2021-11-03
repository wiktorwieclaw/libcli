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
        libcli::parse<T>(arg, *result);
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

struct OptionInfo {
    std::string long_name;
    std::string short_name;
};

namespace detail {

struct Option {
    template <typename T>
    Option(std::string long_name, std::string short_name, T& result)
        : info{std::move(long_name), std::move(short_name)},
          bound_variable{detail::make_bound_variable(result)}
    {
    }

    OptionInfo info;
    detail::BoundVariable bound_variable;
};

}  // namespace detail

}  // namespace libcli

#endif  // LIBCLI_OPTION_HPP
