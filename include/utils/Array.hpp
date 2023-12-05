/// \file Array.hpp
/// \brief A stl-like vector implementation but with more c-like memory control.
/// 	This array allows you to manipulate its data freely(for some dirty and ugly hack).
///		But be aware that this implementation use memmove, which knows none about c++ copy
///		behavior of a class(especially virtual class, in which case virtual table may be
/// 	overwritten).
///
/// 	Compliant compilers(tested currently):
/// 		- GCC 4.8.x and 4.9.x (in C++11 mode)
/// 		- Clang 3.4, 3.5, 3.6 (in C++11 mode)
/// 		- Clang under OSX (in C++11 mode)
/// 		- Microsoft Visual Studio 2015
///
/// 	Why choose another vector implementation instead of std::vector ?
/// 		std::vector does a great job for most use cases, but there are some limitations,
/// 		due to implementation choices or what the standard actually allows.
/// 		- Growing strategy
/// 			The first limitation is the growth strategy chosen by the implementation.
/// 			Many of them made one choice that can't be changed by the user. For instance,
/// 			LLVM's std::vector implementation (as of January 2015) will multiply by 2 the
/// 			vector capacity if room is needed. You might want to choose a smaller factor,
/// 			or simply not to do this if for instance your vector already takes 2GB of
/// 			memory.
/// 		- sizeof(std::vector)
/// 			The second one is the size of an std::vector object. Most implementations uses
/// 			three pointers to store the beginning, the end (of objects) and the end of
/// 			storage of the container. This leads to a 24 bytes object on 64-bit systems.
/// 			If your container as less than ~2**32 objects (which might be often the case),
/// 			it can be interesting to use two 32-bits unsigned integers to store the number
/// 			of objects and the capacity of the container.
/// 		- realloc support
/// 			The third one is the lack of support for realloc. The realloc might allow
/// 			"in-place" reallocation as there are already room available at the end of the
/// 			actual buffer, thus removing the need for a copy of the previous buffer into
/// 			the new allocated one. Note that this can only be used for POD objects as this
/// 			copy is implicitly done by realloc if needed. POD type would need a kind of
/// 			realloc_no_copy interface to be efficient (a proposal was done for this
/// 			(http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3495.txt), but never
/// 			accepted :/)
/// 			We might consider using it for non-POD types, but benchmarking must be done to
/// 			see if this useless copy is generally negligible versus the potential "in-place
/// 			reallocation" gain.
/// 		- "resize but do not construct"
/// 			This API can be dangerous with non-POD types.
/// 		- Size-aware allocator
/// 			Last but not least, std::vector does not leverage the possibility that an
/// 			allocator might be able to know the amount of allocated memory of a given
/// 			pointer. This allows two optimisations: being able not to store the capacity
/// 			of the vector (thus gaining memory) and a better memory usage.
///
/// 		Feature
/// 			- stores a pointer and two unsigned integers (for the size and the capacity),
/// 		 	  instead of three pointers as commonly done.
/// 			- POD-types optimisation: uses memcpy, memmove, memcmp and alike functions
/// 			  when possible with POD types.
/// 			- realloc-aware allocator: for instance, realloc can be used for POD types.
/// 			- size-aware allocator: do not store the capacity of the container if the
/// 			  allocator is capable of giving the allocated size associated with a pointer.
/// 			  For instance, malloc_usable_size can be used on GNU systems.
/// 			- configurable growing strategy: The growing strategy is used when the vector
/// 			  needs to grow (when using emplace, emplace_back, push_back or insert). Most
/// 			  vector implementations do not allow the user to choose how to grow the vector
/// 			  capacity (linearly, exponentially, etc...). By default, ie::vector multiply
/// 			  the capacity by 3/2, but you can implement you own strategy.
/// 			- resize_no_construct API: this gives the ability to resize a container without
/// 			  calling the default constructor of the underlying objects. For instance, for
/// 			  a vector of integers, this remove the first initialisation at zero, which can
/// 			  be costly in some situations.
/// 			- if you know what you are doing, integer overflow checks can be disabled for
/// 			  performance reasons.

#ifndef IE_ARRAY_HPP
#define IE_ARRAY_HPP

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <memory>

#include "Memory.hpp"

/*----------------------------------------------------------------------------*/

namespace ie {

struct size_aware_allocator
{
};
struct reallocable_allocator
{
};

template <class Alloc>
struct is_size_aware_allocator : public std::is_base_of<size_aware_allocator, Alloc>
{
};

template <class Alloc>
struct is_reallocable_allocator : public std::is_base_of<reallocable_allocator, Alloc>
{
};

} // namespace ie


/*----------------------------------------------------------------------------*/
// for size_aware_allocator,
// usable_memory_size can tell the reserved size given by the pointer allocated

#ifdef __APPLE__
#	include <malloc/malloc.h>
#else
#	include <malloc.h>
#endif

#if defined __linux__ || defined __gnu_hurd__ || defined _WIN32 || defined __APPLE__ ||       \
    defined __FreeBSD__

#	define IE_ARRAY_SIZE_AWARE_COMPAT

