// https://github.com/martinmoene/value-ptr-lite v0.2.1

#ifndef NONSTD_VALUE_PTR_LITE_HPP
#define NONSTD_VALUE_PTR_LITE_HPP

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4345) // initialization behavior changed
#endif

#include <algorithm> // std::swap() until C++11
#include <initializer_list>
#include <type_traits>
#include <cstddef>
#include <cassert>
#include <functional>
#include <memory>
#include <utility>

#include "utils/detail/in_place.hpp"


/*------------------------------------------------------------------------------------------*/
// remove const/volatile/reference
namespace ie {

// type traits C++20:

template <typename T> struct remove_cvref
{
	typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};
} // namespace ie

/*------------------------------------------------------------------------------------------*/

namespace ie {
namespace detail {

using std::default_delete;

template <class T> struct default_clone
{
	default_clone() = default;

	T *operator()(T const &x) const
	{
		static_assert(
		    !std::is_void<T>::value,
		    "default_clone cannot clone incomplete "
		    "type");
		return new T(x);
	}

	T *operator()(T &&x) const { return new T(std::move(x)); }

	template <class... Args>
	T *operator()(in_place_t (&)(ie::detail::in_place_type_tag<T>), Args &&...args) const
	{
		return new T(std::forward<Args>(args)...);
	}

	template <class U, class... Args>
	T *operator()(
	    in_place_t (&)(ie::detail::in_place_type_tag<T>),
	    std::initializer_list<U> il,
	    Args &&...args) const
	{
		return new T(il, std::forward<Args>(args)...);
	}
};

template <class T, class Cloner, class Deleter>
struct __declspec(empty_bases) compressed_ptr : Cloner, Deleter
{
	typedef T element_type;
	typedef T *pointer;

	typedef Cloner cloner_type;
	typedef Deleter deleter_type;

	// Lifetime:

	~compressed_ptr() { deleter_type()(m_ptr); }

	compressed_ptr() noexcept : m_ptr(nullptr) { }

	compressed_ptr(pointer p) noexcept : m_ptr(p) { }

	compressed_ptr(compressed_ptr const &other)
	    : cloner_type(other),
	      deleter_type(other),
	      m_ptr(other.m_ptr ? cloner_type()(*other.m_ptr) : nullptr)
	{
	}

	compressed_ptr(compressed_ptr &&other) noexcept
	    : cloner_type(std::move(other)),
	      deleter_type(std::move(other)),
	      m_ptr(std::move(other.m_ptr))
	{
		other.m_ptr = nullptr;
	}

	explicit compressed_ptr(element_type const &value) : m_ptr(cloner_type()(value)) { }

	explicit compressed_ptr(element_type &&value) noexcept
	    : m_ptr(cloner_type()(std::move(value)))
	{
	}

	template <class... Args>
	explicit compressed_ptr(
	    in_place_t (&)(ie::detail::in_place_type_tag<T>),
	    Args &&...args)
	    : m_ptr(cloner_type()(ie::in_place_type<T>, std::forward<Args>(args)...))
	{
	}

	template <class U, class... Args>
	explicit compressed_ptr(
	    in_place_t (&)(ie::detail::in_place_type_tag<T>),
	    std::initializer_list<U> il,
	    Args &&...args)
	    : m_ptr(cloner_type()(ie::in_place_type<T>, il, std::forward<Args>(args)...))
	{
	}

	compressed_ptr(element_type const &value, cloner_type const &cloner)
	    : cloner_type(cloner), m_ptr(cloner_type()(value))
	{
	}

	compressed_ptr(element_type &&value, cloner_type &&cloner) noexcept
	    : cloner_type(std::move(cloner)), m_ptr(cloner_type()(std::move(value)))
	{
	}

	compressed_ptr(
	    element_type const &value,
	    cloner_type const &cloner,
	    deleter_type const &deleter)
	    : cloner_type(cloner), deleter_type(deleter), m_ptr(cloner_type()(value))
	{
	}

	compressed_ptr(element_type &&value, cloner_type &&cloner, deleter_type &&deleter) noexcept
	    : cloner_type(std::move(cloner)),
	      deleter_type(std::move(deleter)),
	      m_ptr(cloner_type()(std::move(value)))
	{
	}

