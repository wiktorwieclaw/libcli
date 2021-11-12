#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <cctype>
#include <concepts>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace libcli {
namespace detail {

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct invalid_cli_specification : public std::invalid_argument {
    using invalid_argument::invalid_argument;
};

struct invalid_input : public std::runtime_error {
    using runtime_error::runtime_error;
};

// clang-format off
template <typename T>
concept input_streamable = requires(std::istream& is, T& x) {
    { is >> x } -> std::convertible_to<std::istream&>;
};
// clang-format on

inline void parse(std::string_view input, input_streamable auto& result)
{
    auto ss = std::stringstream{};
    ss << input;
    ss >> result;
}

inline void parse(std::string_view input, std::string& result)
{
    auto ss = std::stringstream{};
    ss << input;
    result = ss.str();
}

inline void parse(std::string_view input, bool& result)
{
    if (input == "1" || input == "true") {
        result = true;
    }
    else if (input == "0" || input == "false") {
        result = false;
    }
    else {
        throw invalid_input{"Invalid flag value"};
    }
}

template <std::default_initializable T>
inline void parse(std::string_view input, std::optional<T>& o)
{
    o = T{};
    parse(input, *o);
}

// clang-format off
template <typename T>
concept parsable = requires(std::string_view str, T& out) {
    { parse(str, out) } -> std::same_as<void>;
};
// clang-format on

struct bound_flag {
    explicit bound_flag(bool& var) : var_ptr{&var} {}
    void assign_parsed(std::string_view input) const { parse(input, *var_ptr); }
    void assign(bool x) { *var_ptr = x; }

   private:
    bool* var_ptr;
};

struct bound_value {
    template <typename T>
    explicit bound_value(T& var)
        : storage_ptr{std::make_unique<storage<T>>(var)}
    {
    }

    void assign_parsed(std::string_view input) const
    {
        storage_ptr->assign_parsed(input);
    }

   private:
    struct storage_base {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~storage_base() = default;
        virtual void assign_parsed(std::string_view input) const = 0;
    };

    template <typename T>
    struct storage : storage_base {
        explicit storage(T& var) : var_ptr{&var} {}

        void assign_parsed(std::string_view input) const final
        {
            parse(input, *var_ptr);
        }

       private:
        T* var_ptr;
    };

    std::unique_ptr<storage_base> storage_ptr;
};

struct bound_container {
    template <parsable T>
    explicit bound_container(std::vector<T>& var)
        : storage_ptr{std::make_unique<storage<T>>(var)}
    {
    }

    void push_back_parsed(std::string_view input) const
    {
        storage_ptr->push_back_parsed(input);
    }

    auto size() const -> std::size_t const { return storage_ptr->size(); }

   private:
    struct storage_base {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~storage_base() = default;
        virtual void push_back_parsed(std::string_view input) const = 0;
        virtual auto size() const -> std::size_t = 0;
    };

    template <parsable T>
    struct storage : storage_base {
        explicit storage(std::vector<T>& var) : var_ptr{&var} {}

        void push_back_parsed(std::string_view input) const final
        {
            parse(input, var_ptr->emplace_back());
        }

        auto size() const -> std::size_t final { return var_ptr->size(); }

       private:
        std::vector<T>* var_ptr;
    };

    std::unique_ptr<storage_base> storage_ptr;
};

inline auto make_bound_variable(auto& var) -> bound_value
{
    return bound_value{var};
}

inline auto make_bound_variable(bool& var) -> bound_flag
{
    return bound_flag{var};
}

template <parsable T>
inline auto make_bound_variable(std::vector<T>& var) -> bound_container
{
    return bound_container{var};
}

struct option_info {
    std::string long_name;
    std::string short_name;
};

struct option {
    using bound_variable = std::variant<bound_flag, bound_value>;

    option(std::string long_name, std::string short_name, auto& var)
        : info{std::move(long_name), std::move(short_name)},
          bound_var{make_bound_variable(var)}
    {
    }

    option_info info;
    bound_variable bound_var;
};

struct argument {
    using bound_variable = std::variant<bound_value, bound_container>;

