#ifndef IE_REF_PTR_HPP
#define IE_REF_PTR_HPP

#include <memory>
#include <utility>
#include <cassert>
#include <stdexcept>

#include "utils/detail/aliases.hpp"

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 4100 4456 4189)
#endif // _MSC_VER

namespace ie {

namespace us {
namespace impl {
class NeverNullTagBase
{
public:
	void never_null_tag() const { }
};
} // namespace impl
} // namespace us


namespace us {
namespace impl {
class RefCStrongPointerTagBase
{
};
} // namespace impl
} // namespace us

class refcounting_null_dereference_error : public std::logic_error
{
public:
	using std::logic_error::logic_error;
};

template <typename T> class ref_ptr;
template <typename T> class non_null_ref_ptr;
template <typename T> class fixed_ref_ptr;
template <typename T> class const_ref_ptr;
template <typename T> class const_not_null_ref_ptr;
template <typename T> class const_fixed_ref_ptr;

template <typename T>
non_null_ref_ptr<T> not_null_from_nullable(const ref_ptr<T>& src);
template <typename T>
const_not_null_ref_ptr<T>
not_null_from_nullable(const const_ref_ptr<T>& src);

class RefCounter
{
private:
	int m_counter;

public:
	RefCounter() : m_counter(1) { }
	virtual ~RefCounter() = default;
	void increment() { m_counter++; }
	void decrement()
	{
		assert(0 <= m_counter);
		m_counter--;
	}
	int use_count() const { return m_counter; }
	virtual void* target_obj_address() const = 0;
};

template <class Y> class RefWithTargetObj : public RefCounter
{
public:
	Y m_object;

	template <class... Args>
	RefWithTargetObj(Args&&... args) : m_object(std::forward<Args>(args)...)
	{
	}

	void* target_obj_address() const
	{
		return const_cast<void*>(static_cast<const void*>(std::addressof(m_object)));
	}
};

/* Some code originally came from this stackoverflow post:
	http://stackoverflow.com/questions/6593770/creating-a-non-thread-safe-shared-ptr */

template <class X> class const_ref_ptr;

/* ref_ptr behaves similar to an std::shared_ptr. Some differences being that it foregoes any thread safety
	mechanisms, it does not accept raw pointer assignment or construction (use make_refcounting<>() instead), and it will throw
	an exception on attempted nullptr dereference. And it's faster. */
template <class X> class ref_ptr : public us::impl::RefCStrongPointerTagBase
{
public:
	ref_ptr() : m_ref_with_target_obj_ptr(nullptr) { }
	 ref_ptr(std::nullptr_t) : m_ref_with_target_obj_ptr(nullptr) { }
	~ref_ptr()
	{
		//release();
		/* Doing it this way instead of just calling release() protects against potential reentrant destructor
			calls caused by a misbehaving (user-defined) destructor of the target object. */
		auto_release keep(m_ref_with_target_obj_ptr);
		m_ref_with_target_obj_ptr = nullptr;
	}
	ref_ptr(const ref_ptr& r) { acquire(r.m_ref_with_target_obj_ptr); }
	ref_ptr(ref_ptr&& r)
	{
		m_ref_with_target_obj_ptr = r.m_ref_with_target_obj_ptr;
		r.m_ref_with_target_obj_ptr = nullptr;
	}
	explicit operator bool() const { return nullptr != get(); }
	void clear() { (*this) = ref_ptr<X>(nullptr); }
	ref_ptr& operator=(const ref_ptr& r)
	{
		if (this != &r)
		{
			auto_release keep(m_ref_with_target_obj_ptr);
			acquire(r.m_ref_with_target_obj_ptr);
		}
		return *this;
	}
	bool operator<(const ref_ptr& r) const { return get() < r.get(); }
	bool operator==(const ref_ptr& r) const { return get() == r.get(); }
	bool operator!=(const ref_ptr& r) const { return get() != r.get(); }

	template <class Y> friend class ref_ptr;
	template <class Y, typename = detail::enable_if_t<std::is_base_of<X, Y>::value>>
	ref_ptr(const ref_ptr<Y>& r)
	{
		acquire(r.m_ref_with_target_obj_ptr);
	}
	template <class Y, typename = detail::enable_if_t<std::is_base_of<X, Y>::value>>
	ref_ptr& operator=(const ref_ptr<Y>& r)
	{
		if (this != &r)
		{
			auto_release keep(m_ref_with_target_obj_ptr);
			acquire(r.m_ref_with_target_obj_ptr);
		}
		return *this;
	}
	template <class Y> bool operator<(const ref_ptr<Y>& r) const
	{
		return get() < r.get();
	}
	template <class Y> bool operator==(const ref_ptr<Y>& r) const
	{
		return get() == r.get();
	}
	template <class Y> bool operator!=(const ref_ptr<Y>& r) const
	{
		return get() != r.get();
	}

	X& operator*() const
	{
		if (!m_ref_with_target_obj_ptr)
		{
			throw refcounting_null_dereference_error(
			    "attempt to dereference null pointer "
			    "- ie::ref_ptr");
		}
		X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
		return (*x_ptr);
	}
	X* operator->() const
	{
		if (!m_ref_with_target_obj_ptr)
		{
			throw refcounting_null_dereference_error(
			    "attempt to dereference null pointer "
			    "- ie::ref_ptr");
		}
		X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
		return x_ptr;
	}
	bool unique() const
	{
		return m_ref_with_target_obj_ptr == nullptr ||
		       (m_ref_with_target_obj_ptr->use_count() == 1);
	}

	template <class... Args> static ref_ptr make(Args&&... args)
	{
		auto new_ptr = new RefWithTargetObj<X>(std::forward<Args>(args)...);
		ref_ptr retval(new_ptr);
		return retval;
	}

private:
	explicit ref_ptr(RefWithTargetObj<X>* p /* = nullptr*/)
	{
		m_ref_with_target_obj_ptr = p;
	}

	void acquire(RefCounter* c)
	{
		m_ref_with_target_obj_ptr = c;
		if (c) { c->increment(); }
	}

	void release() { do_release(m_ref_with_target_obj_ptr); }

	struct auto_release
	{
		explicit auto_release(RefCounter* c) : m_ref_with_target_obj_ptr(c) { }
		~auto_release() { do_release(m_ref_with_target_obj_ptr); }
		RefCounter* m_ref_with_target_obj_ptr;
	};

	void static do_release(RefCounter* ref_with_target_obj_ptr)
	{
		// decrement the count, delete if it is nullptr
		if (ref_with_target_obj_ptr)
		{
			if (1 == ref_with_target_obj_ptr->use_count()) { delete ref_with_target_obj_ptr; }
			else { ref_with_target_obj_ptr->decrement(); }
		}
	}

	X* get() const
	{
		if (!m_ref_with_target_obj_ptr) { return nullptr; }
		else
		{
			X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
			return x_ptr;
		}
	}
	X* unchecked_get() const
	{
		X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
		return x_ptr;
	}

	RefCounter* m_ref_with_target_obj_ptr;

	friend class non_null_ref_ptr<X>;
	friend class const_ref_ptr<X>;
};

template <typename T>
class non_null_ref_ptr
    : public ref_ptr<T>,
      public ie::us::impl::NeverNullTagBase
{
public:
	non_null_ref_ptr(const non_null_ref_ptr& src_cref)
	    : ref_ptr<T>(src_cref)
	{
	}
	non_null_ref_ptr(non_null_ref_ptr&& src_ref)
 noexcept 	    : ref_ptr<T>(src_ref)
	{
	}
	 ~non_null_ref_ptr() = default;
	non_null_ref_ptr<T>& operator=(const non_null_ref_ptr<T>& _Right_cref)
	{
		ref_ptr<T>::operator=(_Right_cref);
		return (*this);
	}

	T& operator*() const
	{
		T* x_ptr = (*this).unchecked_get();
		return *x_ptr;
	}
	T* operator->() const
	{
		T* x_ptr = (*this).unchecked_get();
		return x_ptr;
	}

	template <class... Args> static non_null_ref_ptr make(Args&&... args)
	{
		auto new_ptr = new RefWithTargetObj<T>(std::forward<Args>(args)...);
		non_null_ref_ptr retval(new_ptr);
		return retval;
	}

private:
	explicit non_null_ref_ptr(RefWithTargetObj<T>* p /* = nullptr*/)
	    : ref_ptr<T>(p)
	{
	}

	/* If you want to use this constructor, use not_null_from_nullable() instead. */
	explicit non_null_ref_ptr(const ref_ptr<T>& src_cref)
	    : ref_ptr<T>(src_cref)
	{
		*src_cref; // to ensure that src_cref points to a valid target
	}

	friend class fixed_ref_ptr<T>;
	template <typename T2>
	friend non_null_ref_ptr<T2>
	not_null_from_nullable(const ref_ptr<T2>& src);
};

/* fixed_ref_ptr cannot be retargeted or constructed without a target. This pointer is recommended for passing
	parameters by reference. */
template <typename T> class fixed_ref_ptr : public non_null_ref_ptr<T>
{
public:
	fixed_ref_ptr(const fixed_ref_ptr& src_cref)
	    : non_null_ref_ptr<T>(src_cref)
	{
	}
	fixed_ref_ptr(const non_null_ref_ptr<T>& src_cref)
	    : non_null_ref_ptr<T>(src_cref)
	{
	}
	fixed_ref_ptr(fixed_ref_ptr<T>&& src_ref)
	    : non_null_ref_ptr<T>(::std::forward<decltype(src_ref)>(src_ref))
	{
	}
	fixed_ref_ptr(non_null_ref_ptr<T>&& src_ref)
	    : non_null_ref_ptr<T>(::std::forward<decltype(src_ref)>(src_ref))
	{
	}
	 ~fixed_ref_ptr() { }

	template <class... Args> static fixed_ref_ptr make(Args&&... args)
	{
		auto new_ptr = new RefWithTargetObj<T>(std::forward<Args>(args)...);
		fixed_ref_ptr retval(new_ptr);
		return retval;
	}

private:
	explicit fixed_ref_ptr(RefWithTargetObj<T>* p /* = nullptr*/)
	    : non_null_ref_ptr<T>(p)
	{
	}

	/* If you want to use this constructor, use not_null_from_nullable() instead. */
	fixed_ref_ptr(const ref_ptr<T>& src_cref)
	    : non_null_ref_ptr<T>(src_cref)
	{
	}

	fixed_ref_ptr<T>& operator=(const fixed_ref_ptr<T>&) = delete;



	friend class const_ref_ptr<T>;
};

template <class X, class... Args>
non_null_ref_ptr<X>
make_refcounting(Args&&... args)
{
	return non_null_ref_ptr<X>::make(std::forward<Args>(args)...);
}

template <class X, class... Args>
ref_ptr<X>
make_nullable_refcounting(Args&&... args)
{
	return ref_ptr<X>::make(std::forward<Args>(args)...);
}


template <class X>
class const_ref_ptr : public ie::us::impl::RefCStrongPointerTagBase
{
public:
	const_ref_ptr() : m_ref_with_target_obj_ptr(nullptr) { }
	const_ref_ptr(std::nullptr_t) : m_ref_with_target_obj_ptr(nullptr) { }
	~const_ref_ptr()
	{
		//release();
		/* Doing it this way instead of just calling release() protects against potential reentrant destructor
			calls caused by a misbehaving (user-defined) destructor of the target object. */
		auto_release keep(m_ref_with_target_obj_ptr);
		m_ref_with_target_obj_ptr = nullptr;
	}
	const_ref_ptr(const const_ref_ptr& r)
	{
		acquire(r.m_ref_with_target_obj_ptr);
	}
	const_ref_ptr(const ref_ptr<X>& r)
	{
		acquire(r.m_ref_with_target_obj_ptr);
	}
	const_ref_ptr(const_ref_ptr&& r)
	{
		m_ref_with_target_obj_ptr = r.m_ref_with_target_obj_ptr;
		r.m_ref_with_target_obj_ptr = nullptr;
	}
	const_ref_ptr(ref_ptr<X>&& r)
	{
		m_ref_with_target_obj_ptr = r.m_ref_with_target_obj_ptr;
		r.m_ref_with_target_obj_ptr = nullptr;
	}
	explicit operator bool() const { return nullptr != get(); }
	void clear() { (*this) = const_ref_ptr<X>(nullptr); }
	const_ref_ptr& operator=(const const_ref_ptr& r)
	{
		if (this != &r)
		{
			auto_release keep(m_ref_with_target_obj_ptr);
			acquire(r.m_ref_with_target_obj_ptr);
		}
		return *this;
	}
	bool operator<(const const_ref_ptr& r) const { return get() < r.get(); }
	bool operator==(const const_ref_ptr& r) const { return get() == r.get(); }
	bool operator!=(const const_ref_ptr& r) const { return get() != r.get(); }

