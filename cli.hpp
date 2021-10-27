#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <ranges>
#include <span>
#include <vector>

#include "algorithms.hpp"
#include "option.hpp"

auto is_option(std::string_view str) -> bool {
    return str.starts_with('-');
}

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
        char* argv[])  // NOLINT(cppcoreguidelines-avoid-c-arrays)
    {
        auto args = std::vector<std::string_view>(argv + 1, argv + argc);
        auto opt_its = std::vector<decltype(args)::iterator>{};
        find_all_if(args, back_inserter(opt_its), is_option);

        for (auto it = opt_its.rbegin(); it != opt_its.rend(); ++it) {
            auto name_it = *it;
            auto& opt = find_option(*name_it);
            auto args_begin = name_it + 1;
            auto args_end = args_begin + opt.num_args();
            if (args_end > args.end()) {
                throw std::runtime_error{"parse"};
            }
            opt.parse(std::span{args_begin, args_end});
            args.erase(name_it, args_end);
        }
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

#endif  // LIBCLI_CLI_HPP
