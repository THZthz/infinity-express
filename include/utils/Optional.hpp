/// \file libopus_optional.hpp
/// \brief This require type to be default-constructable and copyable.
/// 	Define LIBOPUS_OPTIONAL_NO_EXCEPTIONS to disable exceptions in ie::optional.

#ifndef IE_OPTIONAL_HPP
#define IE_OPTIONAL_HPP

#include <stdexcept>
#include <cassert>

#if __cplusplus < 201703L
// NOLINTBEGIN(readability-simplify-boolean-expr, misc-redundant-expression)
namespace ie {

// type for nullopt
struct nullopt_t
{
	struct init
	{
	};
	explicit nullopt_t(init) { }
};

// extra parenthesis to prevent the most vexing parse:
const nullopt_t nullopt((nullopt_t::init()));

// optional access error.
class bad_optional_access : public std::logic_error
{
public:
	explicit bad_optional_access() : logic_error("bad optional access") { }
};

// Simplistic optional: requires T to be default constructable, copyable.
template <typename T>
class optional
{
private:
	typedef void (optional::*safe_bool)() const;

public:
	typedef T value_type;

	optional() : has_value_(false) { }

	explicit optional(nullopt_t) : has_value_(false) { }

	explicit optional(T const &arg) : has_value_(true), value_(arg) { }

	template <class U>
	explicit optional(optional<U> const &other) : has_value_(other.has_value())
	{
		if (other.has_value()) value_ = other.value();
	}

	optional &operator=(nullopt_t)
	{
		reset();
		return *this;
	}

	template <class U>
	optional &operator=(optional<U> const &other)
	{
		has_value_ = other.has_value();
		if (other.has_value()) value_ = other.value();
		return *this;
	}

	void swap(optional &rhs)
	{
		using std::swap;
		if (has_value() == true && rhs.has_value() == true) { swap(**this, *rhs); }
		else if (has_value() == false && rhs.has_value() == true)
		{
			initialize(*rhs);
			rhs.reset();
		}
		else if (has_value() == true && rhs.has_value() == false)
		{
			rhs.initialize(**this);
			reset();
		}
	}

	// observers

	value_type const *operator->() const { return assert(has_value()), &value_; }

	value_type *operator->() { return assert(has_value()), &value_; }

	value_type const &operator*() const { return assert(has_value()), value_; }

	value_type &operator*() { return assert(has_value()), value_; }


#	if ((defined(_MSVC_LANG) && !defined(__clang__)) &&                                      \
	     (_MSC_VER == 1900 ? 201103L : _MSVC_LANG) >= 201103L) ||                             \
	    __cplusplus >= 201103L
	// c++11 or greater
	explicit operator bool() const { return has_value(); }
#	else
	operator safe_bool() const
	{
		return has_value() ? &optional::this_type_does_not_support_comparisons : 0;
	}
#	endif

	bool has_value() const { return has_value_; }

	value_type const &value() const
	{
#	if LIBOPUS_OPTIONAL_NO_EXCEPTIONS
		assert(has_value());
#	else
		if (!has_value()) throw bad_optional_access();
#	endif
		return value_;
	}

	value_type &value()
	{
#	if LIBOPUS_OPTIONAL_NO_EXCEPTIONS
		assert(has_value());
#	else
		if (!has_value()) throw bad_optional_access();
#	endif
		return value_;
	}

	template <class U>
	value_type value_or(U const &v) const
	{
		return has_value() ? value() : static_cast<value_type>(v);
	}

	// modifiers

	void reset() { has_value_ = false; }

private:
	void this_type_does_not_support_comparisons() const { }

	template <typename V>
	void initialize(V const &value)
	{
		assert(!has_value());
		value_ = value;
		has_value_ = true;
	}

private:
	bool has_value_;
	value_type value_;
};

// Relational operators

template <typename T, typename U>
inline bool
operator==(optional<T> const &x, optional<U> const &y)
{
	return bool(x) != bool(y) ? false : bool(x) == false ? true : *x == *y;
}

template <typename T, typename U>
inline bool
operator!=(optional<T> const &x, optional<U> const &y)
{
	return !(x == y);
}