	template <class Y> friend class const_ref_ptr;
	template <class Y, typename = detail::enable_if_t<std::is_base_of<X, Y>::value>>
	explicit const_ref_ptr(const const_ref_ptr<Y>& r)
	{
		acquire(r.m_ref_with_target_obj_ptr);
	}
	template <class Y, typename = detail::enable_if_t<std::is_base_of<X, Y>::value>>
	const_ref_ptr& operator=(const const_ref_ptr<Y>& r)
	{
		if (this != &r)
		{
			auto_release keep(m_ref_with_target_obj_ptr);
			acquire(r.m_ref_with_target_obj_ptr);
		}
		return *this;
	}
	template <class Y> bool operator<(const const_ref_ptr<Y>& r) const
	{
		return get() < r.get();
	}
	template <class Y> bool operator==(const const_ref_ptr<Y>& r) const
	{
		return get() == r.get();
	}
	template <class Y> bool operator!=(const const_ref_ptr<Y>& r) const
	{
		return get() != r.get();
	}

	const X& operator*() const
	{
		if (!m_ref_with_target_obj_ptr)
		{
			throw refcounting_null_dereference_error(
			    "attempt to dereference null pointer "
			    "- ie::const_ref_ptr");
		}
		X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
		return (*x_ptr);
	}
	const X* operator->() const
	{
		if (!m_ref_with_target_obj_ptr)
		{
			throw refcounting_null_dereference_error(
			    "attempt to dereference null pointer "
			    "- ie::const_ref_ptr");
		}
		X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
		return x_ptr;
	}
	bool unique() const
	{
		return m_ref_with_target_obj_ptr == nullptr ||
		       (m_ref_with_target_obj_ptr->use_count() == 1);
	}

private:
	explicit const_ref_ptr(RefWithTargetObj<X>* p /* = nullptr*/)
	{
		m_ref_with_target_obj_ptr = p;
	}

