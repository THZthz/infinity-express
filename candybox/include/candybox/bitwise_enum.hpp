#ifndef CANDYBOX_BITWISE_ENUM_HPP__
#define CANDYBOX_BITWISE_ENUM_HPP__

#include <type_traits>

namespace candybox {
namespace bitwise_enum {

template <typename T, typename = void>
struct is_bitwise_enum : std::false_type
{
};

template <typename T>
struct is_bitwise_enum<
    T,
    typename std::enable_if<
        std::is_enum<T>::value &&static_cast<typename std::underlying_type<T>::type>(
            T::none) == 0>::type> : std::true_type
{
};

template <typename T>
constexpr auto
operator|(T lhs, T rhs) noexcept ->
    typename std::enable_if<is_bitwise_enum<T>::value, T>::type
{
	using U = typename std::underlying_type<T>::type;
	return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <typename T>
constexpr auto
operator&(T lhs, T rhs) noexcept ->
    typename std::enable_if<is_bitwise_enum<T>::value, T>::type
{
	using U = typename std::underlying_type<T>::type;
	return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <typename T>
constexpr auto
operator^(T lhs, T rhs) noexcept ->
    typename std::enable_if<is_bitwise_enum<T>::value, T>::type
{
	using U = typename std::underlying_type<T>::type;
	return static_cast<T>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <typename T>
constexpr auto
operator~(T lhs) noexcept -> typename std::enable_if<is_bitwise_enum<T>::value, T>::type
{
	using U = typename std::underlying_type<T>::type;
	return static_cast<T>(~static_cast<U>(lhs));
}

} // namespace bitwise_enum
} // namespace candybox

#endif // CANDYBOX_BITWISE_ENUM_HPP__
