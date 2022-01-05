#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <cctype>
#include <concepts>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace libcli {

namespace detail {

using namespace std::string_literals;

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
using value_t = typename T::value_type;

template <typename T>
inline constexpr auto is_vector = false;

template <typename T>
inline constexpr auto is_vector<std::vector<T>> = true;

template <typename T>
inline constexpr auto is_optional = false;

template <typename T>
inline constexpr auto is_optional<std::optional<T>> = true;

template <typename... Ts>
inline auto join(Ts&&... ts) -> std::string
{
    auto ss = std::stringstream{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    (ss << ... << std::forward<Ts>(ts));
    return ss.str();
}

}  // namespace detail

struct invalid_cli_definition : public std::invalid_argument {
    using invalid_argument::invalid_argument;
};

struct invalid_program_argument : public std::runtime_error {
    using runtime_error::runtime_error;
};

// clang-format off
template <typename T>
concept from_istream_readable = requires(std::istream& is, T& x) {
    { is >> x } -> std::convertible_to<std::istream&>;
};;
// clang-format on

namespace detail {

inline void from_string(std::string_view input, std::string& out)
{
    out = input;
}

template <from_istream_readable T>
inline void from_string(std::string_view input, T& out)
{
    auto ss = std::stringstream{};
    ss << input;
    ss >> out;
    if (ss.fail()) {
        throw invalid_program_argument(join(input, " is not a valid value"));
    }
}

class bound_flag {
    bool* var_ptr;

   public:
    explicit bound_flag(bool& var) : var_ptr{&var} {}
    void assign(bool x) { *var_ptr = x; }
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct bound_value_storage_base {
    virtual ~bound_value_storage_base() = default;
    virtual void assign_parsed(std::string_view input) const = 0;
};

template <from_istream_readable T>
class bound_value_storage : public bound_value_storage_base {
    T* var_ptr;

   public:
    explicit bound_value_storage(T& var) : var_ptr{&var} {}

    void assign_parsed(std::string_view input) const final
    {
        from_string(input, *var_ptr);
    }
};

template <from_istream_readable T>
    requires std::default_initializable<T>
class bound_optional_value_storage : public bound_value_storage_base {
    std::optional<T>* var_ptr;

   public:
    explicit bound_optional_value_storage(std::optional<T>& var) : var_ptr{&var}
    {
    }

    void assign_parsed(std::string_view input) const final
    {
        var_ptr->emplace();
        from_string(input, **var_ptr);
    }
};

class bound_value {
    std::unique_ptr<bound_value_storage_base> storage_ptr;

   public:
    template <from_istream_readable T>
    explicit bound_value(T& var)
        : storage_ptr{std::make_unique<bound_value_storage<T>>(var)}
    {
    }

    template <from_istream_readable T>
        requires std::default_initializable<T>
    explicit bound_value(std::optional<T>& var)
        : storage_ptr{std::make_unique<bound_optional_value_storage<T>>(var)}
    {
    }

    void assign_parsed(std::string_view input) const
    {
        storage_ptr->assign_parsed(input);
    }
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct bound_container_storage_base {
    virtual ~bound_container_storage_base() = default;
    virtual void push_back_parsed(std::string_view input) const = 0;
    virtual auto size() const -> std::size_t = 0;
};

template <from_istream_readable T>
    requires std::default_initializable<T>
class bound_container_storage : public bound_container_storage_base {
    std::vector<T>* var_ptr;

   public:
    explicit bound_container_storage(std::vector<T>& var) : var_ptr{&var} {}

    void push_back_parsed(std::string_view input) const final
    {
        from_string(input, var_ptr->emplace_back());
    }

    auto size() const -> std::size_t final { return var_ptr->size(); }
};

class bound_container {
    std::unique_ptr<bound_container_storage_base> storage_ptr;

   public:
    template <from_istream_readable T>
        requires std::default_initializable<T>
    explicit bound_container(std::vector<T>& var)
        : storage_ptr{std::make_unique<bound_container_storage<T>>(var)}
    {
    }

