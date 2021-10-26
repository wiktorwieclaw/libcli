#ifndef LIBCLI_CLI_HPP
#define LIBCLI_CLI_HPP

#include <algorithm>
#include <ranges>
#include <span>
#include <vector>

#include "algorithms.hpp"
#include "option.hpp"

bool is_option(std::string_view str) {
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
            auto& opt = find_option(**it);
            if (*it + 1 + opt.var->num() > args.end()) {
                throw std::runtime_error{"parse"};
            }
            opt.var->parse(std::span{*it + 1, opt.var->num()});
            args.erase(*it, *it + 1 + opt.var->num());
        }
    }

   private:
    auto find_option(std::string_view option) -> option_t&
    {
        auto it = std::ranges::find_if(options, [=](auto& o) {
            return o.short_name == option || o.long_name == option;
        });
        if (it == options.end()) {
            throw std::runtime_error{"find_option"};
        }
        return *it;
    }

    std::vector<option_t> options;
};

#endif  // LIBCLI_CLI_HPP
