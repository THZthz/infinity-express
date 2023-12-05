/// \file Array.hpp
/// \brief A stl-like vector implementation but with more c-like memory control.
/// 	This array allows you to manipulate its data freely(for some dirty and ugly hack).
///		But be aware that this implementation use memmove, which knows none about c++ copy
///		behavior of a class(especially virtual class, in which case virtual table may be
/// 	overwritten).
/// \example
/// 	// empty classes are trivial
/// 	struct Trivial1 {};
///
///		// all special members are implicit
/// 	struct Trivial2 {
/// 	    int x;
/// 	};
///
/// 	struct Trivial3 : Trivial2 { // base class is trivial
/// 	    Trivial3() = default; // not a user-provided ctor
/// 	    int y;
/// 	};
///
/// 	struct Trivial4 {
/// 	public:
/// 	    int a;
/// 	private: // no restrictions on access modifiers
/// 	    int b;
/// 	};
///
/// 	struct Trivial5 {
/// 	    Trivial1 a;
/// 	    Trivial2 b;
/// 	    Trivial3 c;
/// 	    Trivial4 d;
/// 	};
///
/// 	struct Trivial6 {
/// 	    Trivial2 a[23];
/// 	};
///
/// 	struct Trivial7 {
/// 	    Trivial6 c;
/// 	    void f(); // it's okay to have non-virtual functions
/// 	};
///
/// 	struct Trivial8 {
/// 	    int x;
/// 	    static NonTrivial1 y; // no restrictions on static members
/// 	}
///
/// 	struct Trivial9 {
/// 	    Trivial9() = default; // not user-provided
/// 	    // a regular constructor is okay because we still have default ctor
/// 	    Trivial9(int x) : x(x) {};
/// 	    int x;
/// 	}
///
/// 	struct NonTrivial1 : Trivial 3 {
/// 	    virtual f(); // virtual members make non-trivial ctors
/// 	}
///
/// 	struct NonTrivial2 {
/// 	    NonTrivial2() : z(42) {} // user-provided ctor
/// 	    int z;
/// 	}
///
/// 	struct NonTrivial3 {
/// 	    NonTrivial3(); // user-provided ctor
/// 	    int w;
/// 	}
/// 	NonTrivial3::NonTrivial3() = default; // defaulted but not on first declaration
///		// still counts as user-provided
/// 	struct NonTrivial5 {
/// 	    virtual ~NonTrivial5(); // virtual destructors are not trivial
/// 	};

#ifndef IE_ARRAY_HPP
#define IE_ARRAY_HPP

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <memory>

#include "Memory.hpp"
#include "Macros.hpp"

