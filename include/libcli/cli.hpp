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

auto is_option(std::string_view str) -> bool { return str.starts_with('-'); }

}  // namespace detail

struct cli_t {
   public:
    template <typename T>
    auto add_option(T& var, std::string long_name, std::string short_name = "")
    {
        options.emplace_back(long_name, short_name, var);
        return &options.front();
    }

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
                auto first_arg_it = name_it + 1;
                if (first_arg_it + opt.num_args() > args.end()) {
                    throw std::runtime_error{"parse"};
                }
                opt.parse({first_arg_it, opt.num_args()});
                args.erase(name_it, name_it + 1 + opt.num_args());
            });
    }

   private:
    auto find_option(std::string_view option) -> option_t&
    {
        auto it = std::ranges::find_if(options, [=](auto& o) {
            return o.short_name() == option || o.long_name() == option;
        });
        if (it == options.end()) {
            throw std::runtime_error{"find_option"};
        }
        return *it;
    }

    std::vector<option_t> options;
};

}  // namespace libcli

#endif  // LIBCLI_CLI_HPP