namespace ie {
// obtain size of block of memory allocated from heap
inline size_t
usable_memory_size(void* p)
{
#	if defined _WIN32
	// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/msize?view=msvc-170
	return _msize(p);
#	elif defined __APPLE__
	return malloc_size(p);
#	elif defined __gnu_hurd__ || __linux__ || defined __FreeBSD__
	// https://linux.die.net/man/3/malloc_usable_size
	return malloc_usable_size(p);
#	endif
}
} // namespace ie

#endif // defined __GNUC__ || defined _WIN32 || defined __APPLE__

/*----------------------------------------------------------------------------*/

namespace ie {

namespace internals {
struct dummy1
{
};
struct dummy2
{
};
} // namespace internals

template <class T, bool make_reallocable = true, bool make_size_aware = false>
struct malloc_allocator
    : public std::conditional<make_reallocable, reallocable_allocator, internals::dummy1>::type
#ifdef IE_ARRAY_SIZE_AWARE_COMPAT
    ,
      public std::conditional<make_size_aware, size_aware_allocator, internals::dummy2>::type
#endif // IE_ARRAY_SIZE_AWARE_COMPAT
{
public:
	using value_type = T;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using reference = value_type&;
	using const_reference = const value_type&;
	using difference_type = ptrdiff_t;
	using size_type = size_t;

	template <class U>
	struct rebind
	{
		typedef malloc_allocator<U, make_reallocable, make_size_aware> other;
	};

public:
	malloc_allocator() noexcept = default;
	malloc_allocator(malloc_allocator const&) noexcept { }
	template <class U>
	explicit malloc_allocator(malloc_allocator<U> const&) noexcept
	{
	}

	pointer address(reference x) const { return &x; }
	const_pointer address(const_reference x) const { return &x; }

	pointer allocate(size_type n, const void* /*hint*/ = nullptr)
	{
		auto const ret = reinterpret_cast<pointer>(malloc(n * sizeof(value_type)));
		if (ret == nullptr) { throw std::bad_alloc(); }
		return ret;
	}

	void deallocate(pointer p, size_type) { free(p); }

	size_type max_size() const noexcept // NOLINT(modernize-use-nodiscard)
	{
		size_type max = static_cast<size_type>(-1) / sizeof(value_type);
		return (max > 0 ? max : 1);
	}

	template <typename U, typename... Args>
	void construct(U* p, Args&&... args)
	{
		::new (p) U(std::forward<Args>(args)...);
	}

	void destroy(pointer p) { p->~value_type(); }

#ifdef IE_ARRAY_SIZE_AWARE_COMPAT
	size_type usable_size(const_pointer p) const
	{
		return usable_memory_size(const_cast<pointer>(p)) / sizeof(value_type);
	}
#endif // IE_ARRAY_SIZE_AWARE_COMPAT

	pointer realloc(pointer p, size_type const n)
	{
		auto const ret = reinterpret_cast<pointer>(::realloc(p, n * sizeof(value_type)));
		if (ret == nullptr) { throw std::bad_alloc(); }
		return ret;
	}
};

/// Uses the C standard malloc,free,realloc to provide the "reallocable" idiom.
/// If GNU extensions are available, it uses the malloc_usable_size function to provide the
/// "size-aware" idiom.
template <>
class malloc_allocator<void>
{
public:
	using pointer = void*;
	using const_pointer = const void*;
	using value_type = void;

	template <typename U>
	struct rebind
	{
		typedef malloc_allocator<U> other;
	};
};

} // namespace ie

/*----------------------------------------------------------------------------*/
// The growing strategy
// When a ie::vector object needs to grow (using emplace_back for instance), it has to decide
// about its new capacity size. The first solution would just be to add the necessary space,
// but this can lead to quadratic growth performance
// (see http://www.drdobbs.com/c-made-easier-how-vectors-grow/184401375 for a nice explanation
// of this phenomena).

namespace ie {

// just return the wanted capacity
struct recommended_size_dummy
{
	template <class SizeType>
	static inline SizeType
	recommended(SizeType const /*ms*/, SizeType const /*old_cap*/, SizeType const new_cap)
	{
		return new_cap;
	}
};

// multiply the old capacity by a rational fraction. This is the one used by default with 3/2
template <size_t Num = 3, size_t Dem = 2>
struct recommended_size_multiply_by
{
	static_assert(Num > Dem, "Num <= Dem !");