    void push_back_parsed(std::string_view input) const
    {
        storage_ptr->push_back_parsed(input);
    }

    auto size() const -> std::size_t { return storage_ptr->size(); }
};

template <from_istream_readable T>
inline auto make_bound_variable(T& var) -> bound_value
{
    return bound_value{var};
}

template <from_istream_readable T>
    requires std::default_initializable<T>
inline auto make_bound_variable(std::optional<T>& var) -> bound_value
{
    return bound_value{var};
}

inline auto make_bound_variable(bool& var) -> bound_flag
{
    return bound_flag{var};
}

template <from_istream_readable T>
    requires std::default_initializable<T>
inline auto make_bound_variable(std::vector<T>& var) -> bound_container
{
    return bound_container{var};
}

}  // namespace detail

// clang-format off
template <typename T>
concept to_optional_argument_bindable =
    from_istream_readable<T>
    || (detail::is_optional<T>
        && std::default_initializable<detail::value_t<T>>
        && from_istream_readable<detail::value_t<T>>);;

template <typename T>
concept to_positional_argument_bindable =
    from_istream_readable<T>
    || (detail::is_vector<T>
        && std::default_initializable<std::ranges::range_value_t<T>>
        && from_istream_readable<std::ranges::range_value_t<T>>);;
// clang-format on

namespace detail {

struct option {
    using bound_variable = std::variant<bound_flag, bound_value>;

    bound_variable bound_var;
    std::string name;
    std::string shorthand;

    template <to_optional_argument_bindable T>
    option(std::string long_name, std::string short_name, T& var)
        : bound_var{make_bound_variable(var)},
          name{std::move(long_name)},
          shorthand{std::move(short_name)}
    {
    }

    auto is_flag() const -> bool
    {
        return std::holds_alternative<bound_flag>(bound_var);
    }

    void write_parsed(std::string_view str)
    {
        std::get<bound_value>(bound_var).assign_parsed(str);
    }

    void write(bool value) { std::get<bound_flag>(bound_var).assign(value); }
};

struct argument {
    using bound_variable = std::variant<bound_value, bound_container>;

    bound_variable bound_var;

