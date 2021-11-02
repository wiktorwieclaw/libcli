#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <ranges>
#include <span>
#include <vector>

#include "algorithms.hpp"
#include "option.hpp"

namespace libcli {

namespace detail {

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

auto is_option(std::string_view str) -> bool { return str.starts_with('-'); }

}  // namespace detail

struct Cli {
   public:
    template <typename T>
    auto add_option(T& var, std::string long_name, std::string short_name = "")
    {
        options.emplace_back(long_name, short_name, var);
        return &options.front();
    }

    // TODO: fix this monstrosity
    void parse(
        int argc,
        const char* argv[])  // NOLINT(cppcoreguidelines-avoid-c-arrays)
    {
        [[maybe_unused]] auto name = argv[0];
        auto args = std::vector<std::string_view>(argv + 1, argv + argc);
        auto option_its = std::vector<decltype(args)::iterator>{};
        detail::find_all_if(args, back_inserter(option_its), detail::is_option);

        std::for_each(
            option_its.rbegin(),
            option_its.rend(),
            [this, &args](auto name_it) {
                auto& opt = find_option(*name_it);
                std::visit(
                    detail::Overloaded{
                        [&](detail::BoundFlag& x) {
                            x.assign(true);
                            args.erase(name_it, name_it + 1);
                        },
                        [&](std::unique_ptr<detail::BoundValueBase>& x) {
                            auto first_arg_it = name_it + 1;
                            if (first_arg_it > args.end()) {
                                throw std::runtime_error{"parse"};
                            }
                            x->assign(*first_arg_it);
                            args.erase(name_it, first_arg_it + 1);
                        }},
                    opt.bound_variable);
            });
    }

   private:
    auto find_option(std::string_view option) -> Option&
    {
        auto it = std::ranges::find_if(options, [=](auto& o) {
            return o.short_name() == option || o.long_name() == option;
        });
        if (it == options.end()) {
            throw std::runtime_error{"find_option"};
        }
        return *it;
    }

    std::vector<Option> options;
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