	template <class SizeType>
	static inline SizeType
	recommended(SizeType const ms, SizeType const old_cap, SizeType const new_cap)
	{
#ifndef _MSC_VER
		static_assert(
		    Num < std::numeric_limits<SizeType>::max(),
		    "Num is too big for current "
		    "size_type");
		static_assert(
		    Dem < std::numeric_limits<SizeType>::max(),
		    "Dem is too big for current "
		    "size_type");
#endif

		if (old_cap >= ((ms / Num) * Dem)) { return ms; }
		return std::max((SizeType)((Num * old_cap + (Dem - 1)) / Dem), new_cap);
	}
};

// just add a constant the old capacity
template <size_t N = 15>
struct recommended_size_add_by
{
	template <class SizeType>
	static inline SizeType
	recommended(SizeType const& ms, SizeType const old_cap, SizeType const new_cap)
	{
#ifndef _MSC_VER
		static_assert(N < std::numeric_limits<SizeType>::max(), "N is too big for current "
		                                                        "size_type");
#endif
		static constexpr SizeType N_ = N;

		if (old_cap >= (ms - N_)) { return ms; }
		return std::max((SizeType)(old_cap + N_), new_cap);
	}
};

// The default one is used with a factor of 1.5, which allows for a better memory usage with
// common allocators.
// (see https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md for an
// explanation).
using default_recommended_size = recommended_size_multiply_by<3, 2>;

} // namespace ie

/*----------------------------------------------------------------------------*/

namespace ie {

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
class vector;

namespace internals {

template <class Alloc, class SizeType, bool is_size_aware>
struct size_storage_impl;

template <class Alloc, class SizeType>
struct size_storage_impl<Alloc, SizeType, false>
{
	using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;
	using size_type = SizeType;

public:
	size_storage_impl() : _n(0), _alloc(0) { }

	size_storage_impl(size_storage_impl const& o) : _n(o._n), _alloc(o._alloc) { }

	size_storage_impl(size_type const n, size_type const alloc) : _n(n), _alloc(alloc) { }

public:
	void set_size(size_type const n) { _n = n; }
	void set_storage_size(size_type const n) { _alloc = n; }
	void set_zero()
	{
		_n = 0;
		_alloc = 0;
	}

	size_type size() const { return _n; }
	size_type storage_size(Alloc const&, const_pointer) const { return _alloc; }

private:
	size_type _n;
	size_type _alloc;
};

template <class Alloc, class SizeType>
struct size_storage_impl<Alloc, SizeType, true>
{
	using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;
	using size_type = SizeType;

public:
	size_storage_impl() : _n(0) { }

	size_storage_impl(size_storage_impl const& o) : _n(o._n) { }

	size_storage_impl(size_type const n, size_type const) : _n(n) { }

public:
	void set_size(size_type const n) { _n = n; }
	void set_storage_size(size_type const) { }
	void set_zero() { _n = 0; }

	size_type size() const { return _n; }
	size_type storage_size(Alloc const& a, const_pointer p) const { return a.usable_size(p); }

private:
	size_type _n;
};

template <class Alloc, class SizeType>
using size_storage = size_storage_impl<Alloc, SizeType, is_size_aware_allocator<Alloc>::value>;

template <bool is_reallocable>
struct pod_reallocate_impl;

template <>
struct pod_reallocate_impl<true>
{
	template <class PS, class Pointer, class SizeType>
	static inline Pointer
	reallocate(PS& ps, Pointer buf, const SizeType, const SizeType, const SizeType new_size)
	{
		return ps.allocator().realloc(buf, new_size);
	}
};

template <>
struct pod_reallocate_impl<false>
{
	template <class PS, class Pointer, class SizeType>
	static inline Pointer reallocate(
	    PS& ps,
	    Pointer buf,
	    const SizeType old_size,
	    const SizeType nobjs,
	    const SizeType new_size)
	{
		return static_cast<typename PS::base_type&>(ps)
		    .reallocate(buf, old_size, nobjs, new_size);
	}
};


template <
    class T,
    class AllocOrg,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow,
    class Storage>
class vector_storage_base : public Alloc
{
	friend class vector<T, AllocOrg, SizeType, RecommendedSize, check_size_overflow>;

	template <bool IR>
	friend struct pod_reallocate_impl;

protected:
	using storage_type = Storage;
	using allocator_type = Alloc;
	using value_type = T;
	using pointer = typename std::allocator_traits<allocator_type>::pointer;
	using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
	using size_type = SizeType;
	using recommended_size_type = RecommendedSize;

protected:
	explicit vector_storage_base(allocator_type const& alloc = allocator_type())
	    : allocator_type(alloc), _buf(nullptr), _sizes(0, 0){};

	vector_storage_base(
	    vector_storage_base const& o,
	    allocator_type const& alloc = allocator_type())
	    : Alloc(alloc), _buf(nullptr)
	{
		force_allocate(o.size());
		force_size(o.size());
		storage_().construct_copy(begin(), o.begin(), size());
	}

	vector_storage_base(
	    vector_storage_base&& o,
	    allocator_type const& alloc = allocator_type()) noexcept
	    : Alloc(alloc), _buf(o._buf), _sizes(o._sizes)
	{
		o._buf = nullptr;
		o._sizes.set_zero();
	}

	~vector_storage_base() { free(); }

public:
	storage_type& storage_() { return static_cast<storage_type&>(*this); }

public:
	vector_storage_base& operator=(vector_storage_base const& o)
	{
		// self-assignment already checked!
		allocate_if_needed(o.size());
		storage_().copy(begin(), o.begin(), size());
		storage_().construct_copy(at(size()), o.at(size()), o.size() - size());
		force_size(o.size());
		return *this;
	}