    template <to_positional_argument_bindable T>
    explicit argument(T& var) : bound_var{make_bound_variable(var)}
    {
    }
};

struct positional_token {
    std::string value;
};

struct option_token {
    std::string name;
    std::string value;
    std::size_t option_idx;
};

struct flag_token {
    std::string name;
    std::size_t flag_idx;
};

using token = std::variant<positional_token, option_token, flag_token>;

class program_argument_token_view
    : std::ranges::view_interface<program_argument_token_view>  //
{
    struct sentinel {
    };

    class iterator_impl {
        program_argument_token_view const* parent;
        std::vector<std::string>::const_iterator current;
        std::optional<token> tok;
        std::vector<flag_token> flags_buffer;
        bool are_options_terminated = false;

       public:
        iterator_impl(
            program_argument_token_view const* parent,
            std::vector<std::string>::const_iterator cursor)
            : parent{parent}, current{cursor}
        {
            next();
        }

        void next()
        {
            if (!flags_buffer.empty()) {
                tok = flags_buffer.back();
                flags_buffer.pop_back();
                return;
            }
            if (current == parent->strs.end()) {
                tok.reset();
                return;
            }
            if (*current == "--") {
                are_options_terminated = true;
                ++current;
                if (current == parent->strs.end()) {
                    tok.reset();
                    return;
                }
            }
            make_next();
            ++current;
        }

        auto has_value() const -> bool { return tok.has_value(); }

        auto value() const -> token const& { return *tok; }

       private:
        struct match_option_result {
            bool is_flag;
            std::size_t idx;
        };

        auto match_option(std::string_view str) -> match_option_result
        {
            auto const it =
                std::ranges::find_if(*parent->opts, [&](auto const& o) {
                    return o.shorthand == str || o.name == str;
                });
            if (it == parent->opts->end()) {
                throw invalid_program_argument{join(str, " is not an option")};
            }
            return {
                it->is_flag(),
                static_cast<std::size_t>(it - parent->opts->begin())};
        }

        void make_next()
        {
            if ((*current)[0] != '-' || are_options_terminated) {
                tok = positional_token{*current};
            }
            else if ((*current)[1] != '-') {
                process_single_dash_string();
            }
            else {
                process_double_dash_string();
            }
        }

        void process_single_dash_string()
        {
            if (current->length() > 2) {
                auto const name = current->substr(0, 2);
                auto const [is_flag, idx] = match_option(name);
                if (is_flag) { process_adjacent_flags(idx); }
                else {
                    auto const value = current->substr(2);
                    tok = option_token{name, value, idx};
                }
            }
            else {
                process_regular_option();
            }
        }

        void process_double_dash_string()
        {
            auto const pos = current->find('=');
            if (pos != std::string_view::npos) {
                process_option_with_equal_sign(pos);
            }
            else {
                process_regular_option();
            }
        }

        void process_option_with_equal_sign(std::size_t pos)
        {
            auto const name = current->substr(0, pos);
            auto const value = current->substr(pos + 1);
            auto const [is_flag, idx] = match_option(name);
            if (is_flag) {
                throw invalid_program_argument{join(*current, " is invalid")};
            }
            tok = option_token{name, value, idx};
        }

        void process_regular_option()
        {
            auto const [is_flag, idx] = match_option(*current);
            if (is_flag) { tok = flag_token{*current, idx}; }
            else {
                auto const name = *current;
                if (++current == parent->strs.end()) {
                    throw invalid_program_argument{
                        join(name, " is missing an argument")};
                }
                tok = option_token{name, *current, idx};
            }
        }

        void process_adjacent_flags(std::size_t idx)
        {
            for (auto const f :
                 *current | std::views::drop(1) | std::views::reverse) {
                auto name = "- "s;
                name[1] = f;
                auto const [is_flag, idx_] = match_option(name);
                if (!is_flag) {
                    throw invalid_program_argument{
                        join(name, " is not a flag")};
                }
                flags_buffer.emplace_back(name, idx_);
            }
            tok = flag_token{current->substr(2), idx};
        }
    };

    class iterator {
        std::shared_ptr<iterator_impl> pimpl;

       public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = token;
        using reference = token const&;
        using pointer = token const*;

        iterator() = default;

        iterator(
            program_argument_token_view const* parent,
            std::vector<std::string>::const_iterator cursor)
            : pimpl{std::make_shared<iterator_impl>(parent, cursor)}
        {
        }

        auto operator++() -> iterator&
        {
            cow();
            pimpl->next();
            return *this;
        }

        auto operator++(int) -> iterator
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        auto operator*() const -> reference { return pimpl->value(); }

        auto operator->() const -> pointer { return &**this; }

        friend auto operator==(iterator const& it, sentinel const&) -> bool
        {
            return !it.pimpl->has_value();
        }

       private:
        void cow()
        {
            if (pimpl.use_count() > 1) {
                pimpl = std::make_shared<iterator_impl>(*pimpl);
            }
        }
    };

    std::vector<std::string> strs;
    std::vector<option> const* opts;

   public:
    program_argument_token_view(
        std::vector<std::string> strs,
        std::vector<option> const& opts)
        : strs{std::move(strs)}, opts{&opts}
    {
    }

    auto begin() const { return iterator{this, strs.begin()}; }

    auto end() const { return sentinel{}; }
};

inline void validate_option_name(std::string_view name)
{
    if (name.size() < 3 || name.substr(0, 2) != "--") {
        throw invalid_cli_definition{
            "Option name has to start with -- and at least one character"};
    }
    for (auto const c : name.substr(2)) {
        if (std::isalnum(c) == 0 && c != '-') {
            throw invalid_cli_definition{
                "Option name has to be composed with alphanumeric "
                "characters or dashes"};
        }
    }
}

inline void validate_option_shorthand(std::string_view shorthand)
{
    if (shorthand.size() != 2 || shorthand[0] != '-') {
        throw invalid_cli_definition{
            "Option shorthand has to start with - and one character"};
    }
    if (std::isalpha(shorthand[1]) == 0) {
        throw invalid_cli_definition{"Option shorthand has to be alphabetic"};
    }
}

inline void validate_uniqueness(
    std::string_view name,
    std::string_view shorthand,
    std::vector<option> const& opts)
{
    for (auto const& o : opts) {
        if (o.name == name) {
            throw invalid_cli_definition{join(name, " is already defined")};
        }
        if (o.shorthand == shorthand) {
            throw invalid_cli_definition{
                join(shorthand, " is already defined")};
        }
    }
}

inline void validate_option_specification(
    std::string_view name,
    std::string_view shorthand,
    std::vector<option> const& opts)
{
    validate_option_name(name);
    validate_option_shorthand(shorthand);
    validate_uniqueness(name, shorthand, opts);
}

}  // namespace detail

class cli {
    std::vector<detail::option> opts;
    std::vector<detail::argument> args;
    bool has_multi_argument = false;

