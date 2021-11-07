#ifndef LIBCLI_OPTION_HPP
#define LIBCLI_OPTION_HPP

#include <cassert>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace libcli {

namespace detail {

template <typename T>
void parse(std::string_view arg, T& result)
{
    auto ss = std::stringstream{};
    ss << arg;
    ss >> result;
}

struct BoundFlag {
    explicit BoundFlag(bool& var) : var_ptr{&var} {}
    void set() const { *var_ptr = true; }

   private:
    bool* var_ptr;
};

struct BoundValue {
    template <typename T>
    explicit BoundValue(T& var) : storage{std::make_unique<Storage<T>>(var)}
    {
    }

    void assign_parsed(std::string_view arg) const
    {
        storage->assign_parsed(arg);
    }

   private:
    struct StorageBase {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~StorageBase() = default;
        virtual void assign_parsed(std::string_view arg) const = 0;
    };

    template <typename T>
    struct Storage : StorageBase {
        explicit Storage(T& var) : var_ptr{&var} {}

        void assign_parsed(std::string_view arg) const final
        {
            parse(arg, *var_ptr);
        }

       private:
        T* var_ptr;
    };

    std::unique_ptr<StorageBase> storage;
};

struct BoundContainer {
    template <typename Container>
    explicit BoundContainer(Container& var)
        : storage{std::make_unique<Storage<Container>>(var)}
    {
    }

    void assign_parsed(std::span<std::string_view> args) const
    {
        storage->assign_parsed(args);
    }

    auto size() const -> std::size_t const { return storage->size(); }

   private:
    struct StorageBase {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~StorageBase() = default;
        virtual void assign_parsed(std::span<std::string_view> args) const = 0;
        virtual auto size() const -> std::size_t = 0;
    };

    template <typename Container>
    struct Storage : StorageBase {
        explicit Storage(Container& var) : var_ptr{&var} {}

        void assign_parsed(std::span<std::string_view> args) const final
        {
            for (auto arg : args) {
                typename Container::value_type x{};
                parse(arg, x);
                var_ptr->push_back(x);
            }
        }

        auto size() const -> std::size_t final { return var_ptr->size(); }

       private:
        Container* var_ptr;
    };

    std::unique_ptr<StorageBase> storage;
};

template <typename T>
auto make_bound_variable(T& var) -> BoundValue
{
    return BoundValue{var};
}

auto make_bound_variable(bool& var) -> BoundFlag { return BoundFlag{var}; }

template <typename T>
auto make_bound_variable(std::vector<T>& var) -> BoundContainer
{
    return BoundContainer{var};
}

}  // namespace detail

struct OptionInfo {
    std::string long_name;
    std::string short_name;
};

namespace detail {

struct Option {
    using BoundVariable = std::variant<BoundFlag, BoundValue>;

    template <typename T>
    Option(std::string long_name, std::string short_name, T& var)
        : info{std::move(long_name), std::move(short_name)},
          bound_variable{make_bound_variable(var)}
    {
    }

    OptionInfo info;
    BoundVariable bound_variable;
};

struct Argument {
    using BoundVariable = std::variant<BoundValue, BoundContainer>;

    template <typename T>
    explicit Argument(T& var) : bound_variable{make_bound_variable(var)}
    {
    }

    BoundVariable bound_variable;
};

}  // namespace detail

}  // namespace libcli

#endif  // LIBCLI_OPTION_HPP