	void acquire(RefCounter* c)
	{
		m_ref_with_target_obj_ptr = c;
		if (c) { c->increment(); }
	}

	void release() { do_release(m_ref_with_target_obj_ptr); }

	struct auto_release
	{
		explicit auto_release(RefCounter* c) : m_ref_with_target_obj_ptr(c) { }
		~auto_release() { do_release(m_ref_with_target_obj_ptr); }
		RefCounter* m_ref_with_target_obj_ptr;
	};

	void static do_release(RefCounter* ref_with_target_obj_ptr)
	{
		// decrement the count, delete if it is nullptr
		if (ref_with_target_obj_ptr)
		{
			if (1 == ref_with_target_obj_ptr->use_count()) { delete ref_with_target_obj_ptr; }
			else { ref_with_target_obj_ptr->decrement(); }
			ref_with_target_obj_ptr = nullptr;
		}
	}

	const X* get() const
	{
		if (!m_ref_with_target_obj_ptr) { return nullptr; }
		else
		{
			X* x_ptr = static_cast<X*>(m_ref_with_target_obj_ptr->target_obj_address());
			return x_ptr;
		}
	}
	const X* unchecked_get() const
	{
		const X* x_ptr =
		    static_cast<const X*>(m_ref_with_target_obj_ptr->target_obj_address());
		return x_ptr;
	}