	explicit compressed_ptr(cloner_type const &cloner)
	    : cloner_type(cloner), m_ptr(nullptr) { }

	explicit compressed_ptr(cloner_type &&cloner) noexcept
	    : cloner_type(std::move(cloner)), m_ptr(nullptr)
	{
	}

	explicit compressed_ptr(deleter_type const &deleter)
	    : deleter_type(deleter), m_ptr(nullptr)
	{
	}

	explicit compressed_ptr(deleter_type &&deleter) noexcept
	    : deleter_type(std::move(deleter)), m_ptr(nullptr)
	{
	}

	compressed_ptr(cloner_type const &cloner, deleter_type const &deleter)
	    : cloner_type(cloner), deleter_type(deleter), m_ptr(nullptr)
	{
	}

	compressed_ptr(cloner_type &&cloner, deleter_type &&deleter) noexcept
	    : cloner_type(std::move(cloner)), deleter_type(std::move(deleter)), m_ptr(nullptr)
	{
	}

	// Observers:

	pointer get() const noexcept { return m_ptr; }

	cloner_type &get_cloner() noexcept { return *this; }

	deleter_type &get_deleter() noexcept { return *this; }

	// Modifiers:

	pointer release() noexcept
	{
		using std::swap;
		pointer result = nullptr;
		swap(result, m_ptr);
		return result;
	}

	void reset(pointer p) noexcept
	{
		get_deleter()(m_ptr);
		m_ptr = p;
	}

	void reset(element_type const &v) { reset(get_cloner()(v)); }

	void reset(element_type &&v) { reset(get_cloner()(std::move(v))); }

	void swap(compressed_ptr &other) noexcept
	{
		using std::swap;
		swap(m_ptr, other.m_ptr);
	}

	pointer m_ptr;
};

} // namespace detail

// class value_ptr:

template <
    class T,
    class Cloner = detail::default_clone<T>,
    class Deleter = detail::default_delete<T> >
class value_ptr
{
public:
	typedef T element_type;
	typedef T *pointer;
	typedef T &reference;
	typedef T const *const_pointer;
	typedef T const &const_reference;

	typedef Cloner cloner_type;
	typedef Deleter deleter_type;

	// Lifetime

	~value_ptr() = default;

	value_ptr() noexcept : m_ptr(cloner_type(), deleter_type()) { }

	explicit value_ptr(std::nullptr_t) noexcept : m_ptr(cloner_type(), deleter_type()) { }

	explicit value_ptr(pointer p) noexcept : m_ptr(p) { }

	value_ptr(value_ptr const &other) : m_ptr(other.m_ptr) { }

	value_ptr(value_ptr &&other) noexcept : m_ptr(std::move(other.m_ptr)) { }

	explicit value_ptr(element_type const &value) : m_ptr(value) { }

	explicit value_ptr(element_type &&value) noexcept : m_ptr(std::move(value)) { }

	template <
	    class... Args,
	    typename std::enable_if<(std::is_constructible<T, Args &&...>::value), int>::type = 0

	    >
	explicit value_ptr(in_place_t (&)(ie::detail::in_place_type_tag<T>), Args &&...args)
	    : m_ptr(ie::in_place_type<T>, std::forward<Args>(args)...)
	{
	}

	template <
	    class U,
	    class... Args,
	    typename std::enable_if<
	        (std::is_constructible<T, std::initializer_list<U> &, Args &&...>::value),
	        int>::type = 0>
	explicit value_ptr(
	    in_place_t (&)(ie::detail::in_place_type_tag<T>),
	    std::initializer_list<U> il,
	    Args &&...args)
	    : m_ptr(ie::in_place_type<T>, il, std::forward<Args>(args)...)
	{
	}

	explicit value_ptr(cloner_type const &cloner) : m_ptr(cloner) { }

	explicit value_ptr(cloner_type &&cloner) noexcept : m_ptr(std::move(cloner)) { }

	explicit value_ptr(deleter_type const &deleter) : m_ptr(deleter) { }

	explicit value_ptr(deleter_type &&deleter) noexcept : m_ptr(std::move(deleter)) { }