	vector_storage_base& operator=(vector_storage_base&& o) noexcept
	{
		// self-assignment already checked!
		free();
		_buf = o._buf;
		_sizes = o._sizes;
		o._buf = nullptr;
		o._sizes.set_zero();
		return *this;
	}

protected:
	allocator_type allocator() { return static_cast<allocator_type&>(*this); }

	allocator_type const& allocator() const
	{
		return static_cast<allocator_type const&>(*this);
	}

	void allocate_if_needed(const size_type n)
	{
		if (n <= storage_size()) { return; }
		force_allocate(n);
	}

	inline size_type compute_size_grow(size_type const grow_by) const
	{
		const size_type size_ = size();
		if (check_size_overflow && (size_ > (max_size() - grow_by)))
		{
			throw std::length_error("size will overflow");
		}
		return size_ + grow_by;
	}

	size_type max_size() const
	{
		return std::min(
		    (size_t)allocator().max_size(), (size_t)std::numeric_limits<size_type>::max());
	}

	size_type grow_if_needed(const size_type grow_by)
	{
		const size_type n = compute_size_grow(grow_by);
		const size_type cap = storage_size();
		if (n <= cap) { return n; }
		const size_type new_alloc = recommended_size_type::recommended(max_size(), cap, n);
		force_allocate(new_alloc);
		return n;
	}

	inline pointer allocate_buffer(const size_type n) { return allocator().allocate(n); }

	void force_allocate(const size_type n)
	{
		if (begin() == nullptr)
		{
			_buf = allocate_buffer(n);
			force_size(0);
		}
		else
		{
			_buf = storage_().reallocate(_buf, storage_size(), size(), n);
			force_size(std::min(n, size()));
		}

		set_storage_size(n);
	}

	void clear()
	{
		storage_().destroy(begin(), end());
		force_size(0);
	}

	bool shrink_to_fit()
	{
		if (size() == storage_size()) { return false; }

		force_allocate(size());
		return true;
	}

	void free()
	{
		if (begin())
		{
			storage_().destroy(begin(), end());
			deallocate(begin(), storage_size());
		}
	}

	void set_buffer(pointer buf, const size_type size)
	{
		_buf = buf;
		set_storage_size(size);
	}

	inline void deallocate(pointer begin, size_type const n)
	{
		allocator().deallocate(begin, n);
	}

	pointer reallocate(
	    pointer buf,
	    const size_type old_size,
	    const size_type nobjs,
	    const size_type new_size)
	{
		pointer new_buf = allocate_buffer(new_size);
		size_type nmov;
		if (new_size < nobjs)
		{
			storage_().destroy(buf + new_size, buf + nobjs);
			nmov = new_size;
		}
		else { nmov = nobjs; }
		storage_().construct_move(new_buf, buf, nmov);
		allocator().deallocate(buf, old_size);
		return new_buf;
	}

	void set_size(size_type const n)
	{
		if (n < size()) { storage_().destroy(at(n), at(size())); }
		force_size(n);
	}

	void force_size(size_type const n) { _sizes.set_size(n); }

	template <class... Args>
	void construct_args(pointer p, Args&&... args)
	{
		allocator().construct(p, std::forward<Args>(args)...);
	}

protected:
	inline pointer begin() { return _buf; }
	inline const_pointer begin() const { return _buf; }

	inline pointer at(const size_type i)
	{
		assert(i <= storage_size());
		return begin() + i;
	}
	inline const_pointer at(const size_type i) const
	{
		assert(i <= storage_size());
		return begin() + i;
	}

	inline pointer last() { return end() - 1; }
	inline const_pointer last() const { return end() - 1; }

	inline pointer end() { return _buf + size(); }
	inline const_pointer end() const { return _buf + size(); }

	inline pointer end_storage() { return _buf + storage_size(); }
	inline const_pointer end_storage() const { return _buf + storage_size(); }

	inline size_type size() const { return _sizes.size(); }
	inline size_type storage_size() const { return _sizes.storage_size(allocator(), begin()); }