template <typename T, typename U>
inline bool
operator<(optional<T> const &x, optional<U> const &y)
{
	return (!y) ? false : (!x) ? true : *x < *y;
}

template <typename T, typename U>
inline bool
operator>(optional<T> const &x, optional<U> const &y)
{
	return (y < x);
}

template <typename T, typename U>
inline bool
operator<=(optional<T> const &x, optional<U> const &y)
{
	return !(y < x);
}

template <typename T, typename U>
inline bool
operator>=(optional<T> const &x, optional<U> const &y)
{
	return !(x < y);
}

// Comparison with nullopt

template <typename T>
inline bool
operator==(optional<T> const &x, nullopt_t)
{
	return (!x);
}

template <typename T>
inline bool
operator==(nullopt_t, optional<T> const &x)
{
	return (!x);
}

template <typename T>
inline bool
operator!=(optional<T> const &x, nullopt_t)
{
	return bool(x);
}

template <typename T>
inline bool
operator!=(nullopt_t, optional<T> const &x)
{
	return bool(x);
}

template <typename T>
inline bool
operator<(optional<T> const &, nullopt_t)
{
	return false;
}

template <typename T>
inline bool
operator<(nullopt_t, optional<T> const &x)
{
	return bool(x);
}

template <typename T>
inline bool
operator<=(optional<T> const &x, nullopt_t)
{
	return (!x);
}

template <typename T>
inline bool
operator<=(nullopt_t, optional<T> const &)
{
	return true;
}

template <typename T>
inline bool
operator>(optional<T> const &x, nullopt_t)
{
	return bool(x);
}

template <typename T>
inline bool
operator>(nullopt_t, optional<T> const &)
{
	return false;
}

template <typename T>
inline bool
operator>=(optional<T> const &, nullopt_t)
{
	return true;
}

template <typename T>
inline bool
operator>=(nullopt_t, optional<T> const &x)
{
	return (!x);
}

// Comparison with T

template <typename T, typename U>
inline bool
operator==(optional<T> const &x, U const &v)
{
	return bool(x) ? *x == v : false;
}

template <typename T, typename U>
inline bool
operator==(U const &v, optional<T> const &x)
{
	return bool(x) ? v == *x : false;
}

template <typename T, typename U>
inline bool
operator!=(optional<T> const &x, U const &v)
{
	return bool(x) ? *x != v : true;
}

template <typename T, typename U>
inline bool
operator!=(U const &v, optional<T> const &x)
{
	return bool(x) ? v != *x : true;
}

template <typename T, typename U>
inline bool
operator<(optional<T> const &x, U const &v)
{
	return bool(x) ? *x < v : true;
}

template <typename T, typename U>
inline bool
operator<(U const &v, optional<T> const &x)
{
	return bool(x) ? v < *x : false;
}

template <typename T, typename U>
inline bool
operator<=(optional<T> const &x, U const &v)
{
	return bool(x) ? *x <= v : true;
}

template <typename T, typename U>
inline bool
operator<=(U const &v, optional<T> const &x)
{
	return bool(x) ? v <= *x : false;
}

template <typename T, typename U>
inline bool
operator>(optional<T> const &x, U const &v)
{
	return bool(x) ? *x > v : false;
}

template <typename T, typename U>
inline bool
operator>(U const &v, optional<T> const &x)
{
	return bool(x) ? v > *x : true;
}

template <typename T, typename U>
inline bool
operator>=(optional<T> const &x, U const &v)
{
	return bool(x) ? *x >= v : false;
}

template <typename T, typename U>
inline bool
operator>=(U const &v, optional<T> const &x)
{
	return bool(x) ? v >= *x : true;
}

// Convenience function to create an optional.

template <typename T>
inline optional<T>
make_optional(T const &v)
{
	return optional<T>(v);
}

} // namespace ie
// NOLINTEND(readability-simplify-boolean-expr, misc-redundant-expression)
#else
#	include <optional>
namespace ie {
using nullopt = std::nullopt;
using optional = std::optional;
using make_optional = std::make_optional;
} // namespace ie
#endif

#endif // IE_OPTIONAL_HPP