    explicit argument(auto& var) : bound_var{make_bound_variable(var)} {}

    bound_variable bound_var;
};

inline void verify_option_name(std::string_view name)
{
    if (name.size() < 3 || name.substr(0, 2) != "--") {
        throw invalid_cli_specification{
            "Option name has to start with -- and at least one character"};
    }
    for (auto const c : name.substr(2)) {
        if (std::isalnum(c) == 0 && c != '-') {
            throw invalid_cli_specification{
                "Option name has to be composed with alphanumeric "
                "characters or dashes"};
        }
    }
}

inline void verify_option_shorthand(std::string_view shorthand)
{
    if (shorthand.size() != 2 || shorthand[0] != '-') {
        throw invalid_cli_specification{
            "Option shorthand has to start with - and one character"};
    }
    if (std::isalpha(shorthand[1]) == 0) {
        throw invalid_cli_specification{
            "Option shorthand has to be alphabetic"};
    }
}

inline void verify_option_specification(
    std::string_view name,
    std::string_view shorthand)
{
    verify_option_name(name);
    verify_option_shorthand(shorthand);
}

struct cli {
   public:
    auto add_option(auto& var, std::string name, std::string shorthand = "")
        -> option_info const&
    {
        verify_option_specification(name, shorthand);
        options.emplace_back(std::move(name), std::move(shorthand), var);
        return options.back().info;
    }

    void add_argument(auto& var) { positional_args.emplace_back(var); }

    void parse(
        int argc,
        char const* const* argv)  // NOLINT(cppcoreguidelines-avoid-c-arrays)
    {
        if (argc <= 0) {
            throw std::logic_error{"Input cannot be empty"};
        }
        program_name = *argv;
        auto unmatched = parse_options({argv + 1, argv + argc});
        parse_positional_arguments(unmatched);
    }

    void parse(std::initializer_list<char const*> input)
    {
        parse(static_cast<int>(input.size()), data(input));
    }

   private:
    auto match_option(std::string_view input) -> option&
    {
        auto it = std::ranges::find_if(options, [&](auto& o) {
            return o.info.short_name == input || o.info.long_name == input;
        });
        if (it == options.end()) {
            throw invalid_input{"Invalid option"};
        }
        return *it;
    }

    auto parse_options(std::span<char const* const> input)
        -> std::vector<char const* const*>
    {
        auto unmatched = std::vector<char const* const*>{};
        for (auto it = input.begin(); it < input.end(); ++it) {
            auto const str = std::string_view{*it};
            if (str.starts_with('-')) {
                if (auto const pos = str.find('=');
                    pos != std::string_view::npos) {
                    auto const lhs = str.substr(0, pos);
                    auto const rhs = str.substr(pos + 1);
                    auto& opt = match_option(lhs);
                    std::visit(
                        [=](auto& x) { x.assign_parsed(rhs); },
                        opt.bound_var);
                }
                else {
                    auto& opt = match_option(str);
                    std::visit(
                        overloaded{
                            [&](bound_flag& x) { x.assign(true); },
                            [&](bound_value& x) {
                                auto value_it = it + 1;
                                if (value_it >= input.end()) {
                                    throw invalid_input{"Missing option value"};
                                }
                                x.assign_parsed(*value_it);
                                ++it;
                            }},
                        opt.bound_var);
                }
            }
            else {
                unmatched.push_back(&*it);
            }
        }
        return unmatched;
    }

    // TODO assert only one multiarg
    void parse_positional_arguments(std::span<char const* const*> input)
    {
        auto input_it = input.begin();
        for (auto it = positional_args.begin(); it < positional_args.end();
             ++it) {
            if (input_it == input.end()) {
                throw invalid_input("Wrong number of arguments");
            }
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
                            throw invalid_input{"Wrong number of arguments"};
                        }
                        while (input_it < limit) {
                            x.push_back_parsed(**input_it);
                            ++input_it;
                        }
                    }},
                it->bound_var);
        }
    }

    std::string program_name;
    std::vector<option> options;
    std::vector<argument> positional_args;
};
}  // namespace detail

using detail::cli;
using detail::invalid_cli_specification;
using detail::invalid_input;

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