   public:
    template <to_optional_argument_bindable T>
    auto add_option(T& var, std::string name, std::string shorthand = "")
    {
        validate_option_specification(name, shorthand, opts);
        opts.emplace_back(std::move(name), std::move(shorthand), var);
    }

    template <to_positional_argument_bindable T>
    void add_argument(T& var)
    {
        auto const& ref = args.emplace_back(var);
        if (std::holds_alternative<detail::bound_container>(ref.bound_var)) {
            if (has_multi_argument) {
                throw invalid_cli_definition{
                    "There cannot be more than one multi-argument"};
            }
            has_multi_argument = true;
        }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    void parse(int argc, char const* const* argv)
    {
        using namespace std::ranges;
        if (argc <= 0) { throw std::logic_error{"Input cannot be empty"}; }
        auto const args =
            subrange{argv + 1, argv + argc}
            | views::transform([](auto x) { return std::string{x}; });
        auto const tokens = detail::program_argument_token_view{
            std::vector(args.begin(), args.end()),
            opts};
        auto const unmatched = parse_options(tokens);
        parse_positional_arguments(unmatched);
    }

    void parse(std::initializer_list<char const*> input)
    {
        parse(static_cast<int>(input.size()), data(input));
    }

   private:
    auto parse_options(const detail::program_argument_token_view& tokens)
        -> std::vector<detail::positional_token>
    {
        auto unmatched = std::vector<detail::positional_token>{};
        auto token_visitor = detail::overloaded{
            [&](detail::positional_token const& tok) {
                unmatched.push_back(tok);
            },
            [&](detail::flag_token const& tok) {
                opts[tok.flag_idx].write(true);
            },
            [&](detail::option_token const& tok) {
                opts[tok.option_idx].write_parsed(tok.value);
            }};
        for (const auto& tok : tokens) { std::visit(token_visitor, tok); }
        return unmatched;
    }

    void parse_positional_arguments(
        std::vector<detail::positional_token> const& tokens)
    {
        auto token_it = tokens.begin();
        auto arg_it = args.begin();
        auto bound_variable_visitor = detail::overloaded{
            [&](detail::bound_value& value) {
                value.assign_parsed(token_it->value);
                ++token_it;
            },
            [&](detail::bound_container& container) {
                auto const num_args_left = args.end() - (arg_it + 1);
                auto const limit = tokens.end() - num_args_left;
                if (token_it > limit) {
                    throw invalid_program_argument{"Wrong number of arguments"};
                }
                while (token_it < limit) {
                    container.push_back_parsed(token_it->value);
                    ++token_it;
                }
            }};
        while (arg_it < args.end()) {
            if (token_it == tokens.end()) {
                throw invalid_program_argument("Wrong number of arguments");
            }
            std::visit(bound_variable_visitor, arg_it->bound_var);
            ++arg_it;
        }
    }
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