	RefCounter* m_ref_with_target_obj_ptr;

	friend class const_not_null_ref_ptr<X>;
};

template <typename T>
class const_not_null_ref_ptr
    : public const_ref_ptr<T>,
      public ie::us::impl::NeverNullTagBase
{
public:
	const_not_null_ref_ptr(const const_not_null_ref_ptr& src_cref)
	    : const_ref_ptr<T>(src_cref)
	{
	}
	const_not_null_ref_ptr(const non_null_ref_ptr<T>& src_cref)
	    : const_ref_ptr<T>(src_cref)
	{
	}
	const_not_null_ref_ptr(const_not_null_ref_ptr&& src_ref)
	    : const_ref_ptr<T>(src_ref)
	{
	}
	const_not_null_ref_ptr(non_null_ref_ptr<T>&& src_ref)
	    : const_ref_ptr<T>(src_ref)
	{
	}
	 ~const_not_null_ref_ptr() { }
	const_not_null_ref_ptr<T>&
	operator=(const const_not_null_ref_ptr<T>& _Right_cref)
	{
		const_ref_ptr<T>::operator=(_Right_cref);
		return (*this);
	}

	const T& operator*() const
	{
		const T* x_ptr = (*this).unchecked_get();
		return *x_ptr;
	}
	const T* operator->() const
	{
		const T* x_ptr = (*this).unchecked_get();
		return x_ptr;
	}

private:
	/* If you want to use this constructor, use not_null_from_nullable() instead. */
	const_not_null_ref_ptr(const const_ref_ptr<T>& src_cref)
	    : const_ref_ptr<T>(src_cref)
	{
		*src_cref; // to ensure that src_cref points to a valid target
	}
	const_not_null_ref_ptr(const ref_ptr<T>& src_cref)
	    : const_ref_ptr<T>(src_cref)
	{
		*src_cref; // to ensure that src_cref points to a valid target
	}



	friend class const_fixed_ref_ptr<T>;
	template <typename _Ty2>
	friend const_not_null_ref_ptr<_Ty2>
	not_null_from_nullable(const const_ref_ptr<_Ty2>& src);
};

/* const_fixed_ref_ptr cannot be retargeted or constructed without a target. This pointer is recommended for passing
	parameters by reference. */
template <typename T>
class const_fixed_ref_ptr : public const_not_null_ref_ptr<T>
{
public:
	const_fixed_ref_ptr(const const_fixed_ref_ptr& src_cref)
	    : const_not_null_ref_ptr<T>(src_cref)
	{
	}
	const_fixed_ref_ptr(const fixed_ref_ptr<T>& src_cref)
	    : const_not_null_ref_ptr<T>(src_cref)
	{
	}
	const_fixed_ref_ptr(const const_not_null_ref_ptr<T>& src_cref)
	    : const_not_null_ref_ptr<T>(src_cref)
	{
	}
	const_fixed_ref_ptr(const non_null_ref_ptr<T>& src_cref)
	    : const_not_null_ref_ptr<T>(src_cref)
	{
	}