// base of ie::vector

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
	typedef typename std::allocator_traits<Alloc>::const_pointer const_pointer;
	typedef SizeType size_type;

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
	    const SizeType n_objs,
	    const SizeType new_size)
	{
		return static_cast<typename PS::base_type&>(ps)
		    .reallocate(buf, old_size, n_objs, new_size);
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
	typedef Storage storage_type;
	typedef Alloc allocator_type;
	typedef T value_type;
	typedef typename std::allocator_traits<allocator_type>::pointer pointer;
	typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;
	typedef SizeType size_type;
	typedef RecommendedSize recommended_size_type;

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

	virtual pointer reallocate(
	    pointer buf,
	    const size_type old_size,
	    const size_type n_objs,
	    const size_type new_size)
	{
		pointer new_buf = allocate_buffer(new_size);
		size_type n_mov;
		if (new_size < n_objs)
		{
			storage_().destroy(buf + new_size, buf + n_objs);
			n_mov = new_size;
		}
		else { n_mov = n_objs; }
		storage_().construct_move(new_buf, buf, n_mov);
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
	typedef vector_storage_base<
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
	    base_type;
	friend base_type;

	using typename base_type::const_pointer;
	using typename base_type::pointer;
	using typename base_type::value_type;
	using typename base_type::size_type;
	// MSVC does not accept using typename base_type::allocator_type here!
	typedef typename base_type::allocator_type allocator_type;

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
	    const size_type n_objs,
	    const size_type new_size)
	{
		return pod_reallocate_impl<is_reallocable_allocator<allocator_type>::value>::
		    reallocate(*this, buf, old_size, n_objs, new_size);
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
	friend class vector<T, AllocOrg, SizeType, RecommendedSize, check_size_overflow>;

	using typename base_type::const_pointer;
	using typename base_type::pointer;
	using typename base_type::value_type;
	using typename base_type::size_type;

protected:
	typedef Alloc allocator_type;

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

namespace ie {

/// \brief Vector compliant with the C++11 standard of std::vector.
template <
    class T,
    class Alloc = malloc_allocator<T>,
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
	using const_reference = const value_type&;
	using recommended_size_type = RecommendedSize;

	static constexpr bool is_pod = std::is_pod<value_type>::value;

	static_assert(
	    std::numeric_limits<size_type>::is_signed == false,
	    "SizeType must be an unsigned integer type!");

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

	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

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
	explicit vector(size_type reserved) { reserve(reserved); }

	void reserve(size_type n);
	void resize(size_type n);
	void resize(size_type n, const value_type& v);
	void reserve_fit(size_type n);
	void resize_no_construct(size_type n);
	void resize_fit(size_type n);
	void shrink_to_fit();
	void clear();

	size_type size() const;
	size_type max_size() const;
	size_type capacity() const;
	bool empty() const;

	template <class InputIterator>
	void assign(InputIterator first, InputIterator last);
	void assign(std::initializer_list<value_type> const& li);
	void assign(size_type n, value_type const& v);

	void swap(vector& x);

	void push_back(value_type const& v);
	void pop_back();
	template <class... Args>
	void emplace_back(Args&&... args);
	template <class InputIt>
	iterator insert(
	    typename std::enable_if<
	        std::is_base_of<std::input_iterator_tag,
	                        typename std::iterator_traits<InputIt>::iterator_category>::value,
	        const_iterator>::type pos,
	    InputIt first,
	    InputIt last);
	iterator insert(const_iterator pos, size_type count, value_type const& v);
	template <class... Args>
	iterator emplace(const_iterator pos, Args&&... args);
	iterator insert(const_iterator pos, value_type&& v);
	iterator insert(const_iterator pos, value_type const& v);
	iterator insert(const_iterator pos, std::initializer_list<value_type> const& il);

	iterator erase(const_iterator first, const_iterator last);
	inline iterator erase(const_iterator pos);

	allocator_type get_allocator() const;

	reference at(size_type i);
	const_reference at(size_type i) const;
	reference operator[](size_type i);
	const_reference operator[](size_type i) const;

	reference front();
	const_reference front() const;
	reference back();
	const_reference back() const;
	pointer data() noexcept;
	const_pointer data() const noexcept;
	iterator begin();
	iterator end();
	const_iterator cbegin();
	const_iterator cend();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;
	reverse_iterator rbegin();
	reverse_iterator rend();
	const_reverse_iterator crbegin();
	const_reverse_iterator crend();
	const_reverse_iterator rbegin() const;
	const_reverse_iterator rend() const;
	const_reverse_iterator crbegin() const;
	const_reverse_iterator crend() const;

	inline bool operator==(vector const& o) const;
	inline bool operator<(vector const& o) const;

	vector& operator=(vector const& o);
	vector& operator=(vector&& o) noexcept;

private:
	template <class F>
	inline iterator insert_gen(const_iterator pos, size_type count, F const& fins);
	inline size_type compute_size_grow(size_type grow_by) const;

	storage_type _storage;
};

} // namespace ie

// implementation for ie::vector.

namespace ie {

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reserve(size_type n)
{
	_storage.allocate_if_needed(n);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::resize(size_type n)
{
	size_type old_size = size();
	_storage.allocate_if_needed(n);
	if (n > old_size)
	{
		_storage.construct(_storage.begin() + old_size, _storage.begin() + n);
	}
	_storage.set_size(n);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    resize(size_type n, const value_type& v)
{
	size_type old_size = size();
	_storage.allocate_if_needed(n);
	if (n > old_size)
	{
		_storage.construct(_storage.begin() + old_size, _storage.begin() + n, v);
	}
	_storage.set_size(n);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reserve_fit(size_type n)
{
	_storage.force_allocate(n);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::resize_no_construct(
    size_type n)
{
	_storage.allocate_if_needed(n);
	_storage.set_size(n);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::resize_fit(size_type n)
{
	size_type old_size = size();
	_storage.force_allocate(n);
	if (n > old_size)
	{
		_storage.construct(_storage.begin() + old_size, _storage.begin() + n);
	}
	_storage.set_size(n);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::shrink_to_fit()
{
	_storage.shrink_to_fit();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::size_type
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::size() const
{
	return _storage.size();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::size_type
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::max_size() const
{
	return _storage.max_size();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::size_type
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::capacity() const
{
	return _storage.storage_size();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
bool
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::empty() const
{
	return _storage.size() == 0;
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
template <class InputIterator>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    assign(InputIterator first, InputIterator last)
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

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::assign(
    std::initializer_list<value_type> const& li)
{
	assign(li.begin(), li.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    assign(size_type const n, value_type const& v)
{
	clear();
	_storage.allocate_if_needed(n);
	_storage.set_size(n);
	_storage.construct(_storage.begin(), _storage.end(), v);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::push_back(
    value_type const& v)
{
	const size_type new_size = _storage.grow_if_needed(1);
	_storage.construct(_storage.end(), v);
	_storage.set_size(new_size);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::pop_back()
{
	_storage.destroy(_storage.last());
	_storage.force_size(size() - 1);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::swap(vector& x)
{
	std::swap(_storage, x._storage);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::clear()
{
	_storage.clear();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
template <class... Args>
void
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::emplace_back(Args&&... args)
{
	const size_type new_size = _storage.grow_if_needed(1);
	//_storage.allocate_if_needed(1);
	_storage.construct_args(_storage.end(), std::forward<Args>(args)...);
	_storage.force_size(new_size);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
template <class InputIt>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::insert(
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

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    insert(const_iterator pos, size_type count, value_type const& v)
{
	return insert_gen(pos, count, [this, &v](pointer start, pointer end) {
		this->_storage.construct(start, end, v);
	});
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
template <class... Args>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    emplace(const_iterator pos, Args&&... args)
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 9) || __GNUC__ >= 5
	return insert_gen(pos, 1, [this, &args...](pointer start, pointer) {
		this->_storage.construct_args(start, std::forward<Args>(args)...);
	});
#else
	// AG: lambda with variadic arguments only works starting with GCC 4.9.
	// Copy/paste the code otherwise... :/
	assert(pos <= cend());
	const size_type n_pos = std::distance(cbegin(), pos);
	const size_type new_size = compute_size_grow(1);
	pointer buf;
	size_type alloc_size = 0;
	const size_type cap = _storage.storage_size();
	if (_storage.begin() == nullptr || cap < new_size)
	{
		alloc_size = recommended_size_type::recommended(max_size(), cap, new_size);
		buf = _storage.allocate_buffer(alloc_size);
		_storage.construct_move(buf, _storage.begin(), _storage.begin() + n_pos);
	}
	else { buf = _storage.begin(); }
	// Make room for the new objects
	_storage.construct_move_alias_reverse(
	    buf + n_pos + 1, _storage.begin() + n_pos, _storage.end());

	// Insert objects
	_storage.construct_args(&buf[n_pos], std::forward<Args>(args)...);

	if (alloc_size > 0)
	{
		_storage.destroy(_storage.begin(), _storage.end());
		_storage.deallocate(_storage.begin(), _storage.storage_size());
		_storage.set_buffer(buf, alloc_size);
	}
	_storage.force_size(new_size);
	return iterator(_storage.begin() + n_pos);
#endif
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    insert(const_iterator pos, value_type&& v)
{
	return emplace(pos, std::move(v));
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    insert(const_iterator pos, value_type const& v)
{
	return insert(pos, 1, v);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    insert(const_iterator pos, std::initializer_list<value_type> const& il)
{
	return insert(pos, il.begin(), il.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    erase(const_iterator first, const_iterator last)
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

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
inline typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::erase(const_iterator pos)
{
	assert(pos < cend());
	return erase(pos, pos + 1);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::allocator_type
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::get_allocator() const
{
	return _storage.allocator();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::at(size_type i)
{
	return *_storage.at(i);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::at(size_type i) const
{
	return *_storage.at(i);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::operator[](size_type i)
{
	return at(i);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::operator[](size_type i) const
{
	return at(i);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::front()
{
	return *_storage.begin();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::front() const
{
	return *_storage.begin();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::back()
{
	return *_storage.last();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_reference
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::back() const
{
	return *_storage.last();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::pointer
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::data() noexcept
{
	return _storage.begin();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_pointer
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::data() const noexcept
{
	return _storage.begin();
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::begin()
{
	return iterator(_storage.begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::end()
{
	return iterator(_storage.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::cbegin()
{
	return const_iterator(_storage.begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::cend()
{
	return const_iterator(_storage.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::begin() const
{
	return const_iterator(_storage.begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::end() const
{
	return const_iterator(_storage.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::cbegin() const
{
	return const_iterator(_storage.begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::const_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::cend() const
{
	return const_iterator(_storage.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reverse_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::rbegin()
{
	return reverse_iterator(end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::reverse_iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::rend()
{
	return reverse_iterator(begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    const_reverse_iterator
    vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::crbegin()
{
	return const_reverse_iterator(end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    const_reverse_iterator
    vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::crend()
{
	return const_reverse_iterator(begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    const_reverse_iterator
    vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::rbegin() const
{
	return const_reverse_iterator(end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    const_reverse_iterator
    vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::rend() const
{
	return const_reverse_iterator(begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    const_reverse_iterator
    vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::crbegin() const
{
	return const_reverse_iterator(end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    const_reverse_iterator
    vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::crend() const
{
	return const_reverse_iterator(begin());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
inline bool
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
operator==(vector const& o) const
{
	return storage_type::equals(_storage.begin(), size(), o._storage.begin(), o.size());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
inline bool
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
operator<(vector const& o) const
{
	return std::lexicographical_compare(begin(), end(), o.begin(), o.end());
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>&
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::operator=(vector const& o)
{
	if (&o != this) { _storage = o._storage; }
	return *this;
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>&
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
operator=(vector&& o) noexcept
{
	if (&o != this) { _storage = std::move(o._storage); }
	return *this;
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
template <class F>
inline typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::iterator
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::
    insert_gen(const_iterator pos, size_type count, F const& fins)
{
	assert(pos <= cend());
	if (count == 0) return iterator(pos);
	const size_type n_pos = std::distance(cbegin(), pos);
	const size_type new_size = compute_size_grow(count);
	pointer buf;
	size_type alloc_size = 0;
	const size_type cap = _storage.storage_size();
	if (_storage.begin() == nullptr || cap < new_size)
	{
		alloc_size = recommended_size_type::recommended(max_size(), cap, new_size);
		buf = _storage.allocate_buffer(alloc_size);
		_storage.construct_move(buf, _storage.begin(), _storage.begin() + n_pos);
	}
	else { buf = _storage.begin(); }
	// Make room for the new objects
	_storage.construct_move_alias_reverse(
	    buf + n_pos + count, _storage.begin() + n_pos, _storage.end());

	// Insert objects
	fins(&buf[n_pos], &buf[n_pos + count]);

	if (alloc_size > 0)
	{
		_storage.destroy(_storage.begin(), _storage.end());
		_storage.deallocate(_storage.begin(), _storage.storage_size());
		_storage.set_buffer(buf, alloc_size);
	}
	_storage.force_size(new_size);
	return iterator(_storage.begin() + n_pos);
}

template <
    class T,
    class Alloc,
    class SizeType,
    class RecommendedSize,
    bool check_size_overflow>
inline typename vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::size_type
vector<T, Alloc, SizeType, RecommendedSize, check_size_overflow>::compute_size_grow(
    size_type grow_by) const
{
	return _storage.compute_size_grow(grow_by);
}

} // namespace ie

#endif // IE_ARRAY_HPP