	template <
	    class V,
	    class ClonerOrDeleter,
	    typename std::enable_if<
	        (!std::is_same<typename remove_cvref<V>::type,
	                       in_place_t (&)(ie::detail::in_place_type_tag<V>)>::value),
	        int>::type = 0>
	value_ptr(V &&value, ClonerOrDeleter &&cloner_or_deleter)
	    : m_ptr(std::forward<V>(value), std::forward<ClonerOrDeleter>(cloner_or_deleter))
	{
	}

	template <
	    class V,
	    class C,
	    class D,
	    typename std::enable_if<
	        (!std::is_same<typename remove_cvref<V>::type,
	                       in_place_t (&)(ie::detail::in_place_type_tag<V>)>::value),
	        int>::type = 0>
	value_ptr(V &&value, C &&cloner, D &&deleter)
	    : m_ptr(std::forward<V>(value), std::forward<C>(cloner), std::forward<D>(deleter))
	{
	}

	value_ptr &operator=(std::nullptr_t) noexcept
	{
		m_ptr.reset(nullptr);
		return *this;
	}

	value_ptr &operator=(T const &value)
	{
		m_ptr.reset(value);
		return *this;
	}

	template <
	    class U,
	    typename std::enable_if<(std::is_same<typename std::decay<U>::type, T>::value), int>::
	        type = 0>
	value_ptr &operator=(U &&value)
	{
		m_ptr.reset(std::forward<U>(value));
		return *this;
	}

	value_ptr &operator=(value_ptr const &rhs)
	{
		if (this == &rhs) return *this;

		if (rhs) m_ptr.reset(*rhs);
		else m_ptr.reset(pointer(0));
		return *this;
	}

	value_ptr &operator=(value_ptr &&rhs) noexcept
	{
		if (this == &rhs) return *this;

		swap(rhs);

		return *this;
	}

	template <class... Args> void emplace(Args &&...args)
	{
		m_ptr.reset(T(std::forward<Args>(args)...));
	}

	template <class U, class... Args> void emplace(std::initializer_list<U> il, Args &&...args)
	{
		m_ptr.reset(T(il, std::forward<Args>(args)...));
	}

	// Observers:

	pointer get() const noexcept { return m_ptr.get(); }

	cloner_type &get_cloner() noexcept { return m_ptr.get_cloner(); }

	deleter_type &get_deleter() noexcept { return m_ptr.get_deleter(); }

	reference operator*() const
	{
		assert(get() != nullptr);
		return *get();
	}

	pointer operator->() const noexcept
	{
		assert(get() != nullptr);
		return get();
	}

	explicit operator bool() const noexcept { return has_value(); }

	bool has_value() const noexcept { return get(); }

	element_type const &value() const
	{
		assert(has_value());
		return *get();
	}

	element_type &value()
	{
		assert(has_value());
		return *get();
	}

	template <class U> element_type value_or(U &&v) const
	{
		return has_value() ? value() : static_cast<element_type>(std::forward<U>(v));
	}

	// Modifiers:

	pointer release() noexcept { return m_ptr.release(); }

	void reset(pointer p = pointer()) noexcept { m_ptr.reset(p); }

	void swap(value_ptr &other) noexcept { m_ptr.swap(other.m_ptr); }

private:
	detail::compressed_ptr<T, Cloner, Deleter> m_ptr;
};

// Non-member functions:


template <class T>
inline value_ptr<typename std::decay<T>::type>
make_value(T &&v)
{
	return value_ptr<typename std::decay<T>::type>(std::forward<T>(v));
}

template <class T, class... Args>
inline value_ptr<T>
make_value(Args &&...args)
{
	return value_ptr<T>(in_place, std::forward<Args>(args)...);
}

template <class T, class U, class... Args>
inline value_ptr<T>
make_value(std::initializer_list<U> il, Args &&...args)
{
	return value_ptr<T>(in_place, il, std::forward<Args>(args)...);
}
} // namespace ie

