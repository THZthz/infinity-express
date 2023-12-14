#ifndef IE_IN_PLACE_TAG_HPP
#define IE_IN_PLACE_TAG_HPP

/*------------------------------------------------------------------------------------------*/
// C++17 std::in_place in <utility>
namespace ie {
namespace detail {
#ifndef IE_MONOSTATE_INPLACE_MUTEX
#	define IE_MONOSTATE_INPLACE_MUTEX
/// Used to represent an optional with no data; essentially a bool
class monostate
{
};

#endif // IE_MONOSTATE_INPLACE_MUTEX
template <class T> struct in_place_type_tag
{
};

template <std::size_t /*K*/> struct in_place_index_tag
{
};

} // namespace detail

// A tag type to tell optional to construct its value in-place
struct in_place_t
{
};

template <class T>
inline in_place_t
in_place(detail::in_place_type_tag<T> = detail::in_place_type_tag<T>())
{
	return {};
}

template <std::size_t K>
inline in_place_t
in_place(detail::in_place_index_tag<K> = detail::in_place_index_tag<K>())
{
	return {};
}

template <class T>
inline in_place_t
in_place_type(detail::in_place_type_tag<T> = detail::in_place_type_tag<T>())
{
	return {};
}

template <std::size_t K>
inline in_place_t
in_place_index(detail::in_place_index_tag<K> = detail::in_place_index_tag<K>())
{
	return {};
}\
} // namespace ie


//
// value_ptr:
//

#endif // IE_IN_PLACE_TAG_HPP
