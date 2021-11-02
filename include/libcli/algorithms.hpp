#ifndef LIBCLI_ALGORITHMS_HPP
#define LIBCLI_ALGORITHMS_HPP

#include <ranges>

namespace libcli::detail {

template <typename InputIt, typename OutputIt, typename UnaryPredicate>
void find_all_if(
    InputIt first,
    InputIt limit,
    OutputIt out,
    UnaryPredicate pred)
{
    while (first != limit) {
        if (pred(*first)) {
            *out = first;
            ++out;
        }
        ++first;
    }
}

template <typename InputRange, typename OutputIt, typename UnaryPredicate>
    requires std::ranges::input_range<InputRange>
void find_all_if(InputRange&& r, OutputIt out, UnaryPredicate pred)
{
    find_all_if(r.begin(), r.end(), out, pred);
}

}  // namespace libcli::detail

#endif  // LIBCLI_ALGORITHMS_HPP