/*------------------------------------------------------------------------------------------*/
// Comparison between value_ptr-s:
namespace ie {
// NOLINTBEGIN(readability-simplify-boolean-expr)
// compare content:

template <class T1, class D1, class C1, class T2, class D2, class C2>
inline bool
operator==(value_ptr<T1, D1, C1> const &lhs, value_ptr<T2, D2, C2> const &rhs)
{
	return bool(lhs) != bool(rhs) ? false : bool(lhs) == false ? true : *lhs == *rhs;
}

template <class T1, class D1, class C1, class T2, class D2, class C2>
inline bool
operator!=(value_ptr<T1, D1, C1> const &lhs, value_ptr<T2, D2, C2> const &rhs)
{
	return !(lhs == rhs);
}

template <class T1, class D1, class C1, class T2, class D2, class C2>
inline bool
operator<(value_ptr<T1, D1, C1> const &lhs, value_ptr<T2, D2, C2> const &rhs)
{
	return (!rhs) ? false : (!lhs) ? true : *lhs < *rhs;
}

template <class T1, class D1, class C1, class T2, class D2, class C2>
inline bool
operator<=(value_ptr<T1, D1, C1> const &lhs, value_ptr<T2, D2, C2> const &rhs)
{
	return !(rhs < lhs);
}

template <class T1, class D1, class C1, class T2, class D2, class C2>
inline bool
operator>(value_ptr<T1, D1, C1> const &lhs, value_ptr<T2, D2, C2> const &rhs)
{
	return rhs < lhs;
}

template <class T1, class D1, class C1, class T2, class D2, class C2>
inline bool
operator>=(value_ptr<T1, D1, C1> const &lhs, value_ptr<T2, D2, C2> const &rhs)
{
	return !(lhs < rhs);
}

// compare with value:

template <class T, class C, class D>
bool
operator==(value_ptr<T, C, D> const &vp, T const &value)
{
	return bool(vp) ? *vp == value : false;
}

template <class T, class C, class D>
bool
operator==(T const &value, value_ptr<T, C, D> const &vp)
{
	return bool(vp) ? value == *vp : false;
}

template <class T, class C, class D>
bool
operator!=(value_ptr<T, C, D> const &vp, T const &value)
{
	return bool(vp) ? *vp != value : true;
}

template <class T, class C, class D>
bool
operator!=(T const &value, value_ptr<T, C, D> const &vp)
{
	return bool(vp) ? value != *vp : true;
}

template <class T, class C, class D>
bool
operator<(value_ptr<T, C, D> const &vp, T const &value)
{
	return bool(vp) ? *vp < value : true;
}

template <class T, class C, class D>
bool
operator<(T const &value, value_ptr<T, C, D> const &vp)
{
	return bool(vp) ? value < *vp : false;
}

template <class T, class C, class D>
bool
operator<=(value_ptr<T, C, D> const &vp, T const &value)
{
	return bool(vp) ? *vp <= value : true;
}

template <class T, class C, class D>
bool
operator<=(T const &value, value_ptr<T, C, D> const &vp)
{
	return bool(vp) ? value <= *vp : false;
}

template <class T, class C, class D>
bool
operator>(value_ptr<T, C, D> const &vp, T const &value)
{
	return bool(vp) ? *vp > value : false;
}

template <class T, class C, class D>
bool
operator>(T const &value, value_ptr<T, C, D> const &vp)
{
	return bool(vp) ? value > *vp : true;
}

template <class T, class C, class D>
bool
operator>=(value_ptr<T, C, D> const &vp, T const &value)
{
	return bool(vp) ? *vp >= value : false;
}

template <class T, class C, class D>
bool
operator>=(T const &value, value_ptr<T, C, D> const &vp)
{
	return bool(vp) ? value >= *vp : true;
}

// NOLINTEND(readability-simplify-boolean-expr)
} // namespace ie

/*------------------------------------------------------------------------------------------*/
// swap
namespace ie {

template <class T, class D, class C>
inline void
swap(value_ptr<T, D, C> &lhs, value_ptr<T, D, C> &rhs) noexcept
{
	lhs.swap(rhs);
}

} // namespace ie


// Specialize the std::hash algorithm:

/*------------------------------------------------------------------------------------------*/

namespace std {

template <class T, class D, class C> struct hash<ie::value_ptr<T, D, C> >
{
	typedef ie::value_ptr<T, D, C> argument_type;
	typedef size_t result_type;

	result_type operator()(argument_type const &p) const noexcept
	{
		return hash<typename argument_type::const_pointer>()(p.get());
	}
};

} // namespace std

#ifdef _MSVC_VER
#	pragma warning(pop)
#endif

#endif // NONSTD_VALUE_PTR_LITE_HPP