	inline void set_storage_size(size_type const n) { _sizes.set_storage_size(n); }

private:
	pointer _buf;
	size_storage<allocator_type, size_type> _sizes;
};

template <
    class T,
    class AllocOrg,
    class Alloc,
    class SizeType,
    bool is_pod,
    class RecommendedSize,
    bool check_size_overflow>
class vector_storage;

template <
    class T,
    class AllocOrg,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
class vector_storage<T, AllocOrg, Alloc, SizeType, true, RecommendedSize, check_size_overflow>
    : public vector_storage_base<
          T,
          AllocOrg,
          Alloc,
          SizeType,
          RecommendedSize,
          check_size_overflow,
          vector_storage<
              T,
              AllocOrg,
              Alloc,
              SizeType,
              true,
              RecommendedSize,
              check_size_overflow>>
{
	friend class vector<T, AllocOrg, SizeType, RecommendedSize, check_size_overflow>;

	template <bool IR>
	friend struct pod_reallocate_impl;

protected:
	using base_type = vector_storage_base<
	    T,
	    AllocOrg,
	    Alloc,
	    SizeType,
	    RecommendedSize,
	    check_size_overflow,
	    vector_storage<
	        T,
	        AllocOrg,
	        Alloc,
	        SizeType,
	        true,
	        RecommendedSize,
	        check_size_overflow>>;
	friend base_type;

	using typename base_type::const_pointer;
	using typename base_type::pointer;
	using typename base_type::value_type;
	using typename base_type::size_type;
	// MSVC does not accept using typename base_type::allocator_type here!
	using allocator_type = typename base_type::allocator_type;

	using base_type::base_type;

	static inline void construct(pointer begin, pointer end, value_type const v = value_type())
	{
		for (; begin != end; ++begin) { construct(begin, v); }
	}

	static inline void construct(pointer p, value_type const v = value_type()) { *p = v; }

	static inline void destroy(pointer, pointer) { }
	static inline void destroy(pointer){};

	static inline void construct_move(pointer dst, pointer src, const size_type n)
	{
		move(dst, src, n);
	}

	static inline void
	construct_move_alias_reverse(pointer dst, pointer src, const size_type n)
	{
		move_alias(dst, src, n);
	}

	static inline void move(pointer dst, pointer src, const size_type n) { copy(dst, src, n); }

	static inline void move_alias(pointer dst, pointer src, const size_type n)
	{
		memmove(dst, src, n * sizeof(value_type));
	}

	static inline void construct_copy(pointer dst, const_pointer src, const size_type n)
	{
		copy(dst, src, n);
	}

	static inline void copy(pointer dst, const_pointer src, const size_type n)
	{
		memcpy(dst, src, n * sizeof(value_type));
	}

	template <class InputIterator>
	static inline void construct_copy_from_iterator(pointer p, InputIterator it)
	{
		*p = *it;
	}

	static inline void
	construct_copy(pointer dst, const_pointer src_begin, const_pointer src_end)
	{
		construct_copy(dst, std::distance(src_begin, src_end));
	}

	static inline void construct_move(pointer dst, pointer src_begin, pointer src_end)
	{
		construct_move(dst, src_begin, std::distance(src_begin, src_end));
	}

	static inline void
	construct_move_alias_reverse(pointer dst, pointer src_begin, pointer src_end)
	{
		construct_move_alias_reverse(dst, src_begin, std::distance(src_begin, src_end));
	}

	static inline void move(pointer dst, pointer src_begin, pointer src_end)
	{
		move(dst, src_begin, std::distance(src_begin, src_end));
	}

	static inline void move_alias(pointer dst, pointer src_begin, pointer src_end)
	{
		move_alias(dst, src_begin, std::distance(src_begin, src_end));
	}

	static inline void copy(pointer dst, const_pointer src_begin, const_pointer src_end)
	{
		copy(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline pointer reallocate(
	    pointer buf,
	    const size_type old_size,
	    const size_type nobjs,
	    const size_type new_size)
	{
		return pod_reallocate_impl<is_reallocable_allocator<allocator_type>::value>::
		    reallocate(*this, buf, old_size, nobjs, new_size);
	}

	inline static bool equals(
	    const_pointer const start_a,
	    size_type const size_a,
	    const_pointer const start_b,
	    size_type const size_b)
	{
		if (size_a != size_b) { return false; }
		return memcmp(start_a, start_b, size_a * sizeof(value_type)) == 0;
	}
};

template <
    class T,
    class AllocOrg,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
class vector_storage<T, AllocOrg, Alloc, SizeType, false, RecommendedSize, check_size_overflow>
    : public vector_storage_base<
          T,
          AllocOrg,
          Alloc,
          SizeType,
          RecommendedSize,
          check_size_overflow,
          vector_storage<
              T,
              AllocOrg,
              Alloc,
              SizeType,
              false,
              RecommendedSize,
              check_size_overflow>>
{
	friend class vector<T, AllocOrg, SizeType, RecommendedSize, check_size_overflow>;

	using base_type = vector_storage_base<
	    T,
	    AllocOrg,
	    Alloc,
	    SizeType,
	    RecommendedSize,
	    check_size_overflow,
	    vector_storage<
	        T,
	        AllocOrg,
	        Alloc,
	        SizeType,
	        false,
	        RecommendedSize,
	        check_size_overflow>>;
	friend base_type;

	using typename base_type::const_pointer;
	using typename base_type::pointer;
	using typename base_type::value_type;
	using typename base_type::size_type;

protected:
	using allocator_type = Alloc;

protected:
	using base_type::base_type;

	inline void construct(pointer begin, pointer end)
	{
		for (; begin != end; ++begin) { this->allocator().construct(begin); }
	}

	inline void construct(pointer p, value_type const& v)
	{
		this->allocator().construct(p, v);
	}

	inline void construct(pointer p, value_type&& v)
	{
		this->allocator().construct(p, std::move(v));
	}

	inline void construct(pointer begin, pointer end, value_type const& v)
	{
		for (; begin != end; ++begin) { construct(begin, v); }
	}

	inline void destroy(pointer begin, pointer end)
	{
		for (; end != begin; ++begin) { destroy(begin); }
	}

	inline void destroy(pointer p) { this->allocator().destroy(p); }

	inline void construct_copy(pointer dst, const_pointer src, const size_type n)
	{
		for (size_type i = 0; i < n; i++) { construct(&dst[i], src[i]); }
	}

	inline void construct_move(pointer dst, pointer src, const size_type n)
	{
		for (size_type i = 0; i < n; i++) { construct(&dst[i], std::move(src[i])); }
	}

	void construct_move_alias_reverse(pointer dst, pointer src, const size_type n)
	{
		if (n == 0) { return; }
		for (size_type i = n - 1; i > 0; i--) { construct(&dst[i], std::move(src[i])); }
		new (&dst[0]) value_type(std::move(src[0]));
	}

	inline static void move(pointer dst, pointer src, const size_type n)
	{
		for (size_type i = 0; i < n; i++) { dst[i] = std::move(src[i]); }
	}

	inline static void move_alias(pointer dst, pointer src, const size_type n)
	{
		move(dst, src, n);
	}

	inline static void copy(pointer dst, const_pointer src, const size_type n)
	{
		for (size_type i = 0; i < n; i++) { dst[i] = src[i]; }
	}

	template <class InputIterator>
	inline void construct_copy_from_iterator(pointer p, InputIterator it)
	{
		construct(p, *it);
	}

	inline void construct_copy(pointer dst, const_pointer src_begin, const_pointer src_end)
	{
		construct_copy(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline void construct_move(pointer dst, pointer src_begin, pointer src_end)
	{
		construct_move(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline void construct_move_alias_reverse(pointer dst, pointer src_begin, pointer src_end)
	{
		construct_move_alias_reverse(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline void move(pointer dst, pointer src_begin, pointer src_end)
	{
		move(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline void move_alias(pointer dst, pointer src_begin, pointer src_end)
	{
		move_alias(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline void copy(pointer dst, const_pointer src_begin, const_pointer src_end)
	{
		copy(dst, src_begin, std::distance(src_begin, src_end));
	}

	inline static bool equals(
	    const_pointer const start_a,
	    size_type const size_a,
	    const_pointer const start_b,
	    size_type const size_b)
	{
#if defined _MSC_VER || __cplusplus > 201103L
		return std::equal(start_a, start_a + size_a, start_b, start_b + size_b);
#else
		if (size_a != size_b) { return false; }
		// This is deprecated by MSVC 2015!
		return std::equal(start_a, start_a + size_a, start_b);
#endif
	}
};

} // namespace internals

} // namespace ie

/*----------------------------------------------------------------------------*/

namespace ie {

/// \class vector
///  \brief vector interface
///
/// \tparam T the type of objects within the container
/// \tparam Alloc the allocator used. Can be a standard compliant allocator or an advanced allocator as ie::malloc_allocator
/// \tparam SizeType defines the type of unsigned integers used to store the number of objects and allocated size of the vector object
/// \tparam RecommendedSize class that follows the "recommended size" interface to describe the growing strategy
/// \tparam check_size_overflow Integer overflow checks are done in the function that needs to
/// 	enlarge the size of the container (like emplace_back). If such overflow occurs, an
/// 	std::length_error exception is thrown.
///
/// 	Please note that they only occur at the level of the number of objects inside the
/// 	container, not its capacity. This issue at the "capacity level" is handled by the
/// 	growing strategy.
///
/// 	This can be used to disable integer overflow checks	when the container grows
/// 	(for performance reason). This can be dangerous if you don't know what you are doing!
///
/// This is the main vector interface. It is compliant with the C++11 standard of std::vector.
/// Thus, as a reference, the documentation available here
/// (http://en.cppreference.com/w/cpp/container/vector) works for this object.
///
/// See the README.rst file for more information about the extra template
/// arguments and their usefulness.
template <
    class T,
    class Alloc = std::allocator<T>,
    class SizeType = size_t,
    class RecommendedSize = default_recommended_size,
    bool check_size_overflow = true>
class vector
{
public:
	using value_type = T;
	using allocator_type = typename Alloc::template rebind<T>::other;
	using size_type = SizeType;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = value_type const&;
	using recommended_size_type = RecommendedSize;

	static constexpr bool is_pod = std::is_pod<value_type>::value;

	static_assert(
	    std::numeric_limits<size_type>::is_signed == false,
	    "template parameter \"SizeType\" must be an unsigned integer type!");

private:
	using storage_type = internals::vector_storage<
	    value_type,
	    Alloc,
	    allocator_type,
	    size_type,
	    is_pod,
	    recommended_size_type,
	    check_size_overflow>;

public:
	using pointer = typename storage_type::pointer;
	using const_pointer = typename storage_type::const_pointer;

public:
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	vector() = default;

	explicit vector(allocator_type const& alloc) : _storage(alloc) { }

	vector(vector const& o) : _storage(o._storage) { }

	vector(vector&& o) noexcept : _storage(std::move(o._storage)) { }

	vector(std::initializer_list<value_type> const& il)
	{
		_storage.force_allocate(il.size());
		_storage.force_size(il.size());
		size_type i = 0;
		for (value_type const& v : il)
		{
			_storage.construct(_storage.at(i), v);
			i++;
		}
	}

public:
	void reserve(const size_type n) { _storage.allocate_if_needed(n); }

	void resize(const size_type n)
	{
		size_type old_size = size();
		_storage.allocate_if_needed(n);
		if (n > old_size)
		{
			_storage.construct(_storage.begin() + old_size, _storage.begin() + n);
		}
		_storage.set_size(n);
	}

	void resize(const size_type n, value_type const& v)
	{
		size_type old_size = size();
		_storage.allocate_if_needed(n);
		if (n > old_size)
		{
			_storage.construct(_storage.begin() + old_size, _storage.begin() + n, v);
		}
		_storage.set_size(n);
	}

	void reserve_fit(const size_type n) { _storage.force_allocate(n); }

	/// The resize_no_construct function will change the actual size of the container to the
	/// user-supplied one without creating underlying objects.
	/// \warning When using this API with non-POD types, the user is responsible for the
	/// 	creation of the new objects! Maybe this feature will be only available for
	/// 	POD-types in the future.
	void resize_no_construct(const size_type n)
	{
		_storage.allocate_if_needed(n);
		_storage.set_size(n);
	}

	void resize_fit(const size_type n)
	{
		size_type old_size = size();
		_storage.force_allocate(n);
		if (n > old_size)
		{
			_storage.construct(_storage.begin() + old_size, _storage.begin() + n);
		}
		_storage.set_size(n);
	}

	void shrink_to_fit() { _storage.shrink_to_fit(); }

	size_type size() const { return _storage.size(); }
	size_type max_size() const { return _storage.max_size(); }

	size_type capacity() const { return _storage.storage_size(); }
	bool empty() const { return _storage.size() == 0; }

	template <class InputIterator>
	void assign(InputIterator first, InputIterator last)
	{
		const size_type n = std::distance(first, last);
		clear();
		_storage.allocate_if_needed(n);
		size_type i = 0;
		for (; first != last; ++first)
		{
			_storage.construct_copy_from_iterator(_storage.at(i), first);
			i++;
		}
		_storage.set_size(n);
	}

	void assign(std::initializer_list<value_type> const& li) { assign(li.begin(), li.end()); }

	void assign(size_type const n, value_type const& v)
	{
		clear();
		_storage.allocate_if_needed(n);
		_storage.set_size(n);
		_storage.construct(_storage.begin(), _storage.end(), v);
	}

	void push_back(value_type const& v)
	{
		const size_type new_size = _storage.grow_if_needed(1);
		_storage.construct(_storage.end(), v);
		_storage.set_size(new_size);
	}

	void pop_back()
	{
		_storage.destroy(_storage.last());
		_storage.force_size(size() - 1);
	}

	void swap(vector& x) { std::swap(_storage, x._storage); }

	void clear() { _storage.clear(); }

	template <class... Args>
	void emplace_back(Args&&... args)
	{
		const size_type new_size = _storage.grow_if_needed(1);
		//_storage.allocate_if_needed(1);
		_storage.construct_args(_storage.end(), std::forward<Args>(args)...);
		_storage.force_size(new_size);
	}


	template <class InputIt>
	iterator insert(
	    typename std::enable_if<
	        std::is_base_of<std::input_iterator_tag,
	                        typename std::iterator_traits<InputIt>::iterator_category>::value,
	        const_iterator>::type pos,
	    InputIt first,
	    InputIt last)
	{
		return insert_gen(
		    pos, std::distance(first, last), [&first, this](pointer start, pointer end) {
			    for (; start != end; start++)
			    {
				    this->_storage.construct_copy_from_iterator(start, first++);
			    }
		    });
	}

	iterator insert(const_iterator pos, size_type const count, value_type const& v)
	{
		return insert_gen(pos, count, [this, &v](pointer start, pointer end) {
			this->_storage.construct(start, end, v);
		});
	}

	template <class... Args>
	iterator emplace(const_iterator pos, Args&&... args)
	{
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 9) || __GNUC__ >= 5
		return insert_gen(pos, 1, [this, &args...](pointer start, pointer) {
			this->_storage.construct_args(start, std::forward<Args>(args)...);
		});
#else
		// AG: lambda with variadic arguments only works starting with GCC 4.9.
		// Copy/paste the code otherwise... :/
		assert(pos <= cend());
		const size_type npos = std::distance(cbegin(), pos);
		const size_type new_size = compute_size_grow(1);
		pointer buf;
		size_type alloc_size = 0;
		const size_type cap = _storage.storage_size();
		if (_storage.begin() == nullptr || cap < new_size)
		{
			alloc_size = recommended_size_type::recommended(max_size(), cap, new_size);
			buf = _storage.allocate_buffer(alloc_size);
			_storage.construct_move(buf, _storage.begin(), _storage.begin() + npos);
		}
		else { buf = _storage.begin(); }
		// Make room for the new objects
		_storage.construct_move_alias_reverse(
		    buf + npos + 1, _storage.begin() + npos, _storage.end());

		// Insert objects
		_storage.construct_args(&buf[npos], std::forward<Args>(args)...);

		if (alloc_size > 0)
		{
			_storage.destroy(_storage.begin(), _storage.end());
			_storage.deallocate(_storage.begin(), _storage.storage_size());
			_storage.set_buffer(buf, alloc_size);
		}
		_storage.force_size(new_size);
		return iterator(_storage.begin() + npos);
#endif
	}

	iterator insert(const_iterator pos, value_type&& v) { return emplace(pos, std::move(v)); }

	iterator insert(const_iterator pos, value_type const& v) { return insert(pos, 1, v); }

	iterator insert(const_iterator pos, std::initializer_list<value_type> const& il)
	{
		return insert(pos, il.begin(), il.end());
	}

	iterator erase(const_iterator first, const_iterator last)
	{
		assert(first <= last);
		assert(last <= cend());
		assert(first < cend());
		if (first == last) { return begin() + std::distance(cbegin(), first); }
		_storage.move_alias(
		    begin() + std::distance(cbegin(), first), begin() + std::distance(cbegin(), last),
		    end());
		_storage.set_size(size() - std::distance(first, last));
		return iterator(first);
	}

	inline iterator erase(const_iterator pos)
	{
		assert(pos < cend());
		return erase(pos, pos + 1);
	}

	allocator_type get_allocator() const { return _storage.allocator(); }

	reference at(const size_type i) { return *_storage.at(i); }

	const_reference at(const size_type i) const { return *_storage.at(i); }

	reference operator[](const size_type i) { return at(i); }
	const_reference operator[](const size_type i) const { return at(i); }

	reference front() { return *_storage.begin(); }
	const_reference front() const { return *_storage.begin(); }

	reference back() { return *_storage.last(); }
	const_reference back() const { return *_storage.last(); }

	pointer data() noexcept { return _storage.begin(); }
	const_pointer data() const noexcept { return _storage.begin(); }

	iterator begin() { return iterator(_storage.begin()); }
	iterator end() { return iterator(_storage.end()); }

	const_iterator cbegin() { return const_iterator(_storage.begin()); }
	const_iterator cend() { return const_iterator(_storage.end()); }

	const_iterator begin() const { return const_iterator(_storage.begin()); }
	const_iterator end() const { return const_iterator(_storage.end()); }

	const_iterator cbegin() const { return const_iterator(_storage.begin()); }
	const_iterator cend() const { return const_iterator(_storage.end()); }

	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }

	const_reverse_iterator crbegin() { return const_reverse_iterator(end()); }
	const_reverse_iterator crend() { return const_reverse_iterator(begin()); }

	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

public:
	inline bool operator==(vector const& o) const
	{
		return storage_type::equals(_storage.begin(), size(), o._storage.begin(), o.size());
	}

	inline bool operator<(vector const& o) const
	{
		return std::lexicographical_compare(begin(), end(), o.begin(), o.end());
	}

public:
	vector& operator=(vector const& o)
	{
		if (&o != this) { _storage = o._storage; }
		return *this;
	}

	vector& operator=(vector&& o) noexcept
	{
		if (&o != this) { _storage = std::move(o._storage); }
		return *this;
	}

private:
	template <class F>
	inline iterator insert_gen(const_iterator pos, size_type const count, F const& fins)
	{
		assert(pos <= cend());
		if (count == 0) { return iterator(pos); }
		const size_type npos = std::distance(cbegin(), pos);
		const size_type new_size = compute_size_grow(count);
		pointer buf;
		size_type alloc_size = 0;
		const size_type cap = _storage.storage_size();
		if (_storage.begin() == nullptr || cap < new_size)
		{
			alloc_size = recommended_size_type::recommended(max_size(), cap, new_size);
			buf = _storage.allocate_buffer(alloc_size);
			_storage.construct_move(buf, _storage.begin(), _storage.begin() + npos);
		}
		else { buf = _storage.begin(); }
		// Make room for the new objects
		_storage.construct_move_alias_reverse(
		    buf + npos + count, _storage.begin() + npos, _storage.end());

		// Insert objects
		fins(&buf[npos], &buf[npos + count]);

		if (alloc_size > 0)
		{
			_storage.destroy(_storage.begin(), _storage.end());
			_storage.deallocate(_storage.begin(), _storage.storage_size());
			_storage.set_buffer(buf, alloc_size);
		}
		_storage.force_size(new_size);
		return iterator(_storage.begin() + npos);
	}

	inline size_type compute_size_grow(size_type const grow_by) const
	{
		return _storage.compute_size_grow(grow_by);
	}

private:
	storage_type _storage;
};

} // namespace ie

namespace std {

template <class T, class Alloc, class SizeType, class RS, bool co>
void
swap(ie::vector<T, Alloc, SizeType, RS, co>& a, ie::vector<T, Alloc, SizeType, RS, co>& b)
{
	a.swap(b);
}

} // namespace std

#endif // IE_ARRAY_HPP
