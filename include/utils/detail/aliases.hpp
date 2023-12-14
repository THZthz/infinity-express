#ifndef IE_ALIASES_HPP
#define IE_ALIASES_HPP

#include <utility>

namespace ie {

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

} // namespace ie

#endif // IE_ALIASES_HPP
