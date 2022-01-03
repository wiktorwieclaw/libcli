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

template <typename InputIt, typename Visitor>
inline void visit_each(InputIt first, InputIt limit, Visitor&& v)
{
    while (first != limit) {
        std::visit(std::forward<Visitor>(v), *first);
        ++first;
    }
};

template <typename T>
inline constexpr auto is_vector = false;

template <typename T>
inline constexpr auto is_vector<std::vector<T>> = true;

template <typename... Ts>
inline auto join(Ts&&... ts) -> std::string
{
    auto ss = std::stringstream{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    (ss << ... << std::forward<Ts>(ts));
    return ss.str();
}

template <typename T>
class arrow_proxy {
    T t;

   public:
    explicit arrow_proxy(T&& t) : t{std::forward<T>(t)} {}

    auto operator->() -> T* { return &t; }
};

}  // namespace detail

struct invalid_cli_definition : public std::invalid_argument {
    using invalid_argument::invalid_argument;
};

struct invalid_input : public std::runtime_error {
    using runtime_error::runtime_error;
};

// clang-format off
template <typename T>
concept from_istream_readable = requires(std::istream& is, T& x) {
    { is >> x } -> std::convertible_to<std::istream&>;
};;
// clang-format on

namespace detail {

template <typename T>
struct is_from_string_view_parsable;

}  // namespace detail

// clang-format off
template <typename T>
concept from_string_view_parsable =
    detail::is_from_string_view_parsable<T>::value;;
// clang-format on

namespace detail {

inline void parse(std::string_view input, std::string& out) { out = input; }

template <from_istream_readable T>
inline void parse(std::string_view input, T& out)
{
    auto ss = std::stringstream{};
    ss << input;
    ss >> out;
}

template <from_string_view_parsable T>
    requires std::default_initializable<T>
inline void parse(std::string_view input, std::optional<T>& out)
{
    out = T{};
    parse(input, *out);
}

// clang-format off
template <typename T>
struct is_from_string_view_parsable : std::bool_constant<
    requires(std::string_view sv, T& x) {
        { parse(sv, x) };
    }
> {};
// clang-format on

struct parse_fn {
    template <from_string_view_parsable T>
    constexpr void operator()(std::string_view input, T& out) const
    {
        return parse(input, out);
    }
};

}  // namespace detail

inline constexpr auto parse = detail::parse_fn{};

namespace detail {

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

template <from_string_view_parsable T>
class bound_value_storage : public bound_value_storage_base {
    T* var_ptr;

   public:
    explicit bound_value_storage(T& var) : var_ptr{&var} {}

    void assign_parsed(std::string_view input) const final
    {
        libcli::parse(input, *var_ptr);
    }
};

class bound_value {
    std::unique_ptr<bound_value_storage_base> storage_ptr;

   public:
    template <from_string_view_parsable T>
    explicit bound_value(T& var)
        : storage_ptr{std::make_unique<bound_value_storage<T>>(var)}
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

template <from_string_view_parsable T>
    requires std::default_initializable<T>
class bound_container_storage : public bound_container_storage_base {
    std::vector<T>* var_ptr;

   public:
    explicit bound_container_storage(std::vector<T>& var) : var_ptr{&var} {}

    void push_back_parsed(std::string_view input) const final
    {
        libcli::parse(input, var_ptr->emplace_back());
    }

    auto size() const -> std::size_t final { return var_ptr->size(); }
};

class bound_container {
    std::unique_ptr<bound_container_storage_base> storage_ptr;

   public:
    template <from_string_view_parsable T>
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

template <from_string_view_parsable T>
inline auto make_bound_variable(T& var) -> bound_value
{
    return bound_value{var};
}

inline auto make_bound_variable(bool& var) -> bound_flag
{
    return bound_flag{var};
}

template <from_string_view_parsable T>
    requires std::default_initializable<T>
inline auto make_bound_variable(std::vector<T>& var) -> bound_container
{
    return bound_container{var};
}

}  // namespace detail

// clang-format off
template <typename T>
concept to_option_argument_bindable =
    std::same_as<T, bool> || from_string_view_parsable<T>;;

template <typename T>
concept to_positional_argument_bindable =
    from_string_view_parsable<T>
    || (detail::is_vector<T>
        && std::default_initializable<std::ranges::range_value_t<T>>
        && from_string_view_parsable<std::ranges::range_value_t<T>>);;

template <typename T>
concept to_program_argument_bindable =
    to_positional_argument_bindable<T> || to_option_argument_bindable<T>;;
// clang-format on

namespace detail {

struct option_description {
    std::string name;
    std::string shorthand;
    bool is_flag;
};

struct option {
    using bound_variable = std::variant<bound_flag, bound_value>;

