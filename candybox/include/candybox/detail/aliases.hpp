#ifndef CANDYBOX_ALIASES_HPP__
#define CANDYBOX_ALIASES_HPP__

#include <utility>

namespace candybox {

/// Used to represent an optional with no data; essentially a bool
class monostate
{
};

namespace detail {

// C++14-style aliases for brevity
template <class T> using remove_const_t = typename std::remove_const<T>::type;
template <class T> using remove_reference_t = typename std::remove_reference<T>::type;
template <class T> using decay_t = typename std::decay<T>::type;
template <bool E, class T = void> using enable_if_t = typename std::enable_if<E, T>::type;
template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

} // namespace detail

} // namespace candybox

#endif // CANDYBOX_ALIASES_HPP__