	const_fixed_ref_ptr(const_fixed_ref_ptr&& src_ref)
	    : const_not_null_ref_ptr<T>(MSE_FWD(src_ref))
	{
	}
	const_fixed_ref_ptr(fixed_ref_ptr<T>&& src_ref)
	    : const_not_null_ref_ptr<T>(MSE_FWD(src_ref))
	{
	}
	const_fixed_ref_ptr(const_not_null_ref_ptr<T>&& src_ref)
	    : const_not_null_ref_ptr<T>(MSE_FWD(src_ref))
	{
	}
	const_fixed_ref_ptr(non_null_ref_ptr<T>&& src_ref)
	    : const_not_null_ref_ptr<T>(MSE_FWD(src_ref))
	{
	}

	 ~const_fixed_ref_ptr() { }

private:
	/* If you want to use this constructor, use not_null_from_nullable() instead. */
	const_fixed_ref_ptr(const const_ref_ptr<T>& src_cref)
	    : const_not_null_ref_ptr<T>(src_cref)
	{
	}
	const_fixed_ref_ptr(const ref_ptr<T>& src_cref)
	    : const_not_null_ref_ptr<T>(src_cref)
	{
	}

	const_fixed_ref_ptr<T>&
	operator=(const const_fixed_ref_ptr<T>& _Right_cref) = delete;
};
} // namespace ie

