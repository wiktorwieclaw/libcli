#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <ranges>
#include <span>
#include <vector>

#include "option.hpp"

namespace libcli {

namespace detail {

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