    bound_variable bound_var;
    option_description desc;

    template <to_option_argument_bindable T>
    option(std::string long_name, std::string short_name, T& var)
        : bound_var{make_bound_variable(var)},
          desc{
              std::move(long_name),
              std::move(short_name),
              std::holds_alternative<bound_flag>(bound_var)}
    {
    }

    void write_parsed(std::string_view str)
    {
        std::get<bound_value>(bound_var).assign_parsed(str);
    }

    void write(bool value) { std::get<bound_flag>(bound_var).assign(value); }
};

struct match_result {
    option_description const* desc;
    std::size_t idx;
};

auto match(std::string_view str, std::vector<option> const& opts)
    -> match_result
{
    auto const it = std::find_if(opts.begin(), opts.end(), [&](auto const& o) {
        return o.desc.shorthand == str || o.desc.name == str;
    });
    if (it == opts.end()) {
        throw invalid_input{join(str, " is not an option")};
    }
    return {&it->desc, static_cast<size_t>(it - opts.begin())};
}

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

    explicit positional_token(std::string_view value) : value{value} {}

    auto operator==(positional_token const& other) const -> bool
    {
        return value == other.value;
    }
};

struct option_token {
    std::string name;
    std::string value;
    std::size_t idx;

    option_token(std::string_view name, std::string_view value, std::size_t idx)
        : name{name}, value{value}, idx{idx}
    {
    }

    auto operator==(option_token const& other) const -> bool
    {
        return name == other.name && value == other.value;
    }
};

struct flag_token {
    std::string name;
    std::size_t idx;

    explicit flag_token(std::string_view name, std::size_t idx)
        : name{name}, idx{idx}
    {
    }

    auto operator==(flag_token const& other) const -> bool
    {
        return name == other.name;
    }
};

using token = std::variant<positional_token, option_token, flag_token>;

template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, std::string_view>
class token_view : std::ranges::view_interface<token_view<R>> {
    R range;
    std::vector<option> const* opts;

    struct sentinel {
    };

    class iterator_impl {
        token_view* parent;
        std::ranges::iterator_t<R> it;
        std::optional<token> tok;
        std::vector<flag_token> flags_buffer;
        bool are_options_terminated = false;

       public:
        iterator_impl(token_view* parent, std::ranges::iterator_t<R> cursor)
            : parent{parent}, it{cursor}
        {
        }

        auto operator==(iterator_impl const& other) const -> bool
        {
            return parent == other.parent && tok == other.tok
                && flags_buffer == other.flags_buffer
                && are_options_terminated == other.are_options_terminated;
        };

        auto operator!=(iterator_impl const& other) const -> bool
        {
            return !(*this == other);
        };

        auto init() -> bool
        {
            if (it == std::ranges::end(parent->range)) { return false; }
            tok = make_token();
            ++it;
            return true;
        }

        auto get() -> token const& { return *tok; }

        auto next() -> bool
        {
            if (!flags_buffer.empty()) {
                tok = flags_buffer.back();
                flags_buffer.pop_back();
                return true;
            }
            if (it == std::ranges::end(parent->range)) { return false; }
            if (*it == "--") {
                are_options_terminated = true;
                if (++it == std::ranges::end(parent->range)) { return false; }
            }
            tok = make_token();
            ++it;
            return true;
        }

       private:
        auto make_token() -> detail::token
        {
            if ((*it)[0] != '-' || are_options_terminated) {
                return positional_token{*it};
            }
            if ((*it)[1] != '-') {
                if (it->length() > 2) {
                    auto const name = it->substr(0, 2);
                    auto const value = it->substr(2);
                    auto const [desc, idx] = match(name, *parent->opts);
                    if (!desc->is_flag) {
                        return option_token{name, value, idx};
                    }
                    std::for_each(value.rbegin(), value.rend(), [&](auto f) {
                        auto name = "- "s;
                        name[1] = f;
                        auto const idx_ = match(name, *parent->opts).idx;
                        flags_buffer.emplace_back(name, idx_);
                    });
                    return flag_token{name, idx};
                }
            }
            else if (auto const pos = it->find('=');
                     pos != std::string_view::npos) {
                auto const name = it->substr(0, pos);
                auto const value = it->substr(pos + 1);
                auto const [desc, idx] = match(name, *parent->opts);
                if (desc->is_flag) {
                    throw invalid_input{join(*it, " is invalid")};
                }
                return option_token{name, value, idx};
            }
            auto const [desc, idx] = match(*it, *parent->opts);
            if (desc->is_flag) { return flag_token{*it, idx}; }
            auto const name = *it;
            if (++it == std::ranges::end(parent->range)) {
                throw invalid_input{join(name, " is missing an argument")};
            }
            return option_token{name, *it, idx};
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

        iterator(token_view* parent, std::ranges::iterator_t<R> cursor)
            : pimpl{std::make_shared<iterator_impl>(parent, cursor)}
        {
            if (!pimpl->init()) { pimpl.reset(); }
        }

        auto operator++() -> iterator&
        {
            copy_on_write();
            if (!pimpl->next()) { pimpl.reset(); }
            return *this;
        }

        auto operator++(int) -> token
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        auto operator*() const -> reference { return pimpl->get(); }

        auto operator->() const -> pointer { return &**this; }

        friend auto operator==(iterator const& lhs, iterator const& rhs) -> bool
        {
            if (lhs.pimpl == nullptr || rhs.pimpl == nullptr) {
                return lhs.pimpl == rhs.pimpl;
            }
            return *lhs.pimpl == *rhs.pimpl;
        }

       private:
        void copy_on_write()
        {
            if (pimpl != nullptr && pimpl.use_count() > 1) {
                pimpl = std::make_shared<iterator_impl>(*pimpl);
            }
        }
    };

   public:
    token_view(R range, std::vector<option> const& opts)
        : range{std::move(range)}, opts{&opts}
    {
    }

    auto begin() { return iterator{this, std::ranges::begin(range)}; }

    auto end() { return iterator{}; }
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
    std::for_each(opts.begin(), opts.end(), [&](auto const& o) {
        if (o.desc.name == name) {
            throw invalid_cli_definition{join(name, " is already defined")};
        }
        if (o.desc.shorthand == shorthand) {
            throw invalid_cli_definition{
                join(shorthand, " is already defined")};
        }
    });
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
    std::string program_name;
    std::vector<detail::option> opts;
    std::vector<detail::argument> args;
    bool has_multi_argument = false;

   public:
    template <to_option_argument_bindable T>
    auto add_option(T& var, std::string name, std::string shorthand = "")
        -> detail::option_description const&
    {
        validate_option_specification(name, shorthand, opts);
        opts.emplace_back(std::move(name), std::move(shorthand), var);
        return opts.back().desc;
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
        if (argc <= 0) { throw std::logic_error{"Input cannot be empty"}; }
        program_name = argv[0];
        using namespace std::ranges;
        auto const args_view =
            subrange{argv + 1, argv + argc}
            | views::transform([](auto x) { return std::string_view{x}; });
        auto const args = std::vector(args_view.begin(), args_view.end());
        auto unmatched = parse_options(detail::token_view{args, opts});
        parse_positional_arguments(unmatched);
    }

    void parse(std::initializer_list<char const*> input)
    {
        parse(static_cast<int>(input.size()), data(input));
    }

   private:
    template <std::ranges::input_range R>
    auto parse_options(R&& range) -> std::vector<detail::positional_token>
    {
        auto unmatched = std::vector<detail::positional_token>{};
        auto token_visitor = detail::overloaded{
            [&](detail::positional_token const& tok) {
                unmatched.push_back(tok);
            },
            [&](detail::flag_token const& tok) { opts[tok.idx].write(true); },
            [&](detail::option_token const& tok) {
                opts[tok.idx].write_parsed(tok.value);
            }};
        visit_each(
            std::ranges::begin(range),
            std::ranges::end(range),
            token_visitor);
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
                    throw invalid_input{"Wrong number of arguments"};
                }
                while (token_it < limit) {
                    container.push_back_parsed(token_it->value);
                    ++token_it;
                }
            }};
        while (arg_it < args.end()) {
            if (token_it == tokens.end()) {
                throw invalid_input("Wrong number of arguments");
            }
            std::visit(bound_variable_visitor, arg_it->bound_var);
            ++arg_it;
        }
    }
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