namespace std {
template <class T> struct hash<ie::ref_ptr<T>>
{ // hash functor
	typedef ie::ref_ptr<T> argument_type;
	typedef size_t result_type;
	size_t operator()(const ie::ref_ptr<T>& _Keyval) const noexcept
	{
		const T* ptr1 = nullptr;
		if (_Keyval) { ptr1 = std::addressof(*_Keyval); }
		return (hash<const T*>()(ptr1));
	}
};
template <class T> struct hash<ie::non_null_ref_ptr<T>>
{ // hash functor
	typedef ie::non_null_ref_ptr<T> argument_type;
	typedef size_t result_type;
	size_t operator()(const ie::non_null_ref_ptr<T>& _Keyval) const noexcept
	{
		const T* ptr1 = nullptr;
		if (_Keyval) { ptr1 = std::addressof(*_Keyval); }
		return (hash<const T*>()(ptr1));
	}
};
template <class T> struct hash<ie::fixed_ref_ptr<T>>
{ // hash functor
	typedef ie::fixed_ref_ptr<T> argument_type;
	typedef size_t result_type;
	size_t operator()(const ie::fixed_ref_ptr<T>& _Keyval) const noexcept
	{
		const T* ptr1 = nullptr;
		if (_Keyval) { ptr1 = std::addressof(*_Keyval); }
		return (hash<const T*>()(ptr1));
	}
};

template <class T> struct hash<ie::const_ref_ptr<T>>
{ // hash functor
	typedef ie::const_ref_ptr<T> argument_type;
	typedef size_t result_type;
	size_t operator()(const ie::const_ref_ptr<T>& _Keyval) const noexcept
	{
		const T* ptr1 = nullptr;
		if (_Keyval) { ptr1 = std::addressof(*_Keyval); }
		return (hash<const T*>()(ptr1));
	}
};
template <class T> struct hash<ie::const_not_null_ref_ptr<T>>
{ // hash functor
	typedef ie::const_not_null_ref_ptr<T> argument_type;
	typedef size_t result_type;
	size_t operator()(const ie::const_not_null_ref_ptr<T>& _Keyval) const noexcept
	{
		const T* ptr1 = nullptr;
		if (_Keyval) { ptr1 = std::addressof(*_Keyval); }
		return (hash<const T*>()(ptr1));
	}
};
template <class T> struct hash<ie::const_fixed_ref_ptr<T>>
{ // hash functor
	typedef ie::const_fixed_ref_ptr<T> argument_type;
	typedef size_t result_type;
	size_t operator()(const ie::const_fixed_ref_ptr<T>& _Keyval) const noexcept
	{
		const T* ptr1 = nullptr;
		if (_Keyval) { ptr1 = std::addressof(*_Keyval); }
		return (hash<const T*>()(ptr1));
	}
};
} // namespace std

namespace ie {


template <typename T>
non_null_ref_ptr<T>
not_null_from_nullable(const ref_ptr<T>& src)
{
	return src;
}
template <typename T>
const_not_null_ref_ptr<T>
not_null_from_nullable(const const_ref_ptr<T>& src)
{
	return src;
}

} // namespace ie

#ifdef _MSC_VER
#	pragma warning(pop)
#endif /*_MSC_VER*/

#endif // IE_REF_PTR_HPP
