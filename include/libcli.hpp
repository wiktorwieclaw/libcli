#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <concepts>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace libcli {

template <typename T>
concept streamable = requires(std::istream& is, T& x)
{
    {is >> x};
};
;
namespace detail {

inline void parse(std::string_view arg, streamable auto& result)
{
    auto ss = std::stringstream{};
    ss << arg;
    ss >> result;
}

struct bound_flag {
    explicit bound_flag(bool& var) : var_ptr{&var} {}
    void set() const { *var_ptr = true; }

   private:
    bool* var_ptr;
};

struct bound_value {
    template <streamable T>
    explicit bound_value(T& var)
        : storage_ptr{std::make_unique<storage<T>>(var)}
    {
    }

    void assign_parsed(std::string_view arg) const
    {
        storage_ptr->assign_parsed(arg);
    }

   private:
    struct storage_base {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~storage_base() = default;
        virtual void assign_parsed(std::string_view arg) const = 0;
    };

    template <streamable T>
    struct storage : storage_base {
        explicit storage(T& var) : var_ptr{&var} {}

        void assign_parsed(std::string_view arg) const final
        {
            parse(arg, *var_ptr);
        }

       private:
        T* var_ptr;
    };

    std::unique_ptr<storage_base> storage_ptr;
};

struct bound_container {
    template <streamable T>
        requires std::default_initializable<T>
    explicit bound_container(std::vector<T>& var)
        : storage_ptr{std::make_unique<storage<T>>(var)}
    {
    }

    void push_back_parsed(std::string_view args) const
    {
        storage_ptr->push_back_parsed(args);
    }

    auto size() const -> std::size_t const { return storage_ptr->size(); }

   private:
    struct storage_base {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~storage_base() = default;
        virtual void push_back_parsed(std::string_view arg) const = 0;
        virtual auto size() const -> std::size_t = 0;
    };

    template <streamable T>
        requires std::default_initializable<T>
    struct storage : storage_base {
        explicit storage(std::vector<T>& var) : var_ptr{&var} {}

        void push_back_parsed(std::string_view arg) const final
        {
            parse(arg, var_ptr->emplace_back());
        }

        auto size() const -> std::size_t final { return var_ptr->size(); }

       private:
        std::vector<T>* var_ptr;
    };

    std::unique_ptr<storage_base> storage_ptr;
};

template <streamable T>
inline auto make_bound_variable(T& var) -> bound_value
{
    return bound_value{var};
}

inline auto make_bound_variable(bool& var) -> bound_flag
{
    return bound_flag{var};
}

template <streamable T>
    requires std::default_initializable<T>
inline auto make_bound_variable(std::vector<T>& var) -> bound_container
{
    return bound_container{var};
}

}  // namespace detail

struct option_info {
    std::string long_name;
    std::string short_name;
};

namespace detail {

struct option {
    using bound_variable = std::variant<bound_flag, bound_value>;

    template <typename T>
    option(std::string long_name, std::string short_name, T& var)
        : info{std::move(long_name), std::move(short_name)},
          var{make_bound_variable(var)}
    {
    }

    option_info info;
    bound_variable var;
};

struct argument {
    using bound_variable = std::variant<bound_value, bound_container>;

    template <typename T>
    explicit argument(T& var) : var{make_bound_variable(var)}
    {
    }

    bound_variable var;
};

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace detail

struct cli {
   public:
    template <typename T>
    auto add_option(T& var, std::string long_name, std::string short_name = "")
        -> option_info const&
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
        program_name = *argv;
        auto unmatched = parse_options({argv + 1, argv + argc});
        parse_positional_arguments(unmatched);
    }

   private:
    auto find_option(std::string_view option) -> detail::option&
    {
        auto it = std::ranges::find_if(options, [&](auto& o) {
            return o.info.short_name == option || o.info.long_name == option;
        });
        if (it == options.end()) {
            throw std::runtime_error{"find_option"};
        }
        return *it;
    }

    auto parse_options(std::span<char const*> input)
        -> std::vector<char const**>
    {
        using namespace detail;
        auto unmatched = std::vector<char const**>{};
        for (auto it = input.begin(); it < input.end(); ++it) {
            if (std::string_view{*it}.starts_with('-')) {
                auto& opt = find_option(*it);
                std::visit(
                    overloaded{
                        [&](bound_flag& x) { x.set(); },
                        [&](bound_value& x) {
                            auto value_it = it + 1;
                            if (value_it >= input.end()) {
                                throw std::runtime_error{"parse"};
                            }
                            x.assign_parsed(*value_it);
                            ++it;
                        }},
                    opt.var);
            }
            else {
                unmatched.push_back(&*it);
            }
        }
        return unmatched;
    }

    // TODO assert only one multiarg \
            error handling
    void parse_positional_arguments(std::span<char const**> input)
    {
        using namespace detail;
        auto input_it = input.begin();
        for (auto it = positional_args.begin(); it < positional_args.end();
             ++it) {
            std::visit(
                overloaded{
                    [&](bound_value& x) {
                        x.assign_parsed(**input_it);
                        ++input_it;
                    },
                    [&](bound_container& x) {
                        auto num_rhs_positional_args =
                            positional_args.end() - (it + 1);
                        auto limit = input.end() - num_rhs_positional_args;
                        if (input_it > limit) {
                            throw std::runtime_error{"parse"};
                        }
                        while (input_it < limit) {
                            x.push_back_parsed(**input_it);
                            ++input_it;
                        }
                    }},
                it->var);
        }
    }

    std::string program_name;
    std::vector<detail::option> options;
    std::vector<detail::argument> positional_args;
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
