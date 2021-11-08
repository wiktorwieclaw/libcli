#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace libcli {

namespace detail {

template <typename T>
inline void parse(std::string_view arg, T& result)
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

    void push_back_parsed(std::string_view args) const
    {
        storage->push_back_parsed(args);
    }

    auto size() const -> std::size_t const { return storage->size(); }

   private:
    struct StorageBase {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~StorageBase() = default;
        virtual void push_back_parsed(std::string_view arg) const = 0;
        virtual auto size() const -> std::size_t = 0;
    };

    template <typename Container>
    struct Storage : StorageBase {
        explicit Storage(Container& var) : var_ptr{&var} {}

        void push_back_parsed(std::string_view arg) const final
        {
            typename Container::value_type x{};
            parse(arg, x);
            var_ptr->push_back(x);
        }

        auto size() const -> std::size_t final { return var_ptr->size(); }

       private:
        Container* var_ptr;
    };

    std::unique_ptr<StorageBase> storage;
};

template <typename T>
inline auto make_bound_variable(T& var) -> BoundValue
{
    return BoundValue{var};
}

inline auto make_bound_variable(bool& var) -> BoundFlag
{
    return BoundFlag{var};
}

template <typename T>
inline auto make_bound_variable(std::vector<T>& var) -> BoundContainer
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

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace detail

struct Cli {
   public:
    template <typename T>
    auto add_option(T& var, std::string long_name, std::string short_name = "")
        -> OptionInfo const&
    {
        options.emplace_back(std::move(long_name), std::move(short_name), var);
        return options.back().info;
    }

    template <typename T>
    void add_argument(T& var)
    {
        positional_args.emplace_back(var);
    }

    void parse(
        int argc,
        char const** argv)  // NOLINT(cppcoreguidelines-avoid-c-arrays)
    {
        [[maybe_unused]] auto const* name = *argv;
        auto unmatched = parse_options({argv + 1, argv + argc});
        parse_positional_arguments(unmatched);
    }

   private:
    auto find_option(std::string_view option) -> detail::Option&
    {
        auto it = std::ranges::find_if(options, [&](auto& o) {
            return o.info.short_name == option || o.info.long_name == option;
        });
        if (it == options.end()) {
            throw std::runtime_error{"find_option"};
        }
        return *it;
    }

    auto parse_options(std::span<char const*> args) -> std::vector<char const**>
    {
        using namespace detail;
        auto unmatched = std::vector<char const**>{};
        for (auto it = args.begin(); it < args.end(); ++it) {
            if (std::string_view{*it}.starts_with('-')) {
                auto& opt = find_option(*it);
                std::visit(
                    Overloaded{
                        [&](BoundFlag& x) { x.set(); },
                        [&](BoundValue& x) {
                            auto value_it = it + 1;
                            if (value_it >= args.end()) {
                                throw std::runtime_error{"parse"};
                            }
                            x.assign_parsed(*value_it);
                            ++it;
                        }},
                    opt.bound_variable);
            }
            else {
                unmatched.push_back(&*it);
            }
        }
        return unmatched;
    }

    // TODO assert only one multiarg \
            error handling
    void parse_positional_arguments(std::span<char const**> args)
    {
        using namespace detail;
        auto arg_it = args.begin();
        for (auto it = positional_args.begin(); it < positional_args.end();
             ++it) {
            std::visit(
                Overloaded{
                    [&](BoundValue& x) {
                        x.assign_parsed(**arg_it);
                        ++arg_it;
                    },
                    [&](BoundContainer& x) {
                        auto num_rhs_positional_args =
                            positional_args.end() - (it + 1);
                        auto limit = args.end() - num_rhs_positional_args;
                        if (arg_it > limit) {
                            throw std::runtime_error{"parse"};
                        }
                        while (arg_it < limit) {
                            x.push_back_parsed(**arg_it);
                            ++arg_it;
                        }
                    }},
                it->bound_variable);
        };
    }

    std::vector<detail::Option> options;
    std::vector<detail::Argument> positional_args;
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP