#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
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

struct invalid_cli_specification : public std::invalid_argument {
    using invalid_argument::invalid_argument;
};

struct invalid_input : public std::runtime_error {
    using runtime_error::runtime_error;
};

template <typename T, typename = void>
struct is_from_istream_readable : std::false_type {
};

template <typename T>
struct is_from_istream_readable<
    T,
    std::void_t<decltype(std::declval<std::istream&>() >> std::declval<T&>())>>
    : std::true_type {
};

template <typename T>
constexpr auto is_from_istream_readable_v = is_from_istream_readable<T>::value;

template <typename T>
inline void parse(std::string_view input, T& result)
{
    static_assert(is_from_istream_readable_v<T>);
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

template <typename T>
inline void parse(std::string_view input, std::optional<T>& o)
{
    static_assert(
        std::is_default_constructible_v<T> && is_from_istream_readable_v<T>);
    o = T{};
    parse(input, *o);
}

class bound_flag {
    bool* var_ptr;

   public:
    explicit bound_flag(bool& var) : var_ptr{&var} {}
    void assign(bool x) { *var_ptr = x; }
};

class bound_value {
    struct storage_base {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~storage_base() = default;
        virtual void assign_parsed(std::string_view input) const = 0;
    };

    template <typename T>
    class storage : public storage_base {
        T* var_ptr;

       public:
        explicit storage(T& var) : var_ptr{&var} {}

        void assign_parsed(std::string_view input) const final
        {
            parse(input, *var_ptr);
        }
    };

    std::unique_ptr<storage_base> storage_ptr;

   public:
    template <typename T>
    explicit bound_value(T& var)
        : storage_ptr{std::make_unique<storage<T>>(var)}
    {
    }

    void assign_parsed(std::string_view input) const
    {
        storage_ptr->assign_parsed(input);
    }
};

class bound_container {
    struct storage_base {  // NOLINT(cppcoreguidelines-special-member-functions)
        virtual ~storage_base() = default;
        virtual void push_back_parsed(std::string_view input) const = 0;
        virtual auto size() const -> std::size_t = 0;
    };

    template <typename T>
    class storage : public storage_base {
        std::vector<T>* var_ptr;

       public:
        explicit storage(std::vector<T>& var) : var_ptr{&var} {}

        void push_back_parsed(std::string_view input) const final
        {
            parse(input, var_ptr->emplace_back());
        }

        auto size() const -> std::size_t final { return var_ptr->size(); }
    };

    std::unique_ptr<storage_base> storage_ptr;

   public:
    template <typename T>
    explicit bound_container(std::vector<T>& var)
        : storage_ptr{std::make_unique<storage<T>>(var)}
    {
    }

    void push_back_parsed(std::string_view input) const
    {
        storage_ptr->push_back_parsed(input);
    }

    auto size() const -> std::size_t { return storage_ptr->size(); }
};

template <typename T>
inline auto make_bound_variable(T& var) -> bound_value
{
    return bound_value{var};
}

inline auto make_bound_variable(bool& var) -> bound_flag
{
    return bound_flag{var};
}

template <typename T>
inline auto make_bound_variable(std::vector<T>& var) -> bound_container
{
    return bound_container{var};
}

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

    option_token(std::string_view name, std::string_view value)
        : name{name}, value{value}
    {
    }

    auto operator==(option_token const& other) const -> bool
    {
        return name == other.name && value == other.value;
    }
};

struct flag_token {
    std::string name;

    explicit flag_token(std::string_view name) : name{name} {}

    auto operator==(flag_token const& other) const -> bool
    {
        return name == other.name;
    }
};

using token = std::variant<positional_token, option_token, flag_token>;

struct option_description {
    std::string long_name;
    std::string short_name;
    bool is_flag;
};

struct option {
    using bound_variable = std::variant<bound_flag, bound_value>;

    bound_variable bound_var;
    option_description description;

    template <typename T>
    option(std::string long_name, std::string short_name, T& var)
        : bound_var{make_bound_variable(var)},
          description{
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

struct argument {
    using bound_variable = std::variant<bound_value, bound_container>;

    bound_variable bound_var;

    template <typename T>
    explicit argument(T& var) : bound_var{make_bound_variable(var)}
    {
    }
};

struct options_t {
    std::vector<option> opts;

    // todo
    struct option_match_result {
        option_description* opt_desc;
        std::size_t idx;
    };

    auto match(std::string_view input) -> option&
    {
        auto it = std::find_if(opts.begin(), opts.end(), [&](auto& o) {
            return o.description.short_name == input ||
                   o.description.long_name == input;
        });
        if (it == opts.end()) {
            throw invalid_input{"Invalid option"};
        }
        return *it;
    }
};

template <typename Converted, typename InputIt>
class conversion_iterator {
    static_assert(
        std::is_assignable_v<
            Converted,
            typename std::iterator_traits<InputIt>::value_type>);

    InputIt it;
    Converted converted;

   public:
    using iterator_type = InputIt;
    using iterator_category = std::input_iterator_tag;
    using difference_type =
        typename std::iterator_traits<InputIt>::difference_type;
    using value_type = Converted;
    using reference = Converted&;
    using pointer = Converted*;

    explicit conversion_iterator(InputIt ptr) : it{ptr} {}

    auto operator==(conversion_iterator const& other) const -> bool
    {
        return it == other.it;
    }

    auto operator!=(conversion_iterator const& other) const -> bool
    {
        return !(*this == other);
    };

    auto operator++() -> conversion_iterator&
    {
        ++it;
        return *this;
    }

    auto operator++(int) -> conversion_iterator
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    auto operator*() -> Converted const&
    {
        converted = *it;
        return converted;
    }

    auto operator->() -> Converted const* { return &**this; }
};

template <typename InputIt>
class token_iterator_impl {
    static_assert(std::is_same_v<
                  typename std::iterator_traits<InputIt>::value_type,
                  std::string_view>);

    InputIt current;
    InputIt end;
    std::optional<token> tok;
    options_t* opts;
    std::vector<std::array<char, 2>> flags_buffer;
    bool are_options_terminated = false;

   public:
    token_iterator_impl(InputIt start, InputIt end, options_t& opts)
        : current{start}, end{end}, opts{&opts}
    {
    }

    auto operator==(token_iterator_impl const& other) const -> bool
    {
        return end == other.end && current == other.current &&
               tok == other.tok && opts == other.opts &&
               flags_buffer == other.flags_buffer &&
               are_options_terminated == other.are_options_terminated;
    };

    auto operator!=(token_iterator_impl const& other) const -> bool
    {
        return !(*this == other);
    };

    auto init() -> bool
    {
        if (current == end) {
            return false;
        }
        tok = make_token();
        return true;
    }

    auto token() -> token const& { return *tok; }

    auto next() -> bool
    {
        if (!flags_buffer.empty()) {
            auto flag = flags_buffer.back();
            flags_buffer.pop_back();
            tok = flag_token{std::string{flag.data(), flag.size()}};
            return true;
        }

        if (current == end) {
            return false;
        }

        tok = make_token();

        if (!tok.has_value()) {
            return false;
        }

        ++current;
        return true;
    }

   private:
    // todo refactor
    auto make_token() -> std::optional<detail::token>
    {
        if (*current == "--") {
            are_options_terminated = true;
            ++current;
            if (current == end) {
                return std::optional<detail::token>{std::nullopt};
            }
        }

        if (are_options_terminated || (*current)[0] != '-') {
            return positional_token{*current};
        }

        if ((*current)[1] != '-' && current->length() > 2) {
            for (auto it = current->rbegin(); it != current->rend() - 2; ++it) {
                flags_buffer.push_back({'-', *it});
            }
            return flag_token{"-"s + (*current)[1]};
        }

        // todo "-o foo" same as "-ofoo"

        if (auto const pos = current->find('=');
            pos != std::string_view::npos) {
            auto name = current->substr(0, pos);
            auto value = current->substr(pos + 1);
            auto const& opt = opts->match(name);
            if (opt.description.is_flag) {
                throw invalid_input{""};
            }
            return option_token{name, value};
        }

        auto const& opt = opts->match(*current);
        if (opt.description.is_flag) {
            return flag_token{*current};
        }

        auto const name = *current;
        ++current;
        if (current == end) {
            throw std::runtime_error{"Not enough args"};  // todo
        }
        return option_token{name, *current++};
    }
};

template <typename InputIt>
class token_iterator {
    using impl = token_iterator_impl<InputIt>;

    std::shared_ptr<impl> pimpl;

   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type =
        typename std::iterator_traits<InputIt>::difference_type;
    using value_type = token;
    using reference = token&;
    using pointer = token*;

    token_iterator() = default;

    token_iterator(InputIt start, InputIt end, options_t& opts)
        : pimpl{std::make_shared<impl>(start, end, opts)}
    {
        if (!pimpl->init()) {
            pimpl.reset();
        }
    }

    auto operator++() -> token_iterator&
    {
        copy_on_write();
        if (!pimpl->next()) {
            pimpl.reset();
        }
        return *this;
    }

    auto operator*() const -> token const& { return pimpl->token(); }

    friend auto operator==(token_iterator const& lhs, token_iterator const& rhs)
        -> bool
    {
        if (lhs.pimpl == nullptr || rhs.pimpl == nullptr) {
            return lhs.pimpl == rhs.pimpl;
        }
        return *lhs.pimpl == *rhs.pimpl;
    }

    friend auto operator!=(token_iterator const& lhs, token_iterator const& rhs)
        -> bool
    {
        return !(lhs == rhs);
    }

   private:
    void copy_on_write()
    {
        if (pimpl != nullptr && pimpl.use_count() > 1) {
            pimpl = std::make_shared<impl>(*pimpl);
        }
    }
};

inline void validate_option_name(std::string_view name)
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

inline void validate_option_shorthand(std::string_view shorthand)
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

inline void validate_option_specification(
    std::string_view name,
    std::string_view shorthand)
{
    validate_option_name(name);
    validate_option_shorthand(shorthand);
}

template <typename InputIt, typename Visitor>
void visit_each(InputIt first, InputIt limit, Visitor&& v)
{
    while (first != limit) {
        std::visit(std::forward<Visitor>(v), *first);
        ++first;
    }
};

class cli {
    std::string program_name;
    options_t opts;
    std::vector<argument> args;
    bool has_multi_argument = false;

   public:
    template <typename T>
    auto add_option(T& var, std::string name, std::string shorthand = "")
        -> option_description const&
    {
        validate_option_specification(name, shorthand);  // TODO move to parsing
        opts.opts.emplace_back(std::move(name), std::move(shorthand), var);
        return opts.opts.back().description;
    }

    template <typename T>
    void add_argument(T& var)
    {
        auto const& ref = args.emplace_back(var);
        if (std::holds_alternative<bound_container>(ref.bound_var)) {
            if (has_multi_argument) {
                throw invalid_cli_specification{
                    "There cannot be more than one multi-argument"};
            }
            has_multi_argument = true;
        }
    }

    void parse(
        int argc,
        char const* const* argv)  // NOLINT(cppcoreguidelines-avoid-c-arrays)
    {
        if (argc <= 0) {
            throw std::logic_error{"Input cannot be empty"};
        }
        program_name = argv[0];
        auto unmatched = parse_options(argv + 1, argv + argc);
        parse_positional_arguments(unmatched);
    }

    void parse(std::initializer_list<char const*> input)
    {
        parse(static_cast<int>(input.size()), data(input));
    }

   private:
    auto parse_options(char const* const* start, char const* const* end)
        -> std::vector<positional_token>
    {
        auto unmatched = std::vector<positional_token>{};
        auto token_visitor = overloaded{
            [&](positional_token const& tok) { unmatched.push_back(tok); },
            [&](flag_token const& tok) { opts.match(tok.name).write(true); },
            [&](option_token const& tok) {
                opts.match(tok.name).write_parsed(tok.value);
            }};
        using iterator = conversion_iterator<std::string_view, decltype(start)>;
        visit_each(
            token_iterator{iterator{start}, iterator{end}, opts},
            token_iterator<iterator>{},
            token_visitor);
        return unmatched;
    }

    void parse_positional_arguments(std::vector<positional_token> const& tokens)
    {
        auto token_it = tokens.begin();
        for (auto arg_it = args.begin(); arg_it < args.end(); ++arg_it) {
            if (token_it == tokens.end()) {
                throw invalid_input("Wrong number of arguments");
            }
            auto bound_variable_visitor = overloaded{
                [&](bound_value& value) {
                    value.assign_parsed(token_it->value);
                    ++token_it;
                },
                [&](bound_container& container) {
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
            std::visit(bound_variable_visitor, arg_it->bound_var);
        }
    }
};
}  // namespace detail

using detail::cli;
using detail::invalid_cli_specification;
using detail::invalid_input;

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
