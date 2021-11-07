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

auto is_option_str(std::string_view str) -> bool
{
    return str.starts_with('-');
}

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
        arguments.emplace_back(var);
    }

    // TODO: fix this monstrosity
    void parse(
        int argc,
        const char* argv[])  // NOLINT(cppcoreguidelines-avoid-c-arrays)
    {
        [[maybe_unused]] auto name = argv[0];
        auto args = std::vector<std::string_view>(argv + 1, argv + argc);
        for (auto it = args.begin(); it < args.end();) {
            if (detail::is_option_str(*it)) {
                auto& opt = find_option(*it);
                std::visit(
                    detail::Overloaded{
                        [&](detail::BoundFlag& x) {
                            x.set();
                            it = args.erase(it);
                        },
                        [&](detail::BoundValue& x) {
                            auto value_it = it + 1;
                            if (value_it >= args.end()) {
                                throw std::runtime_error{"parse"};
                            }
                            x.assign_parsed(*value_it);
                            it = args.erase(it, value_it + 1);
                        }},
                    opt.bound_variable);
            }
            else {
                ++it;
            }
        }

        // TODO assert only one multiarg \
            multiarg needs at least 1 arg \
            error handling
        auto index = 0;
        for (auto it = arguments.begin(); it < arguments.end(); ++it) {
            std::visit(
                detail::Overloaded{
                    [&](detail::BoundContainer& x) {
                        auto num_lhs_args = it - arguments.begin();
                        auto num_rhs_args = arguments.end() - it;
                        auto args_to_take =
                            args.size() - num_lhs_args - num_rhs_args + 1;

                        auto span =
                            std::span{args.begin() + index, args_to_take};
                        x.assign_parsed(span);
                        index += span.size();
                    },
                    [&](detail::BoundValue& x) {
                        x.assign_parsed(args[index]);
                        ++index;
                    }},
                it->bound_variable);
        };
    }

   private:
    auto find_option(std::string_view option) -> detail::Option&
    {
        auto it = std::ranges::find_if(options, [=](auto& o) {
            return o.info.short_name == option || o.info.long_name == option;
        });
        if (it == options.end()) {
            throw std::runtime_error{"find_option"};
        }
        return *it;
    }

    std::vector<detail::Option> options;
    std::vector<detail::Argument> arguments;
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
