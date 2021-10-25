#ifndef LIBCLI_ALGORITHMS_HPP
#define LIBCLI_ALGORITHMS_HPP

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

#endif  // LIBCLI_ALGORITHMS_HPP
