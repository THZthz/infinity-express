#ifndef __CANDYBOX_SMART_PTR_HPP__
#define __CANDYBOX_SMART_PTR_HPP__

//
//  smart_ptr.hpp
//
//  For convenience, this header includes the rest of the smart
//  pointer library headers.
//
//  Copyright (c) 2003 Peter Dimov  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.
//

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/*#define DISABLE_THREADS*/
/*#define HAS_SCHED_YIELD*/
#define HAS_THREADS
/*#define HAS_PTHREADS*/
/*#define SP_DISABLE_THREADS*/
#define SP_USE_STD_ATOMIC
/*#define SP_UES_PTHREADS*/
/*#define AC_DISABLE_THREADS*/
#define AC_USE_STD_ATOMIC
/*#define AC_USE_PTHREADS*/
#define SP_USE_QUICK_ALLOCATOR
/*#define SP_USE_STD_ALLOCATOR*/
/*#define HAS_NANOSLEEP*/         // the platform has the POSIX API nanosleep
/*#define ENABLE_ASSERT_HANDLER*/ // call boost::assertion_failed when assertion failed

#if defined(__GNUC__) && __GNUC__ >= 4
#  define SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#elif defined(_MSC_VER)
#  define SYMBOL_VISIBLE
#endif

#include <memory>
#include <functional>
#include <type_traits>
#include <memory>
#include <utility>
#include <limits>
#include <new>

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstdint>

// Macro to use before a function declaration/definition to designate
// the function as not returning normally (i.e. with a return statement
// or by leaving the function scope, if the function return type is void).
#if defined(_MSC_VER)
#  define NORETURN __declspec(noreturn)
#elif defined(__GNUC__) || defined(__CODEGEARC__) && defined(__clang__)
#  define NORETURN __attribute__((__noreturn__))
#elif defined(__has_attribute) && defined(__SUNPRO_CC) && (__SUNPRO_CC > 0x5130)
#  if __has_attribute(noreturn)
#    define NORETURN [[noreturn]]
#  endif
#elif defined(__has_cpp_attribute)
#  if __has_cpp_attribute(noreturn)
#    define NORETURN [[noreturn]]
#  endif
#endif

// Macro to use in place of 'inline' to force a function to be inline
#if !defined(FORCEINLINE)
#  if defined(_MSC_VER)
#    define FORCEINLINE __forceinline
#  elif defined(__GNUC__) && __GNUC__ > 3
#    define FORCEINLINE                                                                       \
      inline __attribute__((__always_inline__)) // Clang also defines __GNUC__ (as 4)
#  else
#    define FORCEINLINE inline
#  endif
#endif

namespace boost {
namespace movelib {
template <class T, class D> class unique_ptr;
} // namespace movelib
} // namespace boost

namespace boost {
using std::is_arithmetic;
using std::is_pointer;
using std::is_enum;
using std::is_function;
using std::is_scalar;
using std::is_member_pointer;
using std::is_reference;
using std::is_volatile;
using std::conditional;
using std::is_convertible;
using std::alignment_of;
using std::is_void;
using std::is_same;
using std::remove_const;
using std::remove_extent;
using std::remove_cv;
using std::extent;
using std::has_virtual_destructor;
#ifndef BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED
#  define BOOST_TT_REMOVE_REFERENCE_HPP_INCLUDED
using std::remove_reference;
#endif
//using std::is_array;

#ifndef BOOST_CORE_ADDRESSOF_HPP
#  define BOOST_CORE_ADDRESSOF_HPP
using std::addressof;
#endif

// All boost exceptions are required to derive from std::exception,
// to ensure compatibility with BOOST_NO_EXCEPTIONS.

inline void throw_exception_assert_compatibility(std::exception const &) {}

template <class E> NORETURN void throw_exception(E const &e) {
  throw_exception_assert_compatibility(e);
  throw(e);
}

using std::uintptr_t;
typedef long long long_long_type;
} // namespace boost

#if defined(ENABLE_ASSERT_HANDLER)
#  undef assert
namespace boost {
void assertion_failed(char const *expr,
                      char const *function,
                      char const *file,
                      long line); // user defined
} // namespace boost
#  define assert(expr)                                                                        \
    (!!(expr) ? ((void)0) : ::boost::assertion_failed(#expr, __func__, __FILE__, __LINE__))
#endif

/* @file_start boost/core/default_allocator.hpp */

namespace boost {
namespace default_ {

template <bool V> struct bool_constant {
  typedef bool value_type;
  typedef bool_constant type;

  static const bool value = V;

  operator bool() const noexcept { return V; }

  bool operator()() const noexcept { return V; }
};

template <bool V> const bool bool_constant<V>::value;

template <class T> struct add_reference {
  typedef T &type;
};

template <> struct add_reference<void> {
  typedef void type;
};

template <> struct add_reference<const void> {
  typedef const void type;
};

template <class T> struct default_allocator {
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef typename add_reference<T>::type reference;
  typedef typename add_reference<const T>::type const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef bool_constant<true> propagate_on_container_move_assignment;
  typedef bool_constant<true> is_always_equal;

  template <class U> struct rebind {
    typedef default_allocator<U> other;
  };

  default_allocator() = default;

  template <class U> constexpr default_allocator(const default_allocator<U> &) noexcept {}

  constexpr std::size_t max_size() const noexcept {
    return static_cast<std::size_t>(-1) / (2 < sizeof(T) ? sizeof(T) : 2);
  }

  T *allocate(std::size_t n) {
    if (n > max_size()) { boost::throw_exception(std::bad_alloc()); }
    void *p = ::operator new(sizeof(T) * n, std::nothrow);
    if (!p) { boost::throw_exception(std::bad_alloc()); }
    return static_cast<T *>(p);
  }

  void deallocate(T *p, std::size_t) { ::operator delete(p, std::nothrow); }
};

template <class T, class U>
constexpr inline bool operator==(const default_allocator<T> &,
                                 const default_allocator<U> &) noexcept {
  return true;
}

template <class T, class U>
constexpr inline bool operator!=(const default_allocator<T> &,
                                 const default_allocator<U> &) noexcept {
  return false;
}

} // namespace default_

using default_::default_allocator;

} // namespace boost

/* @file_end boost/core/default_allocator.hpp */

/* @file_start boost/core/pointer_traits.hpp */

namespace boost {
namespace detail {

struct ptr_none {};

template <class> struct ptr_valid {
  typedef void type;
};

template <class> struct ptr_first {
  typedef ptr_none type;
};

template <template <class, class...> class T, class U, class... Args>
struct ptr_first<T<U, Args...>> {
  typedef U type;
};

template <class T, class = void> struct ptr_element {
  typedef typename ptr_first<T>::type type;
};

template <class T> struct ptr_element<T, typename ptr_valid<typename T::element_type>::type> {
  typedef typename T::element_type type;
};

template <class, class = void> struct ptr_difference {
  typedef std::ptrdiff_t type;
};

template <class T>
struct ptr_difference<T, typename ptr_valid<typename T::difference_type>::type> {
  typedef typename T::difference_type type;
};

template <class, class> struct ptr_transform {};

template <template <class, class...> class T, class U, class... Args, class V>
struct ptr_transform<T<U, Args...>, V> {
  typedef T<V, Args...> type;
};

template <class T, class U, class = void> struct ptr_rebind : ptr_transform<T, U> {};

template <class T, class U>
struct ptr_rebind<T, U, typename ptr_valid<typename T::template rebind<U>>::type> {
  typedef typename T::template rebind<U> type;
};

template <class T, class E> class ptr_to_expr {
  template <class> struct result {
    char x, y;
  };

  static E &source();

  template <class O> static auto check(int) -> result<decltype(O::pointer_to(source()))>;

  template <class> static char check(long);

public:
  static constexpr bool value = sizeof(check<T>(0)) > 1;
};

template <class T, class E> struct ptr_to_expr<T *, E> {
  static constexpr bool value = true;
};

template <class T, class E> struct ptr_has_to {
  static constexpr bool value = ptr_to_expr<T, E>::value;
};

template <class T> struct ptr_has_to<T, void> {
  static constexpr bool value = false;
};

template <class T> struct ptr_has_to<T, const void> {
  static constexpr bool value = false;
};

template <class T> struct ptr_has_to<T, volatile void> {
  static constexpr bool value = false;
};

template <class T> struct ptr_has_to<T, const volatile void> {
  static constexpr bool value = false;
};

template <class T, class E, bool = ptr_has_to<T, E>::value> struct ptr_to {};

template <class T, class E> struct ptr_to<T, E, true> {
  static T pointer_to(E &v) { return T::pointer_to(v); }
};

template <class T> struct ptr_to<T *, T, true> {
  static T *pointer_to(T &v) noexcept { return boost::addressof(v); }
};

template <class T, class E> struct ptr_traits : ptr_to<T, E> {
  typedef T pointer;
  typedef E element_type;
  typedef typename ptr_difference<T>::type difference_type;

  template <class U> struct rebind_to : ptr_rebind<T, U> {};

  template <class U> using rebind = typename rebind_to<U>::type;
};

template <class T> struct ptr_traits<T, ptr_none> {};

} // namespace detail

template <class T>
struct pointer_traits : detail::ptr_traits<T, typename detail::ptr_element<T>::type> {};

template <class T> struct pointer_traits<T *> : detail::ptr_to<T *, T> {
  typedef T *pointer;
  typedef T element_type;
  typedef std::ptrdiff_t difference_type;

  template <class U> struct rebind_to {
    typedef U *type;
  };

  template <class U> using rebind = typename rebind_to<U>::type;
};

template <class T> constexpr inline T *to_address(T *v) noexcept { return v; }

// This require compiler to support type deduction from cxx14.
// An alternative for the implementation below:
//  template <class T>
//  inline typename pointer_traits<T>::element_type *
//  to_address(const T &v) noexcept
//  {
//	    return boost::to_address(v.operator->());
//  }

#if 0
namespace detail {

template <class T>
inline T *
ptr_address(T *v, int) noexcept
{
	return v;
}

template <class T>
inline auto
ptr_address(const T &v, int) noexcept -> decltype(boost::pointer_traits<T>::to_address(v))
{
	return boost::pointer_traits<T>::to_address(v);
}

template <class T>
inline auto
ptr_address(const T &v, long) noexcept
{
	return boost::detail::ptr_address(v.operator->(), 0);
}

} // namespace detail

template <class T>
inline auto
to_address(const T &v) noexcept
{
	return boost::detail::ptr_address(v, 0);
}
#else
template <class T>
inline typename pointer_traits<T>::element_type *to_address(const T &v) noexcept {
  return boost::to_address(v.operator->());
}
#endif
} // namespace boost

/* @file_end boost/core/pointer_traits.hpp */

/* @file_start boost/core/allocator_access.hpp */

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif

namespace boost {

template <class A> struct allocator_value_type {
  typedef typename A::value_type type;
};

namespace detail {

template <class A, class = void> struct alloc_ptr {
  typedef typename boost::allocator_value_type<A>::type *type;
};

template <class> struct alloc_void {
  typedef void type;
};

template <class A> struct alloc_ptr<A, typename alloc_void<typename A::pointer>::type> {
  typedef typename A::pointer type;
};

} // namespace detail

template <class A> struct allocator_pointer {
  typedef typename detail::alloc_ptr<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_const_ptr {
  typedef typename boost::pointer_traits<typename boost::allocator_pointer<A>::type>::
      template rebind_to<const typename boost::allocator_value_type<A>::type>::type type;
};

template <class A>
struct alloc_const_ptr<A, typename alloc_void<typename A::const_pointer>::type> {
  typedef typename A::const_pointer type;
};

} // namespace detail

template <class A> struct allocator_const_pointer {
  typedef typename detail::alloc_const_ptr<A>::type type;
};

namespace detail {

template <class, class> struct alloc_to {};

template <template <class, class...> class A, class T, class U, class... V>
struct alloc_to<A<U, V...>, T> {
  typedef A<T, V...> type;
};

template <class A, class T, class = void> struct alloc_rebind {
  typedef typename alloc_to<A, T>::type type;
};

template <class A, class T>
struct alloc_rebind<A, T, typename alloc_void<typename A::template rebind<T>::other>::type> {
  typedef typename A::template rebind<T>::other type;
};

} // namespace detail

template <class A, class T> struct allocator_rebind {
  typedef typename detail::alloc_rebind<A, T>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_void_ptr {
  typedef typename boost::pointer_traits<
      typename boost::allocator_pointer<A>::type>::template rebind_to<void>::type type;
};

template <class A>
struct alloc_void_ptr<A, typename alloc_void<typename A::void_pointer>::type> {
  typedef typename A::void_pointer type;
};

} // namespace detail

template <class A> struct allocator_void_pointer {
  typedef typename detail::alloc_void_ptr<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_const_void_ptr {
  typedef typename boost::pointer_traits<
      typename boost::allocator_pointer<A>::type>::template rebind_to<const void>::type type;
};

template <class A>
struct alloc_const_void_ptr<A, typename alloc_void<typename A::const_void_pointer>::type> {
  typedef typename A::const_void_pointer type;
};

} // namespace detail

template <class A> struct allocator_const_void_pointer {
  typedef typename detail::alloc_const_void_ptr<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_diff_type {
  typedef typename boost::pointer_traits<
      typename boost::allocator_pointer<A>::type>::difference_type type;
};

template <class A>
struct alloc_diff_type<A, typename alloc_void<typename A::difference_type>::type> {
  typedef typename A::difference_type type;
};

} // namespace detail

template <class A> struct allocator_difference_type {
  typedef typename detail::alloc_diff_type<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_size_type {
  typedef typename std::make_unsigned<typename boost::allocator_difference_type<A>::type>::type
      type;
};

template <class A>
struct alloc_size_type<A, typename alloc_void<typename A::size_type>::type> {
  typedef typename A::size_type type;
};

} // namespace detail

template <class A> struct allocator_size_type {
  typedef typename detail::alloc_size_type<A>::type type;
};

namespace detail {

typedef std::false_type alloc_false;

template <class A, class = void> struct alloc_pocca {
  typedef alloc_false type;
};

template <class A>
struct alloc_pocca<
    A,
    typename alloc_void<typename A::propagate_on_container_copy_assignment>::type> {
  typedef typename A::propagate_on_container_copy_assignment type;
};

} // namespace detail

template <class A, class = void> struct allocator_propagate_on_container_copy_assignment {
  typedef typename detail::alloc_pocca<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_pocma {
  typedef alloc_false type;
};

template <class A>
struct alloc_pocma<
    A,
    typename alloc_void<typename A::propagate_on_container_move_assignment>::type> {
  typedef typename A::propagate_on_container_move_assignment type;
};

} // namespace detail

template <class A> struct allocator_propagate_on_container_move_assignment {
  typedef typename detail::alloc_pocma<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_pocs {
  typedef alloc_false type;
};

template <class A>
struct alloc_pocs<A, typename alloc_void<typename A::propagate_on_container_swap>::type> {
  typedef typename A::propagate_on_container_swap type;
};

} // namespace detail

template <class A> struct allocator_propagate_on_container_swap {
  typedef typename detail::alloc_pocs<A>::type type;
};

namespace detail {

template <class A, class = void> struct alloc_equal {
  typedef typename std::is_empty<A>::type type;
};

template <class A>
struct alloc_equal<A, typename alloc_void<typename A::is_always_equal>::type> {
  typedef typename A::is_always_equal type;
};

} // namespace detail

template <class A> struct allocator_is_always_equal {
  typedef typename detail::alloc_equal<A>::type type;
};

template <class A>
inline typename allocator_pointer<A>::type
allocator_allocate(A &a, typename allocator_size_type<A>::type n) {
  return a.allocate(n);
}

template <class A>
inline void allocator_deallocate(A &a,
                                 typename allocator_pointer<A>::type p,
                                 typename allocator_size_type<A>::type n) {
  a.deallocate(p, n);
}

namespace detail {

template <class> struct alloc_no {
  char x, y;
};

template <class A> class alloc_has_allocate {
  template <class O>
  static auto check(int) -> alloc_no<decltype(std::declval<O &>().allocate(
      std::declval<typename boost::allocator_size_type<A>::type>(),
      std::declval<typename boost::allocator_const_void_pointer<A>::type>()))>;

  template <class> static char check(long);

public:
  static constexpr bool value = sizeof(check<A>(0)) > 1;
};

} // namespace detail

template <class A>
inline typename std::enable_if<detail::alloc_has_allocate<A>::value,
                               typename allocator_pointer<A>::type>::type
allocator_allocate(A &a,
                   typename allocator_size_type<A>::type n,
                   typename allocator_const_void_pointer<A>::type h) {
  return a.allocate(n, h);
}

template <class A>
inline typename std::enable_if<!detail::alloc_has_allocate<A>::value,
                               typename allocator_pointer<A>::type>::type
allocator_allocate(A &a,
                   typename allocator_size_type<A>::type n,
                   typename allocator_const_void_pointer<A>::type) {
  return a.allocate(n);
}

namespace detail {

template <class A, class T, class... Args> class alloc_has_construct {
  template <class O>
  static auto check(int)
      -> alloc_no<decltype(std::declval<O &>().construct(std::declval<T *>(),
                                                         std::declval<Args &&>()...))>;

  template <class> static char check(long);

public:
  static constexpr bool value = sizeof(check<A>(0)) > 1;
};

template <bool, class = void> struct alloc_if {};

template <class T> struct alloc_if<true, T> {
  typedef T type;
};

} // namespace detail

template <class A, class T, class... Args>
inline typename std::enable_if<detail::alloc_has_construct<A, T, Args...>::value>::type
allocator_construct(A &a, T *p, Args &&...args) {
  a.construct(p, std::forward<Args>(args)...);
}

template <class A, class T, class... Args>
inline typename std::enable_if<!detail::alloc_has_construct<A, T, Args...>::value>::type
allocator_construct(A &, T *p, Args &&...args) {
  ::new ((void *)p) T(std::forward<Args>(args)...);
}

namespace detail {
template <class A, class T> class alloc_has_destroy {
  template <class O>
  static auto check(int)
      -> alloc_no<decltype(std::declval<O &>().destroy(std::declval<T *>()))>;

  template <class> static char check(long);

public:
  static constexpr bool value = sizeof(check<A>(0)) > 1;
};
} // namespace detail

template <class A, class T>
inline typename detail::alloc_if<detail::alloc_has_destroy<A, T>::value>::type
allocator_destroy(A &a, T *p) {
  a.destroy(p);
}

template <class A, class T>
inline typename detail::alloc_if<!detail::alloc_has_destroy<A, T>::value>::type
allocator_destroy(A &, T *p) {
  p->~T();
  (void)p;
}

namespace detail {

template <class A> class alloc_has_max_size {
  template <class O>
  static auto check(int) -> alloc_no<decltype(std::declval<const O &>().max_size())>;

  template <class> static char check(long);

public:
  static constexpr bool value = sizeof(check<A>(0)) > 1;
};

} // namespace detail

template <class A>
inline typename detail::alloc_if<detail::alloc_has_max_size<A>::value,
                                 typename allocator_size_type<A>::type>::type
allocator_max_size(const A &a) noexcept {
  return a.max_size();
}

template <class A>
inline typename detail::alloc_if<!detail::alloc_has_max_size<A>::value,
                                 typename allocator_size_type<A>::type>::type
allocator_max_size(const A &) noexcept {
  return (std::numeric_limits<typename allocator_size_type<A>::type>::max)() /
         sizeof(typename allocator_value_type<A>::type);
}

namespace detail {

template <class A> class alloc_has_soccc {
  template <class O>
  static auto check(int)
      -> alloc_no<decltype(std::declval<const O &>().select_on_container_copy_construction())>;

  template <class> static char check(long);

public:
  static constexpr bool value = sizeof(check<A>(0)) > 1;
};

} // namespace detail

template <class A>
inline typename detail::alloc_if<detail::alloc_has_soccc<A>::value, A>::type
allocator_select_on_container_copy_construction(const A &a) {
  return a.select_on_container_copy_construction();
}

template <class A>
inline typename detail::alloc_if<!detail::alloc_has_soccc<A>::value, A>::type
allocator_select_on_container_copy_construction(const A &a) {
  return a;
}

template <class A, class T> inline void allocator_destroy_n(A &a, T *p, std::size_t n) {
  while (n > 0) { boost::allocator_destroy(a, p + --n); }
}

namespace detail {

template <class A, class T> class alloc_destroyer {
public:
  alloc_destroyer(A &a, T *p) noexcept : a_(a), p_(p), n_(0) {}

  ~alloc_destroyer() { boost::allocator_destroy_n(a_, p_, n_); }

  std::size_t &size() noexcept { return n_; }

private:
  alloc_destroyer(const alloc_destroyer &);

  alloc_destroyer &operator=(const alloc_destroyer &);

  A &a_;
  T *p_;
  std::size_t n_;
};

} // namespace detail

template <class A, class T> inline void allocator_construct_n(A &a, T *p, std::size_t n) {
  detail::alloc_destroyer<A, T> d(a, p);
  for (std::size_t &i = d.size(); i < n; ++i) { boost::allocator_construct(a, p + i); }
  d.size() = 0;
}

template <class A, class T>
inline void allocator_construct_n(A &a, T *p, std::size_t n, const T *l, std::size_t m) {
  detail::alloc_destroyer<A, T> d(a, p);
  for (std::size_t &i = d.size(); i < n; ++i) boost::allocator_construct(a, p + i, l[i % m]);
  d.size() = 0;
}

template <class A, class T, class I>
inline void allocator_construct_n(A &a, T *p, std::size_t n, I b) {
  detail::alloc_destroyer<A, T> d(a, p);
  for (std::size_t &i = d.size(); i < n; void(++i), void(++b))
    boost::allocator_construct(a, p + i, *b);
  d.size() = 0;
}

template <class A> using allocator_value_type_t = typename allocator_value_type<A>::type;

template <class A> using allocator_pointer_t = typename allocator_pointer<A>::type;

template <class A> using allocator_const_pointer_t = typename allocator_const_pointer<A>::type;

template <class A> using allocator_void_pointer_t = typename allocator_void_pointer<A>::type;

template <class A>
using allocator_const_void_pointer_t = typename allocator_const_void_pointer<A>::type;

template <class A>
using allocator_difference_type_t = typename allocator_difference_type<A>::type;

template <class A> using allocator_size_type_t = typename allocator_size_type<A>::type;

template <class A>
using allocator_propagate_on_container_copy_assignment_t =
    typename allocator_propagate_on_container_copy_assignment<A>::type;

template <class A>
using allocator_propagate_on_container_move_assignment_t =
    typename allocator_propagate_on_container_move_assignment<A>::type;

template <class A>
using allocator_propagate_on_container_swap_t =
    typename allocator_propagate_on_container_swap<A>::type;

template <class A>
using allocator_is_always_equal_t = typename allocator_is_always_equal<A>::type;

template <class A, class T> using allocator_rebind_t = typename allocator_rebind<A, T>::type;
} // namespace boost

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

/* @file_end boost/core/allocator_access.hpp */

/* @file_start boost/core/noinit_adaptor.hpp */

namespace boost {

template <class A> struct noinit_adaptor : A {
  typedef void _default_construct_destroy;

  template <class U> struct rebind {
    typedef noinit_adaptor<typename allocator_rebind<A, U>::type> other;
  };

  noinit_adaptor() : A() {}

  template <class U> noinit_adaptor(U &&u) noexcept : A(std::forward<U>(u)) {}

  template <class U>
  noinit_adaptor(const noinit_adaptor<U> &u) noexcept : A(static_cast<const A &>(u)) {}

  template <class U> void construct(U *p) { ::new ((void *)p) U; }

  template <class U> void destroy(U *p) {
    p->~U();
    (void)p;
  }
};

template <class T, class U>
inline bool operator==(const noinit_adaptor<T> &lhs, const noinit_adaptor<U> &rhs) noexcept {
  return static_cast<const T &>(lhs) == static_cast<const U &>(rhs);
}

template <class T, class U>
inline bool operator!=(const noinit_adaptor<T> &lhs, const noinit_adaptor<U> &rhs) noexcept {
  return !(lhs == rhs);
}

template <class A> inline noinit_adaptor<A> noinit_adapt(const A &a) noexcept {
  return noinit_adaptor<A>(a);
}

} // namespace boost

/* @file_end boost/core/noinit_adaptor.hpp */

/* @file_start boost/core/checked_delete.hpp */

namespace boost {

// verify that types are complete for increased safety

template <class T> inline void checked_delete(T *x) noexcept {
  static_assert(sizeof(T) != 0, "Type must be complete");
  delete x;
}

template <class T> inline void checked_array_delete(T *x) noexcept {
  static_assert(sizeof(T) != 0, "Type must be complete");
  delete[] x;
}

template <class T> struct checked_deleter {
  typedef void result_type;
  typedef T *argument_type;

  void operator()(T *x) const noexcept {
    // boost:: disables ADL
    boost::checked_delete(x);
  }
};

template <class T> struct checked_array_deleter {
  typedef void result_type;
  typedef T *argument_type;

  void operator()(T *x) const noexcept { boost::checked_array_delete(x); }
};

} // namespace boost

/* @file_end boost/core/checked_delete.hpp */

/* @file_start boost/core/detail/sp_win32_sleep.hpp */

// Declares the Win32 Sleep() function without including "Windows.h".
namespace boost {
namespace core {
namespace detail {

#if defined(__clang__) && defined(__x86_64__)
// clang x64 warns that __stdcall is ignored
#  define SP_STDCALL
#else
#  define SP_STDCALL __stdcall
#endif

#if defined(__LP64__) // Cygwin 64
extern "C" __declspec(dllimport) void SP_STDCALL Sleep(unsigned int ms);
#else
extern "C" __declspec(dllimport) void SP_STDCALL Sleep(unsigned long ms);
#endif

extern "C" __declspec(dllimport) int SP_STDCALL SwitchToThread();

#undef SP_STDCALL

} // namespace detail
} // namespace core
} // namespace boost

/* @file_end boost/core/detail/sp_win32_sleep.hpp */
/* @file_start boost/core/detail/sp_thread_pause.hpp */

#ifdef _MSC_VER

#  include <intrin.h>

#endif // _MSC_VER

namespace boost {
namespace core {

//   Emits a "pause" instruction.
FORCEINLINE void sp_thread_pause() noexcept {
#ifdef __has_builtin
#  if __has_builtin(__buildtin_ia32_pause) && !defined(__INTEL_COMPILER)
#    define HAS_IA32_PAUSE
#  endif
#endif // __has_built_in

#if defined(HAS_IA32_PAUSE)
  __builtin_ia32_pause();
#  undef HAS_IA32_PAUSE
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  _mm_pause();
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
  __yield();
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
  __asm__ __volatile__("rep; nop" : : : "memory");
#elif defined(__GNUC__) && ((defined(__ARM_ARCH) && __ARM_ARCH >= 8) ||                       \
                            defined(__ARM_ARCH_8A__) || defined(__aarch64__))
  __asm__ __volatile__("yield" : : : "memory");
#else
  ((void)0);
#endif
}

} // namespace core
} // namespace boost

/* @file_end boost/core/detail/sp_thread_pause.hpp */

/* @file_start boost/core/detail/sp_thread_sleep.hpp */

#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)

namespace boost {
namespace core {
namespace detail {

inline void sp_thread_sleep() noexcept { Sleep(1); }

} // namespace detail

using boost::core::detail::sp_thread_sleep;

} // namespace core
} // namespace boost

#elif defined(HAS_NANOSLEEP)

#  include <time.h>

#  if defined(HAS_PTHREADS) && !defined(__ANDROID__)
#    include <pthread.h>
#  endif

namespace boost {
namespace core {

inline void sp_thread_sleep() noexcept {
#  if defined(HAS_PTHREADS) && !defined(__ANDROID__)
  int oldst;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldst);
#  endif

  // g++ -Wextra warns on {} or {0}
  struct timespec rqtp = {0, 0};

  // POSIX says that timespec has tv_sec and tv_nsec
  // But it doesn't guarantee order or placement

  rqtp.tv_sec = 0;
  rqtp.tv_nsec = 1000;

  nanosleep(&rqtp, 0);

#  if defined(HAS_PTHREADS) && !defined(__ANDROID__)
  pthread_setcancelstate(oldst, &oldst);
#  endif
}

} // namespace core
} // namespace boost

#else

/* @file_start boost/core/detail/sp_thread_yield.hpp */

#  if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)

namespace boost {
namespace core {
namespace detail {

inline void sp_thread_yield() noexcept { SwitchToThread(); }

} // namespace detail

using boost::core::detail::sp_thread_yield;

} // namespace core
} // namespace boost

#  elif defined(HAS_SCHED_YIELD)

#    ifndef _AIX
#      include <sched.h>
#    else
// AIX's sched.h defines ::var which sometimes conflicts with Lambda's var
extern "C" int sched_yield(void);
#    endif

namespace boost {
namespace core {

inline void sp_thread_yield() noexcept { sched_yield(); }

} // namespace core
} // namespace boost

#  else

namespace boost {
namespace core {

inline void sp_thread_yield() noexcept { sp_thread_pause(); }

} // namespace core
} // namespace boost

#  endif

/* @file_end boost/core/detail/sp_thread_yield.hpp */

namespace boost {
namespace core {

inline void sp_thread_sleep() noexcept { sp_thread_yield(); }

} // namespace core
} // namespace boost

#endif

/* @file_end boost/core/detail/sp_thread_sleep.hpp */

/* @file_start smart_ptr/detail/sp_forward.hpp */

//  detail/sp_forward.hpp
//
//  Copyright 2008,2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

namespace boost {

namespace detail {

template <class T> T &&sp_forward(T &t) noexcept { return static_cast<T &&>(t); }

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/sp_forward.hpp */

/* @file_start smart_ptr/detail/lightweight_mutex.hpp */

//  typedef <unspecified> boost::detail::lightweight_mutex;
//
//  boost::detail::lightweight_mutex is a header-only implementation of
//  a subset of the Mutex concept requirements:
//
//  http://www.boost.org/doc/html/threads/concepts.html#threads.concepts.Mutex
//
//  It maps to a CRITICAL_SECTION on Windows or a pthread_mutex on POSIX.

#if !defined(BOOST_NO_CXX11_HDR_MUTEX)

/* @file_start smart_ptr/detail/lwm_std_mutex.hpp */

#  include <mutex>

namespace boost {

namespace detail {

class lightweight_mutex {
private:
  std::mutex m_;

  lightweight_mutex(lightweight_mutex const &);

  lightweight_mutex &operator=(lightweight_mutex const &);

public:
  lightweight_mutex() {}

  class scoped_lock;

  friend class scoped_lock;

  class scoped_lock {
  private:
    std::mutex &m_;

    scoped_lock(scoped_lock const &);

    scoped_lock &operator=(scoped_lock const &);

  public:
    scoped_lock(lightweight_mutex &m) : m_(m.m_) { m_.lock(); }

    ~scoped_lock() { m_.unlock(); }
  };
};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/lwm_std_mutex.hpp */

#elif defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
/* @file_start smart_ptr/detail/lwm_win32_cs.hpp */

struct _RTL_CRITICAL_SECTION;

namespace boost {
namespace detail {

struct critical_section {
  struct critical_section_debug *DebugInfo;
  long LockCount;
  long RecursionCount;
  void *OwningThread;
  void *LockSemaphore;
#  if defined(_WIN64)
  unsigned __int64 SpinCount;
#  else
  unsigned long SpinCount;
#  endif
};

extern "C" __declspec(dllimport) void __stdcall InitializeCriticalSection(
    ::_RTL_CRITICAL_SECTION *);
extern "C" __declspec(dllimport) void __stdcall EnterCriticalSection(
    ::_RTL_CRITICAL_SECTION *);
extern "C" __declspec(dllimport) void __stdcall LeaveCriticalSection(
    ::_RTL_CRITICAL_SECTION *);
extern "C" __declspec(dllimport) void __stdcall DeleteCriticalSection(
    ::_RTL_CRITICAL_SECTION *);

typedef ::_RTL_CRITICAL_SECTION rtl_critical_section;

class lightweight_mutex {
private:
  critical_section cs_;

  lightweight_mutex(lightweight_mutex const &);
  lightweight_mutex &operator=(lightweight_mutex const &);

public:
  lightweight_mutex() {
    boost::detail::InitializeCriticalSection(reinterpret_cast<rtl_critical_section *>(&cs_));
  }

  ~lightweight_mutex() {
    boost::detail::DeleteCriticalSection(reinterpret_cast<rtl_critical_section *>(&cs_));
  }

  class scoped_lock;
  friend class scoped_lock;

  class scoped_lock {
  private:
    lightweight_mutex &m_;

    scoped_lock(scoped_lock const &);
    scoped_lock &operator=(scoped_lock const &);

  public:
    explicit scoped_lock(lightweight_mutex &m) : m_(m) {
      boost::detail::EnterCriticalSection(reinterpret_cast<rtl_critical_section *>(&m_.cs_));
    }

    ~scoped_lock() {
      boost::detail::LeaveCriticalSection(reinterpret_cast<rtl_critical_section *>(&m_.cs_));
    }
  };
};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/lwm_win32_cs.hpp */
#else
/* @file_start  smart_ptr/detail/lwm_pthreads.hpp */

#  include <pthread.h>

namespace boost {

namespace detail {

class lightweight_mutex {
private:
  pthread_mutex_t m_;

  lightweight_mutex(lightweight_mutex const &);
  lightweight_mutex &operator=(lightweight_mutex const &);

public:
  lightweight_mutex() {
    // HPUX 10.20 / DCE has a nonstandard pthread_mutex_init

#  if defined(__hpux) && defined(_DECTHREADS_)
    assert(pthread_mutex_init(&m_, pthread_mutexattr_default) == 0);
#  else
    assert(pthread_mutex_init(&m_, 0) == 0);
#  endif
  }

  ~lightweight_mutex() { assert(pthread_mutex_destroy(&m_) == 0); }

  class scoped_lock;
  friend class scoped_lock;

  class scoped_lock {
  private:
    pthread_mutex_t &m_;

    scoped_lock(scoped_lock const &);
    scoped_lock &operator=(scoped_lock const &);

  public:
    scoped_lock(lightweight_mutex &m) : m_(m.m_) { assert(pthread_mutex_lock(&m_) == 0); }

    ~scoped_lock() { assert(pthread_mutex_unlock(&m_) == 0); }
  };
};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/lwm_pthreads.hpp */
#endif

/* @file_end smart_ptr/detail/lightweight_mutex.hpp */

/* @file_start smart_ptr/detail/lightweight_thread.hpp */

#include <cerrno>
#include <memory>

#if defined(HAS_PTHREADS)

#  include <pthread.h>

namespace boost {
namespace detail {

typedef ::pthread_t lw_thread_t;

inline int lw_thread_create_(lw_thread_t *thread,
                             const pthread_attr_t *attr,
                             void *(*start_routine)(void *),
                             void *arg) {
  return ::pthread_create(thread, attr, start_routine, arg);
}

inline void lw_thread_join(lw_thread_t th) { ::pthread_join(th, 0); }

} // namespace detail
} // namespace boost

#else

#  include <process.h>
#  include <windows.h>

namespace boost {
namespace detail {

typedef HANDLE lw_thread_t;

inline int lw_thread_create_(lw_thread_t *thread,
                             void const *,
                             unsigned(__stdcall *start_routine)(void *),
                             void *arg) {
  HANDLE h = (HANDLE)_beginthreadex(0, 0, start_routine, arg, 0, 0);

  if (h != 0) {
    *thread = h;
    return 0;
  } else {
    return EAGAIN;
  }
}

inline void lw_thread_join(lw_thread_t thread) {
  ::WaitForSingleObject(thread, INFINITE);
  ::CloseHandle(thread);
}

} // namespace detail
} // namespace boost

#endif

namespace boost {
namespace detail {

class lw_abstract_thread {
public:
  virtual ~lw_abstract_thread() {}

  virtual void run() = 0;
};

#if defined(HAS_PTHREADS)

extern "C" void *lw_thread_routine(void *pv) {
  std::unique_ptr<lw_abstract_thread> pt(static_cast<lw_abstract_thread *>(pv));
  pt->run();
  return 0;
}

#else

unsigned __stdcall lw_thread_routine(void *pv) {
  std::unique_ptr<lw_abstract_thread> pt(static_cast<lw_abstract_thread *>(pv));
  pt->run();
  return 0;
}

#endif

template <class F> class lw_thread_impl : public lw_abstract_thread {
public:
  explicit lw_thread_impl(F f) : f_(f) {}

  void run() { f_(); }

private:
  F f_;
};

template <class F> int lw_thread_create(lw_thread_t &th, F f) {
  std::unique_ptr<lw_abstract_thread> p(new lw_thread_impl<F>(f));

  int r = lw_thread_create_(&th, 0, lw_thread_routine, p.get());

  if (r == 0) { p.release(); }

  return r;
}

} // namespace detail
} // namespace boost

/* @file_end smart_ptr/detail/lightweight_thread.hpp */

/* @file_start boost/type_traits/integral_constant.hpp */

namespace mpl_ {

template <bool B> struct bool_;
template <class I, I val> struct integral_c;
struct integral_c_tag;
} // namespace mpl_

namespace boost {
namespace mpl {
using ::mpl_::bool_;
using ::mpl_::integral_c;
using ::mpl_::integral_c_tag;
} // namespace mpl
} // namespace boost

namespace boost {

template <class T, T val> struct integral_constant {
  typedef mpl::integral_c_tag tag;
  typedef T value_type;
  typedef integral_constant<T, val> type;
  static const T value = val;

  operator const mpl::integral_c<T, val> &() const {
    static const char data[sizeof(long)] = {0};
    static const void *pdata = data;
    return *(reinterpret_cast<const mpl::integral_c<T, val> *>(pdata));
  }

  constexpr operator T() const { return val; }
};

template <class T, T val> T const integral_constant<T, val>::value;

template <bool val> struct integral_constant<bool, val> {
  typedef mpl::integral_c_tag tag;
  typedef bool value_type;
  typedef integral_constant<bool, val> type;
  static const bool value = val;

  operator const mpl::bool_<val> &() const {
    static const char data[sizeof(long)] = {0};
    static const void *pdata = data;
    return *(reinterpret_cast<const mpl::bool_<val> *>(pdata));
  }

  constexpr operator bool() const { return val; }
};

template <bool val> bool const integral_constant<bool, val>::value;

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

} // namespace boost

/* @file_end boost/type_traits/integral_constants.hpp */

namespace boost {

template <class T> struct is_bounded_array : false_type {};

template <class T, std::size_t N> struct is_bounded_array<T[N]> : true_type {};

template <class T, std::size_t N> struct is_bounded_array<const T[N]> : true_type {};

template <class T, std::size_t N> struct is_bounded_array<volatile T[N]> : true_type {};

template <class T, std::size_t N> struct is_bounded_array<const volatile T[N]> : true_type {};
} // namespace boost

namespace boost {

template <class T> struct is_unbounded_array : false_type {};

template <class T> struct is_unbounded_array<T[]> : true_type {};

template <class T> struct is_unbounded_array<const T[]> : true_type {};

template <class T> struct is_unbounded_array<volatile T[]> : true_type {};

template <class T> struct is_unbounded_array<const volatile T[]> : true_type {};

} // namespace boost
/* @file_start boost/type_traits/is_pod.hpp */

namespace boost {

// forward declaration, needed by 'is_pod_array_helper' template below
template <typename T> struct is_POD;

template <typename T>
struct is_pod
    : public integral_constant<bool,
                               ::boost::is_scalar<T>::value || ::boost::is_void<T>::value ||
                                   std::is_pod<T>::value> {};

template <typename T, std::size_t sz> struct is_pod<T[sz]> : public is_pod<T> {};

// the following help compilers without partial specialization support:
template <> struct is_pod<void> : public true_type {};

template <> struct is_pod<void const> : public true_type {};
template <> struct is_pod<void const volatile> : public true_type {};
template <> struct is_pod<void volatile> : public true_type {};

template <class T> struct is_POD : public is_pod<T> {};

} // namespace boost

/* @file_end boost/type_traits/is_pod.hpp */

/* @file_start boost/type_traits/type_with_alignment.hpp */

#include <cstddef> // size_t

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4121) // alignment is sensitive to packing
#endif

namespace boost {
namespace detail {

union max_align {
  char c;
  short s;
  int i;
  long l;
  long long ll;
#ifndef _MSC_VER
  __int128 i128; // FIXME
#endif
  float f;
  double d;
  long double ld;
#ifndef _MSC_VER
  __float128 f128;
#endif
};

template <std::size_t Target, bool check> struct long_double_alignment {
  typedef long double type;
};
template <std::size_t Target> struct long_double_alignment<Target, false> {
  typedef boost::detail::max_align type;
};

template <std::size_t Target, bool check> struct double_alignment {
  typedef double type;
};
template <std::size_t Target> struct double_alignment<Target, false> {
  typedef
      typename long_double_alignment<Target,
                                     boost::alignment_of<long double>::value >= Target>::type
          type;
};

template <std::size_t Target, bool check> struct long_long_alignment {
  typedef boost::long_long_type type;
};
template <std::size_t Target> struct long_long_alignment<Target, false> {
  typedef typename double_alignment<Target, boost::alignment_of<double>::value >= Target>::type
      type;
};

template <std::size_t Target, bool check> struct long_alignment {
  typedef long type;
};
template <std::size_t Target> struct long_alignment<Target, false> {
  typedef typename long_long_alignment<Target,
                                       boost::alignment_of<boost::long_long_type>::value >=
                                           Target>::type type;
};

template <std::size_t Target, bool check> struct int_alignment {
  typedef int type;
};
template <std::size_t Target> struct int_alignment<Target, false> {
  typedef
      typename long_alignment<Target, boost::alignment_of<long>::value >= Target>::type type;
};

template <std::size_t Target, bool check> struct short_alignment {
  typedef short type;
};
template <std::size_t Target> struct short_alignment<Target, false> {
  typedef typename int_alignment<Target, boost::alignment_of<int>::value >= Target>::type type;
};

template <std::size_t Target, bool check> struct char_alignment {
  typedef char type;
};
template <std::size_t Target> struct char_alignment<Target, false> {
  typedef
      typename short_alignment<Target, boost::alignment_of<short>::value >= Target>::type type;
};

} // namespace detail

template <std::size_t Align> struct type_with_alignment {
  typedef
      typename boost::detail::char_alignment<Align,
                                             boost::alignment_of<char>::value >= Align>::type
          type;
};

#if (defined(__GNUC__) || (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x5130)) ||                \
     defined(__clang__))
namespace tt_align_ns {
struct __attribute__((__aligned__(2))) a2 {};
struct __attribute__((__aligned__(4))) a4 {};
struct __attribute__((__aligned__(8))) a8 {};
struct __attribute__((__aligned__(16))) a16 {};
struct __attribute__((__aligned__(32))) a32 {};
struct __attribute__((__aligned__(64))) a64 {};
struct __attribute__((__aligned__(128))) a128 {};
} // namespace tt_align_ns

template <> struct type_with_alignment<1> {
public:
  typedef char type;
};
template <> struct type_with_alignment<2> {
public:
  typedef tt_align_ns::a2 type;
};
template <> struct type_with_alignment<4> {
public:
  typedef tt_align_ns::a4 type;
};
template <> struct type_with_alignment<8> {
public:
  typedef tt_align_ns::a8 type;
};
template <> struct type_with_alignment<16> {
public:
  typedef tt_align_ns::a16 type;
};
template <> struct type_with_alignment<32> {
public:
  typedef tt_align_ns::a32 type;
};
template <> struct type_with_alignment<64> {
public:
  typedef tt_align_ns::a64 type;
};
template <> struct type_with_alignment<128> {
public:
  typedef tt_align_ns::a128 type;
};

template <> struct is_pod<::boost::tt_align_ns::a2> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a4> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a8> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a16> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a32> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a64> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a128> : public true_type {};

#endif
#if defined(_MSC_VER)
//
// MSVC supports types which have alignments greater than the normal
// maximum: these are used for example in the types __m64 and __m128
// to provide types with alignment requirements which match the SSE
// registers.  Therefore we extend type_with_alignment<> to support
// such types, however, we have to be careful to use a builtin type
// whenever possible otherwise we break previously working code:
// see https://lists.boost.org/Archives/boost/2014/03/212391.php
// for an example and test case.  Thus types like a8 below will
// be used *only* if the existing implementation can't provide a type
// with suitable alignment.  This does mean however, that type_with_alignment<>
// may return a type which cannot be passed through a function call
// by value (and neither can any type containing such a type like
// Boost.Optional).  However, this only happens when we have no choice
// in the matter because no other "ordinary" type is available.
//
namespace tt_align_ns {
struct __declspec(align(8)) a8 {
  char m[8];
  typedef a8 type;
};
struct __declspec(align(16)) a16 {
  char m[16];
  typedef a16 type;
};
struct __declspec(align(32)) a32 {
  char m[32];
  typedef a32 type;
};
struct __declspec(align(64)) a64 {
  char m[64];
  typedef a64 type;
};
struct __declspec(align(128)) a128 {
  char m[128];
  typedef a128 type;
};
} // namespace tt_align_ns

template <> struct type_with_alignment<8> {
  typedef boost::conditional <
      ::boost::alignment_of<boost::detail::max_align>::
          value<8, tt_align_ns::a8, boost::detail::char_alignment<8, false>>::type t1;

public:
  typedef t1::type type;
};
template <> struct type_with_alignment<16> {
  typedef boost::conditional <
      ::boost::alignment_of<boost::detail::max_align>::
          value<16, tt_align_ns::a16, boost::detail::char_alignment<16, false>>::type t1;

public:
  typedef t1::type type;
};
template <> struct type_with_alignment<32> {
  typedef boost::conditional <
      ::boost::alignment_of<boost::detail::max_align>::
          value<32, tt_align_ns::a32, boost::detail::char_alignment<32, false>>::type t1;

public:
  typedef t1::type type;
};
template <> struct type_with_alignment<64> {
  typedef boost::conditional <
      ::boost::alignment_of<boost::detail::max_align>::
          value<64, tt_align_ns::a64, boost::detail::char_alignment<64, false>>::type t1;

public:
  typedef t1::type type;
};
template <> struct type_with_alignment<128> {
  typedef boost::conditional <
      ::boost::alignment_of<boost::detail::max_align>::
          value<128, tt_align_ns::a128, boost::detail::char_alignment<128, false>>::type t1;

public:
  typedef t1::type type;
};

template <> struct is_pod<::boost::tt_align_ns::a8> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a16> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a32> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a64> : public true_type {};
template <> struct is_pod<::boost::tt_align_ns::a128> : public true_type {};

#endif

} // namespace boost

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

/* @file_end boost/type_traits/type_with_alignment.hpp */

/* @file_start smart_ptr/detail/quick_allocator.hpp */

#include <cstddef> // std::size_t
#include <new>     // ::operator new, ::operator delete

namespace boost {

namespace detail {

template <unsigned size, unsigned align_> union freeblock {
  typedef typename boost::type_with_alignment<align_>::type aligner_type;
  aligner_type aligner;
  char bytes[size];
  freeblock *next;
};

template <unsigned size, unsigned align_> struct allocator_impl {
  typedef freeblock<size, align_> block;

  // It may seem odd to use such small pages.
  //
  // However, on a typical Windows implementation that uses
  // the OS allocator, "normal size" pages interact with the
  // "ordinary" operator new, slowing it down dramatically.
  //
  // 512 byte pages are handled by the small object allocator,
  // and don't interfere with ::new.
  //
  // The other alternative is to use much bigger pages (1M.)
  //
  // It is surprisingly easy to hit pathological behavior by
  // varying the page size. g++ 2.96 on Red Hat Linux 7.2,
  // for example, passionately dislikes 496. 512 seems OK.

  enum { items_per_page = 512 / size }; // 1048560 / size

#ifdef HAS_THREADS

  static lightweight_mutex &mutex() {
    static freeblock<sizeof(lightweight_mutex), boost::alignment_of<lightweight_mutex>::value>
        fbm;
    static lightweight_mutex *pm = new (&fbm) lightweight_mutex;
    return *pm;
  }

  static lightweight_mutex *mutex_init;

#endif

  static block *free;
  static block *page;
  static unsigned last;

  static inline void *alloc() {
#ifdef HAS_THREADS
    lightweight_mutex::scoped_lock lock(mutex());
#endif
    if (block *x = free) {
      free = x->next;
      return x;
    } else {
      if (last == items_per_page) {
        // "Listen to me carefully: there is no memory leak"
        // -- Scott Meyers, Eff C++ 2nd Ed Item 10
        page = ::new block[items_per_page];
        last = 0;
      }

      return &page[last++];
    }
  }

  static inline void *alloc(std::size_t n) {
    if (n != size) // class-specific new called for a derived object
    {
      return ::operator new(n);
    } else {
#ifdef HAS_THREADS
      lightweight_mutex::scoped_lock lock(mutex());
#endif
      if (block *x = free) {
        free = x->next;
        return x;
      } else {
        if (last == items_per_page) {
          page = ::new block[items_per_page];
          last = 0;
        }

        return &page[last++];
      }
    }
  }

  static inline void dealloc(void *pv) {
    if (pv != 0) // 18.4.1.1/13
    {
#ifdef HAS_THREADS
      lightweight_mutex::scoped_lock lock(mutex());
#endif
      block *pb = static_cast<block *>(pv);
      pb->next = free;
      free = pb;
    }
  }

  static inline void dealloc(void *pv, std::size_t n) {
    if (n != size) // class-specific delete called for a derived object
    {
      ::operator delete(pv);
    } else if (pv != 0) // 18.4.1.1/13
    {
#ifdef HAS_THREADS
      lightweight_mutex::scoped_lock lock(mutex());
#endif
      block *pb = static_cast<block *>(pv);
      pb->next = free;
      free = pb;
    }
  }
};

#ifdef HAS_THREADS

template <unsigned size, unsigned align_>
lightweight_mutex *allocator_impl<size, align_>::mutex_init =
    &allocator_impl<size, align_>::mutex();

#endif

template <unsigned size, unsigned align_>
freeblock<size, align_> *allocator_impl<size, align_>::free = 0;

template <unsigned size, unsigned align_>
freeblock<size, align_> *allocator_impl<size, align_>::page = 0;

template <unsigned size, unsigned align_>
unsigned allocator_impl<size, align_>::last = allocator_impl<size, align_>::items_per_page;

template <class T>
struct quick_allocator : public allocator_impl<sizeof(T), boost::alignment_of<T>::value> {};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/quick_allocator.hpp */

/* @file_start smart_ptr/bad_weak_ptr.hpp */

#include <exception>

namespace boost {

// The standard library that comes with Borland C++ 5.5.1, 5.6.4
// defines std::exception and its members as having C calling
// convention (-pc). When the definition of bad_weak_ptr
// is compiled with -ps, the compiler issues an error.
// Hence, the temporary #pragma option -pc below.

#if defined(__clang__)
// Intel C++ on Mac defines __clang__ but doesn't support the pragma
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wweak-vtables"
#endif

class bad_weak_ptr : public std::exception {
public:
  char const *what() const noexcept override { return "tr1::bad_weak_ptr"; }
};

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

} // namespace boost

/* @file_end smart_ptr/bad_weak_ptr.hpp */

/* @file_start smart_ptr/detail/yield_k.hpp */

// inline void boost::detail::yield( unsigned k );
//
//   Typical use:
//   for( unsigned k = 0; !try_lock(); ++k ) yield( k );

namespace boost {

namespace detail {

inline void yield(unsigned k) {
  // Experiments on Windows and Fedora 32 show that a single pause,
  // followed by an immediate sp_thread_sleep(), is best.

  if (k & 1) {
    boost::core::sp_thread_sleep();
  } else {
    boost::core::sp_thread_pause();
  }
}

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/yield_k.hpp */

/* @file_start smart_ptr/detail/spinlock.hpp */

//  struct spinlock
//  {
//      void lock();
//      bool try_lock();
//      void unlock();
//
//      class scoped_lock;
//  };
//
//  #define BOOST_DETAIL_SPINLOCK_INIT <unspecified>

#if defined(DISABLE_THREADS)
/* @file_start boost/smart_ptr/detail/spinlock_nt.hpp */

//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

namespace boost {

namespace detail {

class spinlock {
public:
  bool locked_;

public:
  inline bool try_lock() {
    if (locked_) {
      return false;
    } else {
      locked_ = true;
      return true;
    }
  }

  inline void lock() {
    assert(!locked_);
    locked_ = true;
  }

  inline void unlock() {
    assert(locked_);
    locked_ = false;
  }

public:
  class scoped_lock {
  private:
    spinlock &sp_;

    scoped_lock(scoped_lock const &);
    scoped_lock &operator=(scoped_lock const &);

  public:
    explicit scoped_lock(spinlock &sp) : sp_(sp) { sp.lock(); }

    ~scoped_lock() { sp_.unlock(); }
  };
};

} // namespace detail
} // namespace boost

#  define BOOST_DETAIL_SPINLOCK_INIT                                                          \
    { false }

/* @file_end boost/smart_ptr/detail/spinlock_nt.hpp */

#elif defined(SP_USE_PTHREADS)
/* @file_start boost/smart_ptr/detail/spinlock_pt.hpp */

#  include <pthread.h>

namespace boost {

namespace detail {

class spinlock {
public:
  pthread_mutex_t v_;

public:
  bool try_lock() { return pthread_mutex_trylock(&v_) == 0; }

  void lock() { pthread_mutex_lock(&v_); }

  void unlock() { pthread_mutex_unlock(&v_); }

public:
  class scoped_lock {
  private:
    spinlock &sp_;

    scoped_lock(scoped_lock const &);
    scoped_lock &operator=(scoped_lock const &);

  public:
    explicit scoped_lock(spinlock &sp) : sp_(sp) { sp.lock(); }

    ~scoped_lock() { sp_.unlock(); }
  };
};

} // namespace detail
} // namespace boost

#  define BOOST_DETAIL_SPINLOCK_INIT                                                          \
    { PTHREAD_MUTEX_INITIALIZER }

  /* @file_end boost/smart_ptr/detail/spinlock_pt.hpp */

#else
/* @file_start boost/smart_ptr/detail/spinlock_std_atomic.hpp */

#  include <atomic>

namespace boost {

namespace detail {

class spinlock {
public:
  std::atomic_flag v_;

public:
  bool try_lock() noexcept { return !v_.test_and_set(std::memory_order_acquire); }

  void lock() noexcept {
    for (unsigned k = 0; !try_lock(); ++k) { boost::detail::yield(k); }
  }

  void unlock() noexcept { v_.clear(std::memory_order_release); }

public:
  class scoped_lock {
  private:
    spinlock &sp_;

    scoped_lock(scoped_lock const &);

    scoped_lock &operator=(scoped_lock const &);

  public:
    explicit scoped_lock(spinlock &sp) noexcept : sp_(sp) { sp.lock(); }

    ~scoped_lock() /*noexcept*/ { sp_.unlock(); }
  };
};

} // namespace detail
} // namespace boost

#  define BOOST_DETAIL_SPINLOCK_INIT                                                          \
    { ATOMIC_FLAG_INIT }

/* @file_end boost/smart_ptr/detail/spinlock_std_atomic.hpp */
#endif

/* @file_end smart_ptr/detail/spinlock.hpp */

/* @file_start smart_ptr/detail/spinlock_pool.hpp */

//  spinlock_pool<0> is reserved for atomic<>, when/if it arrives
//  spinlock_pool<1> is reserved for shared_ptr reference counts
//  spinlock_pool<2> is reserved for shared_ptr atomic access

#include <cstddef>

namespace boost {

namespace detail {

template <int M> class spinlock_pool {
private:
  static spinlock pool_[41];

public:
  static spinlock &spinlock_for(void const *pv) {
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
    std::size_t i = reinterpret_cast<unsigned long long>(pv) % 41;
#else
    std::size_t i = reinterpret_cast<std::size_t>(pv) % 41;
#endif
    return pool_[i];
  }

  class scoped_lock {
  private:
    spinlock &sp_;

    scoped_lock(scoped_lock const &);

    scoped_lock &operator=(scoped_lock const &);

  public:
    explicit scoped_lock(void const *pv) : sp_(spinlock_for(pv)) { sp_.lock(); }

    ~scoped_lock() { sp_.unlock(); }
  };
};

template <int M>
spinlock spinlock_pool<M>::pool_[41] = {
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT,
    BOOST_DETAIL_SPINLOCK_INIT, BOOST_DETAIL_SPINLOCK_INIT};

} // namespace detail
} // namespace boost

/* @file_end smart_ptr/detail/spinlock_pool.hpp */

/* @file_start smart_ptr/detail/sp_counted_base.hpp */

#if defined(SP_DISABLE_THREADS)

/* @file_start smart_ptr/detail/sp_counted_base_nt.hpp */

namespace boost {

namespace detail {

class SYMBOL_VISIBLE sp_counted_base {
private:
  sp_counted_base(sp_counted_base const &);
  sp_counted_base &operator=(sp_counted_base const &);

  boost::int_least32_t use_count_;  // #shared
  boost::int_least32_t weak_count_; // #weak + (#shared != 0)

public:
  sp_counted_base() noexcept : use_count_(1), weak_count_(1) {}

  virtual ~sp_counted_base() /*noexcept*/ {}

  // dispose() is called when use_count_ drops to zero, to release
  // the resources managed by *this.

  virtual void dispose() noexcept = 0; // nothrow

  // destroy() is called when weak_count_ drops to zero.

  virtual void destroy() noexcept // nothrow
  {
    delete this;
  }

  virtual void *get_deleter(std::type_info const &ti) noexcept = 0;
  virtual void *get_local_deleter(std::type_info const &ti) noexcept = 0;
  virtual void *get_untyped_deleter() noexcept = 0;

  void add_ref_copy() noexcept { ++use_count_; }

  bool add_ref_lock() noexcept // true on success
  {
    if (use_count_ == 0) return false;
    ++use_count_;
    return true;
  }

  void release() noexcept {
    if (--use_count_ == 0) {
      dispose();
      weak_release();
    }
  }

  void weak_add_ref() noexcept { ++weak_count_; }

  void weak_release() noexcept {
    if (--weak_count_ == 0) { destroy(); }
  }

  long use_count() const noexcept { return use_count_; }
};

} // namespace detail

} // namespace boost
/* @file_end smart_ptr/detail/sp_counted_base_nt.hpp */

#elif defined(SP_USE_PTHREADS)
/* @file_start smart_ptr/detail/sp_counted_base_pt.hpp */

#  include <pthread.h>

namespace boost {

namespace detail {

class SYMBOL_VISIBLE sp_counted_base {
private:
  sp_counted_base(sp_counted_base const &);
  sp_counted_base &operator=(sp_counted_base const &);

  boost::int_least32_t use_count_;  // #shared
  boost::int_least32_t weak_count_; // #weak + (#shared != 0)

  mutable pthread_mutex_t m_;

public:
  sp_counted_base() : use_count_(1), weak_count_(1) {
    // HPUX 10.20 / DCE has a nonstandard pthread_mutex_init

#  if defined(__hpux) && defined(_DECTHREADS_)
    assert(pthread_mutex_init(&m_, pthread_mutexattr_default) == 0);
#  else
    assert(pthread_mutex_init(&m_, 0) == 0);
#  endif
  }

  virtual ~sp_counted_base() // nothrow
  {
    assert(pthread_mutex_destroy(&m_) == 0);
  }

  // dispose() is called when use_count_ drops to zero, to release
  // the resources managed by *this.

  virtual void dispose() = 0; // nothrow

  // destroy() is called when weak_count_ drops to zero.

  virtual void destroy() // nothrow
  {
    delete this;
  }

  virtual void *get_deleter(std::type_info const &ti) = 0;
  virtual void *get_local_deleter(std::type_info const &ti) = 0;
  virtual void *get_untyped_deleter() = 0;

  void add_ref_copy() {
    assert(pthread_mutex_lock(&m_) == 0);
    ++use_count_;
    assert(pthread_mutex_unlock(&m_) == 0);
  }

  bool add_ref_lock() // true on success
  {
    assert(pthread_mutex_lock(&m_) == 0);
    bool r = use_count_ == 0 ? false : (++use_count_, true);
    assert(pthread_mutex_unlock(&m_) == 0);
    return r;
  }

  void release() // nothrow
  {
    assert(pthread_mutex_lock(&m_) == 0);
    boost::int_least32_t new_use_count = --use_count_;
    assert(pthread_mutex_unlock(&m_) == 0);

    if (new_use_count == 0) {
      dispose();
      weak_release();
    }
  }

  void weak_add_ref() // nothrow
  {
    assert(pthread_mutex_lock(&m_) == 0);
    ++weak_count_;
    assert(pthread_mutex_unlock(&m_) == 0);
  }

  void weak_release() // nothrow
  {
    assert(pthread_mutex_lock(&m_) == 0);
    boost::int_least32_t new_weak_count = --weak_count_;
    assert(pthread_mutex_unlock(&m_) == 0);

    if (new_weak_count == 0) { destroy(); }
  }

  long use_count() const // nothrow
  {
    assert(pthread_mutex_lock(&m_) == 0);
    boost::int_least32_t r = use_count_;
    assert(pthread_mutex_unlock(&m_) == 0);

    return r;
  }
};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/sp_counted_base_pt.hpp */

#elif defined(SP_USE_STD_ATOMIC)

/* @file_start smart_ptr/detail/sp_counted_base_std_atomic.hpp */

#  include <atomic>
#  include <cstdint>

namespace boost {

namespace detail {

inline void atomic_increment(std::atomic_int_least32_t *pw) noexcept {
  pw->fetch_add(1, std::memory_order_relaxed);
}

inline std::int_least32_t atomic_decrement(std::atomic_int_least32_t *pw) noexcept {
  return pw->fetch_sub(1, std::memory_order_acq_rel);
}

inline std::int_least32_t
atomic_conditional_increment(std::atomic_int_least32_t *pw) noexcept {
  // long r = *pw;
  // if( r != 0 ) ++*pw;
  // return r;

  std::int_least32_t r = pw->load(std::memory_order_relaxed);

  for (;;) {
    if (r == 0) { return r; }

    if (pw->compare_exchange_weak(
            r, r + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
      return r;
    }
  }
}

class SYMBOL_VISIBLE sp_counted_base {
private:
  sp_counted_base(sp_counted_base const &);

  sp_counted_base &operator=(sp_counted_base const &);

  std::atomic_int_least32_t use_count_;  // #shared
  std::atomic_int_least32_t weak_count_; // #weak + (#shared != 0)

public:
  sp_counted_base() noexcept : use_count_(1), weak_count_(1) {}

  virtual ~sp_counted_base() /*noexcept*/ {}

  // dispose() is called when use_count_ drops to zero, to release
  // the resources managed by *this.

  virtual void dispose() noexcept = 0;

  // destroy() is called when weak_count_ drops to zero.

  virtual void destroy() noexcept { delete this; }

  virtual void *get_deleter(std::type_info const &ti) noexcept = 0;

  virtual void *get_local_deleter(std::type_info const &ti) noexcept = 0;

  virtual void *get_untyped_deleter() noexcept = 0;

  void add_ref_copy() noexcept { atomic_increment(&use_count_); }

  bool add_ref_lock() noexcept // true on success
  {
    return atomic_conditional_increment(&use_count_) != 0;
  }

  void release() noexcept {
    if (atomic_decrement(&use_count_) == 1) {
      dispose();
      weak_release();
    }
  }

  void weak_add_ref() noexcept { atomic_increment(&weak_count_); }

  void weak_release() noexcept {
    if (atomic_decrement(&weak_count_) == 1) { destroy(); }
  }

  long use_count() const noexcept { return use_count_.load(std::memory_order_acquire); }
};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/sp_counted_base_std_atomic.hpp */

#endif

/* @file_end smart_ptr/detail/sp_counted_base.hpp */

/* @file_start smart_ptr/detail/sp_counted_impl.hpp */

#if defined(SP_USE_STD_ALLOCATOR) && defined(SP_USE_QUICK_ALLOCATOR)
#  error SP_USE_STD_ALLOCATOR and SP_USE_QUICK_ALLOCATOR are incompatible.
#endif

#include <cstddef> // std::size_t
#include <memory>  // std::allocator, std::allocator_traits

namespace boost {

namespace detail {

// get_local_deleter

template <class D> class local_sp_deleter;

template <class D> D *get_local_deleter(D * /*p*/) noexcept { return 0; }

template <class D> D *get_local_deleter(local_sp_deleter<D> *p) noexcept;

//

template <class X> class SYMBOL_VISIBLE sp_counted_impl_p : public sp_counted_base {
private:
  X *px_;

  sp_counted_impl_p(sp_counted_impl_p const &);

  sp_counted_impl_p &operator=(sp_counted_impl_p const &);

  typedef sp_counted_impl_p<X> this_type;

public:
  explicit sp_counted_impl_p(X *px) : px_(px) {}

  void dispose() noexcept override { boost::checked_delete(px_); }

  void *get_deleter(std::type_info const &) noexcept override { return 0; }

  void *get_local_deleter(std::type_info const &) noexcept override { return 0; }

  void *get_untyped_deleter() noexcept override { return 0; }

#if defined(SP_USE_STD_ALLOCATOR)

  void *operator new(std::size_t) {
    return std::allocator<this_type>().allocate(1, static_cast<this_type *>(0));
  }

  void operator delete(void *p) {
    std::allocator<this_type>().deallocate(static_cast<this_type *>(p), 1);
  }

#endif

#if defined(SP_USE_QUICK_ALLOCATOR)

  void *operator new(std::size_t) { return quick_allocator<this_type>::alloc(); }

  void operator delete(void *p) { quick_allocator<this_type>::dealloc(p); }

#endif
};

//
// Borland's Codeguard trips up over the -Vx- option here:
//
#ifdef __CODEGUARD__
#  pragma option push -Vx-
#endif

template <class P, class D> class SYMBOL_VISIBLE sp_counted_impl_pd : public sp_counted_base {
private:
  P ptr; // copy constructor must not throw
  D del; // copy/move constructor must not throw

  sp_counted_impl_pd(sp_counted_impl_pd const &);

  sp_counted_impl_pd &operator=(sp_counted_impl_pd const &);

  typedef sp_counted_impl_pd<P, D> this_type;

public:
  // pre: d(p) must not throw

  sp_counted_impl_pd(P p, D &d) : ptr(p), del(static_cast<D &&>(d)) {}

  sp_counted_impl_pd(P p) : ptr(p), del() {}

  void dispose() noexcept override { del(ptr); }

  void *get_deleter(std::type_info const &ti) noexcept override {
    return ti == typeid(D) ? &reinterpret_cast<char &>(del) : 0;
  }

  void *get_local_deleter(std::type_info const &ti) noexcept override {
    return ti == typeid(D) ? boost::detail::get_local_deleter(boost::addressof(del)) : 0;
  }

  void *get_untyped_deleter() noexcept override { return &reinterpret_cast<char &>(del); }

#if defined(SP_USE_STD_ALLOCATOR)

  void *operator new(std::size_t) {
    return std::allocator<this_type>().allocate(1, static_cast<this_type *>(0));
  }

  void operator delete(void *p) {
    std::allocator<this_type>().deallocate(static_cast<this_type *>(p), 1);
  }

#endif

#if defined(SP_USE_QUICK_ALLOCATOR)

  void *operator new(std::size_t) { return quick_allocator<this_type>::alloc(); }

  void operator delete(void *p) { quick_allocator<this_type>::dealloc(p); }

#endif
};

template <class P, class D, class A>
class SYMBOL_VISIBLE sp_counted_impl_pda : public sp_counted_base {
private:
  P p_; // copy constructor must not throw
  D d_; // copy/move constructor must not throw
  A a_; // copy constructor must not throw

  sp_counted_impl_pda(sp_counted_impl_pda const &);

  sp_counted_impl_pda &operator=(sp_counted_impl_pda const &);

  typedef sp_counted_impl_pda<P, D, A> this_type;

public:
  // pre: d( p ) must not throw

  sp_counted_impl_pda(P p, D &d, A a) : p_(p), d_(static_cast<D &&>(d)), a_(a) {}

  sp_counted_impl_pda(P p, A a) : p_(p), d_(a), a_(a) {}

  void dispose() noexcept override { d_(p_); }

  void destroy() noexcept override {
    typedef typename std::allocator_traits<A>::template rebind_alloc<this_type> A2;

    A2 a2(a_);

    this->~this_type();

    a2.deallocate(this, 1);
  }

  void *get_deleter(std::type_info const &ti) noexcept override {
    return ti == typeid(D) ? &reinterpret_cast<char &>(d_) : 0;
  }

  void *get_local_deleter(std::type_info const &ti) noexcept override {
    return ti == typeid(D) ? boost::detail::get_local_deleter(boost::addressof(d_)) : 0;
  }

  void *get_untyped_deleter() noexcept override { return &reinterpret_cast<char &>(d_); }
};

#ifdef __CODEGUARD__
#  pragma option pop
#endif

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/sp_counted_impl.hpp */

/* @file_start smart_ptr/scoped_ptr.hpp */

namespace boost {

//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

template <class T>
class scoped_ptr // noncopyable
{
private:
  T *px;

  scoped_ptr(scoped_ptr const &);

  scoped_ptr &operator=(scoped_ptr const &);

  typedef scoped_ptr<T> this_type;

  void operator==(scoped_ptr const &) const;

  void operator!=(scoped_ptr const &) const;

public:
  typedef T element_type;

  explicit scoped_ptr(T *p = 0) noexcept : px(p) {}

  explicit scoped_ptr(std::auto_ptr<T> p) noexcept : px(p.release()) {}

  ~scoped_ptr() noexcept { boost::checked_delete(px); }

  void reset(T *p = 0) noexcept {
    assert(p == 0 || p != px); // catch self-reset errors
    this_type(p).swap(*this);
  }

  T &operator*() const noexcept {
    assert(px != 0);
    return *px;
  }

  T *operator->() const noexcept {
    assert(px != 0);
    return px;
  }

  T *get() const noexcept { return px; }

  // implicit conversion to "bool"
  explicit operator bool() const noexcept { return px != 0; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  void swap(scoped_ptr &b) noexcept {
    T *tmp = b.px;
    b.px = px;
    px = tmp;
  }
};

template <class T> inline bool operator==(scoped_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator==(std::nullptr_t, scoped_ptr<T> const &p) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator!=(scoped_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() != 0;
}

template <class T> inline bool operator!=(std::nullptr_t, scoped_ptr<T> const &p) noexcept {
  return p.get() != 0;
}

template <class T> inline void swap(scoped_ptr<T> &a, scoped_ptr<T> &b) noexcept { a.swap(b); }

// get_pointer(p) is a generic way to say p.get()

template <class T> inline T *get_pointer(scoped_ptr<T> const &p) noexcept { return p.get(); }

} // namespace boost

/* @file_end smart_ptr/scoped_ptr.hpp */

/* @file_start smart_ptr/scoped_array.hpp */

namespace boost {

//  scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
//  is guaranteed, either on destruction of the scoped_array or via an explicit
//  reset(). Use shared_array or std::vector if your needs are more complex.

template <class T>
class scoped_array // noncopyable
{
private:
  T *px;

  scoped_array(scoped_array const &);

  scoped_array &operator=(scoped_array const &);

  typedef scoped_array<T> this_type;

  void operator==(scoped_array const &) const;

  void operator!=(scoped_array const &) const;

public:
  typedef T element_type;

  explicit scoped_array(T *p = 0) noexcept : px(p) {}

  ~scoped_array() noexcept { boost::checked_array_delete(px); }

  void reset(T *p = 0) noexcept {
    assert(p == 0 || p != px); // catch self-reset errors
    this_type(p).swap(*this);
  }

  T &operator[](std::ptrdiff_t i) const noexcept {
    assert(px != 0);
    assert(i >= 0);
    return px[i];
  }

  T *get() const noexcept { return px; }

  // implicit conversion to "bool"
  explicit operator bool() const noexcept { return px != 0; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  void swap(scoped_array &b) noexcept {
    T *tmp = b.px;
    b.px = px;
    px = tmp;
  }
};

template <class T> inline bool operator==(scoped_array<T> const &p, std::nullptr_t) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator==(std::nullptr_t, scoped_array<T> const &p) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator!=(scoped_array<T> const &p, std::nullptr_t) noexcept {
  return p.get() != 0;
}

template <class T> inline bool operator!=(std::nullptr_t, scoped_array<T> const &p) noexcept {
  return p.get() != 0;
}

template <class T> inline void swap(scoped_array<T> &a, scoped_array<T> &b) noexcept {
  a.swap(b);
}

} // namespace boost

/* @file_end smart_ptr/scoped_array.hpp */

/* @file_start smart_ptr/detail/shared_count.hp */

namespace boost {

namespace detail {

struct sp_nothrow_tag {};

template <class D> struct sp_inplace_tag {};

template <class T> class sp_reference_wrapper {
public:
  explicit sp_reference_wrapper(T &t) : t_(boost::addressof(t)) {}

  template <class Y> void operator()(Y *p) const { (*t_)(p); }

private:
  T *t_;
};

template <class D> struct sp_convert_reference {
  typedef D type;
};

template <class D> struct sp_convert_reference<D &> {
  typedef sp_reference_wrapper<D> type;
};

template <class T> std::size_t sp_hash_pointer(T *p) noexcept {
  boost::uintptr_t v = reinterpret_cast<boost::uintptr_t>(p);

  // match boost::hash<T*>
  return static_cast<std::size_t>(v + (v >> 3));
}

class weak_count;

class shared_count {
private:
  sp_counted_base *pi_;

  friend class weak_count;

public:
  constexpr shared_count() noexcept : pi_(0) {}

  constexpr explicit shared_count(sp_counted_base *pi) noexcept : pi_(pi) {}

  template <class Y> explicit shared_count(Y *p) : pi_(0) {
    pi_ = new sp_counted_impl_p<Y>(p);

    if (pi_ == 0) {
      boost::checked_delete(p);
      boost::throw_exception(std::bad_alloc());
    }
  }

  template <class P, class D> shared_count(P p, D d) : pi_(0) {
    pi_ = new sp_counted_impl_pd<P, D>(p, d);

    if (pi_ == 0) {
      d(p); // delete p
      boost::throw_exception(std::bad_alloc());
    }
  }

  template <class P, class D> shared_count(P p, sp_inplace_tag<D>) : pi_(0) {
    pi_ = new sp_counted_impl_pd<P, D>(p);

    if (pi_ == 0) {
      D::operator_fn(p); // delete p
      boost::throw_exception(std::bad_alloc());
    }
  }

  template <class P, class D, class A> shared_count(P p, D d, A a) : pi_(0) {
    typedef sp_counted_impl_pda<P, D, A> impl_type;
    typedef typename std::allocator_traits<A>::template rebind_alloc<impl_type> A2;
    A2 a2(a);

    pi_ = a2.allocate(1);

    if (pi_ != 0) {
      ::new (static_cast<void *>(pi_)) impl_type(p, d, a);
    } else {
      d(p);
      boost::throw_exception(std::bad_alloc());
    }
  }

  template <class P, class D, class A> shared_count(P p, sp_inplace_tag<D>, A a) : pi_(0) {
    typedef sp_counted_impl_pda<P, D, A> impl_type;
    typedef typename std::allocator_traits<A>::template rebind_alloc<impl_type> A2;
    A2 a2(a);

    pi_ = a2.allocate(1);

    if (pi_ != 0) {
      ::new (static_cast<void *>(pi_)) impl_type(p, a);
    } else {
      D::operator_fn(p);
      boost::throw_exception(std::bad_alloc());
    }
  }

  // auto_ptr<Y> is special cased to provide the strong guarantee

  template <class Y>
  explicit shared_count(std::auto_ptr<Y> &r) : pi_(new sp_counted_impl_p<Y>(r.get())) {
    if (pi_ == 0) { boost::throw_exception(std::bad_alloc()); }
    r.release();
  }

  template <class Y, class D> explicit shared_count(std::unique_ptr<Y, D> &r) : pi_(0) {
    typedef typename sp_convert_reference<D>::type D2;

    D2 d2(static_cast<D &&>(r.get_deleter()));
    pi_ = new sp_counted_impl_pd<typename std::unique_ptr<Y, D>::pointer, D2>(r.get(), d2);

    if (pi_ == 0) { boost::throw_exception(std::bad_alloc()); }
    r.release();
  }

  template <class Y, class D>
  explicit shared_count(boost::movelib::unique_ptr<Y, D> &r) : pi_(0) {
    typedef typename sp_convert_reference<D>::type D2;

    D2 d2(r.get_deleter());
    pi_ = new sp_counted_impl_pd<typename boost::movelib::unique_ptr<Y, D>::pointer, D2>(
        r.get(), d2);

    if (pi_ == 0) { boost::throw_exception(std::bad_alloc()); }
    r.release();
  }

  ~shared_count() /*noexcept*/
  {
    if (pi_ != 0) pi_->release();
  }

  shared_count(shared_count const &r) noexcept : pi_(r.pi_) {
    if (pi_ != 0) pi_->add_ref_copy();
  }

  shared_count(shared_count &&r) noexcept : pi_(r.pi_) { r.pi_ = 0; }

  explicit shared_count(weak_count const &r); // throws bad_weak_ptr when r.use_count() == 0
  shared_count(weak_count const &r,
               sp_nothrow_tag) noexcept; // constructs an empty *this when r.use_count() == 0

  shared_count &operator=(shared_count const &r) noexcept {
    sp_counted_base *tmp = r.pi_;

    if (tmp != pi_) {
      if (tmp != 0) tmp->add_ref_copy();
      if (pi_ != 0) pi_->release();
      pi_ = tmp;
    }

    return *this;
  }

  void swap(shared_count &r) noexcept {
    sp_counted_base *tmp = r.pi_;
    r.pi_ = pi_;
    pi_ = tmp;
  }

  long use_count() const noexcept { return pi_ != 0 ? pi_->use_count() : 0; }

  bool unique() const noexcept { return use_count() == 1; }

  bool empty() const noexcept { return pi_ == 0; }

  bool operator==(shared_count const &r) const noexcept { return pi_ == r.pi_; }

  bool operator==(weak_count const &r) const noexcept;

  bool operator<(shared_count const &r) const noexcept {
    return std::less<sp_counted_base *>()(pi_, r.pi_);
  }

  bool operator<(weak_count const &r) const noexcept;

  void *get_deleter(std::type_info const &ti) const noexcept {
    return pi_ ? pi_->get_deleter(ti) : 0;
  }

  void *get_local_deleter(std::type_info const &ti) const noexcept {
    return pi_ ? pi_->get_local_deleter(ti) : 0;
  }

  void *get_untyped_deleter() const noexcept { return pi_ ? pi_->get_untyped_deleter() : 0; }

  std::size_t hash_value() const noexcept { return sp_hash_pointer(pi_); }
};

class weak_count {
private:
  sp_counted_base *pi_;

  friend class shared_count;

public:
  constexpr weak_count() noexcept : pi_(0) {}

  weak_count(shared_count const &r) noexcept : pi_(r.pi_) {
    if (pi_ != 0) pi_->weak_add_ref();
  }

  weak_count(weak_count const &r) noexcept : pi_(r.pi_) {
    if (pi_ != 0) pi_->weak_add_ref();
  }

  // Move support

  weak_count(weak_count &&r) noexcept : pi_(r.pi_) { r.pi_ = 0; }

  ~weak_count() /*noexcept*/
  {
    if (pi_ != 0) pi_->weak_release();
  }

  weak_count &operator=(shared_count const &r) noexcept {
    sp_counted_base *tmp = r.pi_;

    if (tmp != pi_) {
      if (tmp != 0) tmp->weak_add_ref();
      if (pi_ != 0) pi_->weak_release();
      pi_ = tmp;
    }

    return *this;
  }

  weak_count &operator=(weak_count const &r) noexcept {
    sp_counted_base *tmp = r.pi_;

    if (tmp != pi_) {
      if (tmp != 0) tmp->weak_add_ref();
      if (pi_ != 0) pi_->weak_release();
      pi_ = tmp;
    }

    return *this;
  }

  void swap(weak_count &r) noexcept {
    sp_counted_base *tmp = r.pi_;
    r.pi_ = pi_;
    pi_ = tmp;
  }

  long use_count() const noexcept { return pi_ != 0 ? pi_->use_count() : 0; }

  bool empty() const noexcept { return pi_ == 0; }

  bool operator==(weak_count const &r) const noexcept { return pi_ == r.pi_; }

  bool operator==(shared_count const &r) const noexcept { return pi_ == r.pi_; }

  bool operator<(weak_count const &r) const noexcept {
    return std::less<sp_counted_base *>()(pi_, r.pi_);
  }

  bool operator<(shared_count const &r) const noexcept {
    return std::less<sp_counted_base *>()(pi_, r.pi_);
  }

  std::size_t hash_value() const noexcept { return sp_hash_pointer(pi_); }
};

inline shared_count::shared_count(weak_count const &r) : pi_(r.pi_) {
  if (pi_ == 0 || !pi_->add_ref_lock()) { boost::throw_exception(boost::bad_weak_ptr()); }
}

inline shared_count::shared_count(weak_count const &r, sp_nothrow_tag) noexcept : pi_(r.pi_) {
  if (pi_ != 0 && !pi_->add_ref_lock()) { pi_ = 0; }
}

inline bool shared_count::operator==(weak_count const &r) const noexcept {
  return pi_ == r.pi_;
}

inline bool shared_count::operator<(weak_count const &r) const noexcept {
  return std::less<sp_counted_base *>()(pi_, r.pi_);
}

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/shared_count.hpp */

/* @file_start smart_ptr/detail/local_counted_base.hpp */

#include <utility>

namespace boost {

namespace detail {

class SYMBOL_VISIBLE local_counted_base {
private:
  local_counted_base &operator=(local_counted_base const &);

private:
  // not 'int' or 'unsigned' to avoid aliasing and enable optimizations
  enum count_type { min_ = 0, initial_ = 1, max_ = 2147483647 };

  count_type local_use_count_;

public:
  constexpr local_counted_base() noexcept : local_use_count_(initial_) {}

  constexpr local_counted_base(local_counted_base const &) noexcept
      : local_use_count_(initial_) {}

  virtual ~local_counted_base() /*noexcept*/ {}

  virtual void local_cb_destroy() noexcept = 0;

  virtual boost::detail::shared_count local_cb_get_shared_count() const noexcept = 0;

  void add_ref() noexcept {
#if !defined(__NVCC__)
#  if defined(__has_builtin)
#    if __has_builtin(__builtin_assume)
    __builtin_assume(local_use_count_ >= 1);
#    endif
#  endif
#endif

    local_use_count_ = static_cast<count_type>(local_use_count_ + 1);
  }

  void release() noexcept {
    local_use_count_ = static_cast<count_type>(local_use_count_ - 1);

    if (local_use_count_ == 0) { local_cb_destroy(); }
  }

  long local_use_count() const noexcept { return local_use_count_; }
};

class SYMBOL_VISIBLE local_counted_impl : public local_counted_base {
private:
  local_counted_impl(local_counted_impl const &);

private:
  shared_count pn_;

public:
  explicit local_counted_impl(shared_count const &pn) noexcept : pn_(pn) {}

  explicit local_counted_impl(shared_count &&pn) noexcept : pn_(std::move(pn)) {}

  void local_cb_destroy() noexcept override { delete this; }

  boost::detail::shared_count local_cb_get_shared_count() const noexcept override {
    return pn_;
  }
};

class SYMBOL_VISIBLE local_counted_impl_em : public local_counted_base {
public:
  shared_count pn_;

  void local_cb_destroy() noexcept override { shared_count().swap(pn_); }

  boost::detail::shared_count local_cb_get_shared_count() const noexcept override {
    return pn_;
  }
};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/local_counted_base.hpp */

/* @file_start smart_ptr/detail/sp_convertible.hpp */

namespace boost {

namespace detail {

template <class Y, class T> struct sp_convertible {
  typedef char (&yes)[1];
  typedef char (&no)[2];

  static yes f(T *);

  static no f(...);

  enum _vt { value = sizeof((f)(static_cast<Y *>(0))) == sizeof(yes) };
};

template <class Y, class T> struct sp_convertible<Y, T[]> {
  enum _vt { value = false };
};

template <class Y, class T> struct sp_convertible<Y[], T[]> {
  enum _vt { value = sp_convertible<Y[1], T[1]>::value };
};

template <class Y, std::size_t N, class T> struct sp_convertible<Y[N], T[]> {
  enum _vt { value = sp_convertible<Y[1], T[1]>::value };
};

struct sp_empty {};

template <bool> struct sp_enable_if_convertible_impl;

template <> struct sp_enable_if_convertible_impl<true> {
  typedef sp_empty type;
};

template <> struct sp_enable_if_convertible_impl<false> {};

template <class Y, class T>
struct sp_enable_if_convertible
    : public sp_enable_if_convertible_impl<sp_convertible<Y, T>::value> {};

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/sp_convertible.hpp */

/* @file_start smart_ptr/shared_ptr.hpp */

#include <algorithm>  // for std::swap
#include <cstddef>    // for std::size_t
#include <functional> // for std::less
#include <memory>     // for std::auto_ptr
#include <typeinfo>   // for std::bad_cast

#if !defined(BOOST_NO_IOSTREAM)
#  if !defined(BOOST_NO_IOSFWD)

#    include <iosfwd> // for std::basic_ostream

#  else
#    include <ostream>
#  endif
#endif

namespace boost {

template <class T> class shared_ptr;

template <class T> class weak_ptr;

template <class T> class enable_shared_from_this;

class enable_shared_from_raw;

namespace detail {

// sp_element, element_type

template <class T> struct sp_element {
  typedef T type;
};

template <class T> struct sp_element<T[]> {
  typedef T type;
};

template <class T, std::size_t N> struct sp_element<T[N]> {
  typedef T type;
};

// sp_dereference, return type of operator*

template <class T> struct sp_dereference {
  typedef T &type;
};

template <> struct sp_dereference<void> {
  typedef void type;
};

template <> struct sp_dereference<void const> {
  typedef void type;
};

template <> struct sp_dereference<void volatile> {
  typedef void type;
};

template <> struct sp_dereference<void const volatile> {
  typedef void type;
};

template <class T> struct sp_dereference<T[]> {
  typedef void type;
};

template <class T, std::size_t N> struct sp_dereference<T[N]> {
  typedef void type;
};

// sp_member_access, return type of operator->

template <class T> struct sp_member_access {
  typedef T *type;
};

template <class T> struct sp_member_access<T[]> {
  typedef void type;
};

template <class T, std::size_t N> struct sp_member_access<T[N]> {
  typedef void type;
};

// sp_array_access, return type of operator[]

template <class T> struct sp_array_access {
  typedef void type;
};

template <class T> struct sp_array_access<T[]> {
  typedef T &type;
};

template <class T, std::size_t N> struct sp_array_access<T[N]> {
  typedef T &type;
};

// sp_extent, for operator[] index check

template <class T> struct sp_extent {
  enum _vt { value = 0 };
};

template <class T, std::size_t N> struct sp_extent<T[N]> {
  enum _vt { value = N };
};

// enable_shared_from_this support

template <class X, class Y, class T>
inline void sp_enable_shared_from_this(boost::shared_ptr<X> const *ppx,
                                       Y const *py,
                                       boost::enable_shared_from_this<T> const *pe) {
  if (pe != 0) { pe->_internal_accept_owner(ppx, const_cast<Y *>(py)); }
}

template <class X, class Y>
inline void sp_enable_shared_from_this(boost::shared_ptr<X> *ppx,
                                       Y const *py,
                                       boost::enable_shared_from_raw const *pe);

#ifdef _MANAGED

// Avoid C4793, ... causes native code generation

struct sp_any_pointer {
  template <class T> sp_any_pointer(T *) {}
};

inline void sp_enable_shared_from_this(sp_any_pointer, sp_any_pointer, sp_any_pointer) {}

#else  // _MANAGED

inline void sp_enable_shared_from_this(...) {}

#endif // _MANAGED

// rvalue auto_ptr support based on a technique by Dave Abrahams

template <class T, class R> struct sp_enable_if_auto_ptr {};

template <class T, class R> struct sp_enable_if_auto_ptr<std::auto_ptr<T>, R> {
  typedef R type;
};

// sp_assert_convertible

template <class Y, class T> inline void sp_assert_convertible() noexcept {
  // static_assert( sp_convertible< Y, T >::value );
  typedef char tmp[sp_convertible<Y, T>::value ? 1 : -1];
  (void)sizeof(tmp);
}

// pointer constructor helper

template <class T, class Y>
inline void
sp_pointer_construct(boost::shared_ptr<T> *ppx, Y *p, boost::detail::shared_count &pn) {
  boost::detail::shared_count(p).swap(pn);
  boost::detail::sp_enable_shared_from_this(ppx, p, p);
}

template <class T, class Y>
inline void
sp_pointer_construct(boost::shared_ptr<T[]> * /*ppx*/, Y *p, boost::detail::shared_count &pn) {
  sp_assert_convertible<Y[], T[]>();
  boost::detail::shared_count(p, boost::checked_array_deleter<T>()).swap(pn);
}

template <class T, std::size_t N, class Y>
inline void sp_pointer_construct(boost::shared_ptr<T[N]> * /*ppx*/,
                                 Y *p,
                                 boost::detail::shared_count &pn) {
  sp_assert_convertible<Y[N], T[N]>();
  boost::detail::shared_count(p, boost::checked_array_deleter<T>()).swap(pn);
}

// deleter constructor helper

template <class T, class Y> inline void sp_deleter_construct(boost::shared_ptr<T> *ppx, Y *p) {
  boost::detail::sp_enable_shared_from_this(ppx, p, p);
}

template <class T, class Y>
inline void sp_deleter_construct(boost::shared_ptr<T[]> * /*ppx*/, Y * /*p*/) {
  sp_assert_convertible<Y[], T[]>();
}

template <class T, std::size_t N, class Y>
inline void sp_deleter_construct(boost::shared_ptr<T[N]> * /*ppx*/, Y * /*p*/) {
  sp_assert_convertible<Y[N], T[N]>();
}

struct sp_internal_constructor_tag {};

} // namespace detail

//
//  shared_ptr
//
//  An enhanced relative of scoped_ptr with reference counted copy semantics.
//  The object pointed to is deleted when the last shared_ptr pointing to it
//  is destroyed or reset.
//

template <class T> class shared_ptr {
private:
  // Borland 5.5.1 specific workaround
  typedef shared_ptr<T> this_type;

public:
  typedef typename boost::detail::sp_element<T>::type element_type;

  constexpr shared_ptr() noexcept : px(0), pn() {}

  constexpr shared_ptr(std::nullptr_t) noexcept : px(0), pn() {}

  constexpr shared_ptr(boost::detail::sp_internal_constructor_tag,
                       element_type *px_,
                       boost::detail::shared_count const &pn_) noexcept
      : px(px_), pn(pn_) {}

  constexpr shared_ptr(boost::detail::sp_internal_constructor_tag,
                       element_type *px_,
                       boost::detail::shared_count &&pn_) noexcept
      : px(px_), pn(std::move(pn_)) {}

  template <class Y>
  explicit shared_ptr(Y *p)
      : px(p),
        pn() // Y must be complete
  {
    boost::detail::sp_pointer_construct(this, p, pn);
  }

  //
  // Requirements: D's copy/move constructors must not throw
  //
  // shared_ptr will release p by calling d(p)
  //

  template <class Y, class D> shared_ptr(Y *p, D d) : px(p), pn(p, static_cast<D &&>(d)) {
    boost::detail::sp_deleter_construct(this, p);
  }

  template <class D> shared_ptr(std::nullptr_t p, D d) : px(p), pn(p, static_cast<D &&>(d)) {}

  // As above, but with allocator. A's copy constructor shall not throw.

  template <class Y, class D, class A>
  shared_ptr(Y *p, D d, A a) : px(p), pn(p, static_cast<D &&>(d), a) {
    boost::detail::sp_deleter_construct(this, p);
  }

  template <class D, class A>
  shared_ptr(std::nullptr_t p, D d, A a) : px(p), pn(p, static_cast<D &&>(d), a) {}

  //  generated copy constructor, destructor are fine...

  // ... except in C++0x, move disables the implicit copy

  shared_ptr(shared_ptr const &r) noexcept : px(r.px), pn(r.pn) {}

  template <class Y>
  explicit shared_ptr(weak_ptr<Y> const &r)
      : pn(r.pn) // may throw
  {
    boost::detail::sp_assert_convertible<Y, T>();

    // it is now safe to copy r.px, as pn(r.pn) did not throw
    px = r.px;
  }

  template <class Y>
  shared_ptr(weak_ptr<Y> const &r, boost::detail::sp_nothrow_tag) noexcept
      : px(0), pn(r.pn, boost::detail::sp_nothrow_tag()) {
    if (!pn.empty()) { px = r.px; }
  }

  template <class Y>
  shared_ptr(shared_ptr<Y> const &r,
             typename boost::detail::sp_enable_if_convertible<Y, T>::type =
                 boost::detail::sp_empty()) noexcept
      : px(r.px), pn(r.pn) {
    boost::detail::sp_assert_convertible<Y, T>();
  }

  // aliasing
  template <class Y>
  shared_ptr(shared_ptr<Y> const &r, element_type *p) noexcept : px(p), pn(r.pn) {}

  template <class Y> explicit shared_ptr(std::auto_ptr<Y> &r) : px(r.get()), pn() {
    boost::detail::sp_assert_convertible<Y, T>();

    Y *tmp = r.get();
    pn = boost::detail::shared_count(r);

    boost::detail::sp_deleter_construct(this, tmp);
  }

  template <class Y> shared_ptr(std::auto_ptr<Y> &&r) : px(r.get()), pn() {
    boost::detail::sp_assert_convertible<Y, T>();

    Y *tmp = r.get();
    pn = boost::detail::shared_count(r);

    boost::detail::sp_deleter_construct(this, tmp);
  }

  template <class Y, class D> shared_ptr(std::unique_ptr<Y, D> &&r) : px(r.get()), pn() {
    boost::detail::sp_assert_convertible<Y, T>();

    typename std::unique_ptr<Y, D>::pointer tmp = r.get();

    if (tmp != 0) {
      pn = boost::detail::shared_count(r);
      boost::detail::sp_deleter_construct(this, tmp);
    }
  }

  template <class Y, class D>
  shared_ptr(boost::movelib::unique_ptr<Y, D> r) : px(r.get()), pn() {
    boost::detail::sp_assert_convertible<Y, T>();

    typename boost::movelib::unique_ptr<Y, D>::pointer tmp = r.get();

    if (tmp != 0) {
      pn = boost::detail::shared_count(r);
      boost::detail::sp_deleter_construct(this, tmp);
    }
  }

  // assignment

  shared_ptr &operator=(shared_ptr const &r) noexcept {
    this_type(r).swap(*this);
    return *this;
  }

  template <class Y> shared_ptr &operator=(shared_ptr<Y> const &r) noexcept {
    this_type(r).swap(*this);
    return *this;
  }

  template <class Y> shared_ptr &operator=(std::auto_ptr<Y> &r) {
    this_type(r).swap(*this);
    return *this;
  }

  template <class Y> shared_ptr &operator=(std::auto_ptr<Y> &&r) {
    this_type(static_cast<std::auto_ptr<Y> &&>(r)).swap(*this);
    return *this;
  }

  template <class Y, class D> shared_ptr &operator=(std::unique_ptr<Y, D> &&r) {
    this_type(static_cast<std::unique_ptr<Y, D> &&>(r)).swap(*this);
    return *this;
  }

  template <class Y, class D> shared_ptr &operator=(boost::movelib::unique_ptr<Y, D> r) {
    // this_type( static_cast< unique_ptr<Y, D> && >( r ) ).swap( *this );

    boost::detail::sp_assert_convertible<Y, T>();

    typename boost::movelib::unique_ptr<Y, D>::pointer p = r.get();

    shared_ptr tmp;

    if (p != 0) {
      tmp.px = p;
      tmp.pn = boost::detail::shared_count(r);

      boost::detail::sp_deleter_construct(&tmp, p);
    }

    tmp.swap(*this);

    return *this;
  }

  // Move support

  shared_ptr(shared_ptr &&r) noexcept
      : px(r.px), pn(static_cast<boost::detail::shared_count &&>(r.pn)) {
    r.px = 0;
  }

  template <class Y>
  shared_ptr(shared_ptr<Y> &&r,
             typename boost::detail::sp_enable_if_convertible<Y, T>::type =
                 boost::detail::sp_empty()) noexcept
      : px(r.px), pn(static_cast<boost::detail::shared_count &&>(r.pn)) {
    boost::detail::sp_assert_convertible<Y, T>();
    r.px = 0;
  }

  shared_ptr &operator=(shared_ptr &&r) noexcept {
    this_type(static_cast<shared_ptr &&>(r)).swap(*this);
    return *this;
  }

  template <class Y> shared_ptr &operator=(shared_ptr<Y> &&r) noexcept {
    this_type(static_cast<shared_ptr<Y> &&>(r)).swap(*this);
    return *this;
  }

  // aliasing move
  template <class Y> shared_ptr(shared_ptr<Y> &&r, element_type *p) noexcept : px(p), pn() {
    pn.swap(r.pn);
    r.px = 0;
  }

  shared_ptr &operator=(std::nullptr_t) noexcept {
    this_type().swap(*this);
    return *this;
  }

  void reset() noexcept { this_type().swap(*this); }

  template <class Y>
  void reset(Y *p)             // Y must be complete
  {
    assert(p == 0 || p != px); // catch self-reset errors
    this_type(p).swap(*this);
  }

  template <class Y, class D> void reset(Y *p, D d) {
    this_type(p, static_cast<D &&>(d)).swap(*this);
  }

  template <class Y, class D, class A> void reset(Y *p, D d, A a) {
    this_type(p, static_cast<D &&>(d), a).swap(*this);
  }

  template <class Y> void reset(shared_ptr<Y> const &r, element_type *p) noexcept {
    this_type(r, p).swap(*this);
  }

  template <class Y> void reset(shared_ptr<Y> &&r, element_type *p) noexcept {
    this_type(static_cast<shared_ptr<Y> &&>(r), p).swap(*this);
  }

  typename boost::detail::sp_dereference<T>::type operator*() const noexcept {
    assert(px != 0);
    return *px;
  }

  typename boost::detail::sp_member_access<T>::type operator->() const noexcept {
    assert(px != 0);
    return px;
  }

  typename boost::detail::sp_array_access<T>::type
  operator[](std::ptrdiff_t i) const noexcept {
    assert(px != 0);
    assert(i >= 0 && (i < boost::detail::sp_extent<T>::value ||
                      boost::detail::sp_extent<T>::value == 0));

    return static_cast<typename boost::detail::sp_array_access<T>::type>(px[i]);
  }

  element_type *get() const noexcept { return px; }

  // implicit conversion to "bool"

  explicit operator bool() const noexcept { return px != 0; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  bool unique() const noexcept { return pn.unique(); }

  long use_count() const noexcept { return pn.use_count(); }

  void swap(shared_ptr &other) noexcept {
    std::swap(px, other.px);
    pn.swap(other.pn);
  }

  template <class Y> bool owner_before(shared_ptr<Y> const &rhs) const noexcept {
    return pn < rhs.pn;
  }

  template <class Y> bool owner_before(weak_ptr<Y> const &rhs) const noexcept {
    return pn < rhs.pn;
  }

  template <class Y> bool owner_equals(shared_ptr<Y> const &rhs) const noexcept {
    return pn == rhs.pn;
  }

  template <class Y> bool owner_equals(weak_ptr<Y> const &rhs) const noexcept {
    return pn == rhs.pn;
  }

  std::size_t owner_hash_value() const noexcept { return pn.hash_value(); }

  void *_internal_get_deleter(std::type_info const &ti) const noexcept {
    return pn.get_deleter(ti);
  }

  void *_internal_get_local_deleter(std::type_info const &ti) const noexcept {
    return pn.get_local_deleter(ti);
  }

  void *_internal_get_untyped_deleter() const noexcept { return pn.get_untyped_deleter(); }

  bool _internal_equiv(shared_ptr const &r) const noexcept { return px == r.px && pn == r.pn; }

  boost::detail::shared_count _internal_count() const noexcept { return pn; }

  // Tasteless as this may seem, making all members public allows member
  // templates to work in the absence of member template friends. (Matthew
  // Langston)

private:
  template <class Y> friend class shared_ptr;

  template <class Y> friend class weak_ptr;

  element_type *px;               // contained pointer
  boost::detail::shared_count pn; // reference counter

};                                // shared_ptr

template <class T, class U>
inline bool operator==(shared_ptr<T> const &a, shared_ptr<U> const &b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(shared_ptr<T> const &a, shared_ptr<U> const &b) noexcept {
  return a.get() != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template <class T>
inline bool operator!=(shared_ptr<T> const &a, shared_ptr<T> const &b) noexcept {
  return a.get() != b.get();
}

#endif

template <class T> inline bool operator==(shared_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator==(std::nullptr_t, shared_ptr<T> const &p) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator!=(shared_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() != 0;
}

template <class T> inline bool operator!=(std::nullptr_t, shared_ptr<T> const &p) noexcept {
  return p.get() != 0;
}

template <class T, class U>
inline bool operator<(shared_ptr<T> const &a, shared_ptr<U> const &b) noexcept {
  return a.owner_before(b);
}

template <class T> inline void swap(shared_ptr<T> &a, shared_ptr<T> &b) noexcept { a.swap(b); }

template <class T, class U>
shared_ptr<T> static_pointer_cast(shared_ptr<U> const &r) noexcept {
  (void)static_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = static_cast<E *>(r.get());
  return shared_ptr<T>(r, p);
}

template <class T, class U> shared_ptr<T> const_pointer_cast(shared_ptr<U> const &r) noexcept {
  (void)const_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = const_cast<E *>(r.get());
  return shared_ptr<T>(r, p);
}

template <class T, class U>
shared_ptr<T> dynamic_pointer_cast(shared_ptr<U> const &r) noexcept {
  (void)dynamic_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = dynamic_cast<E *>(r.get());
  return p ? shared_ptr<T>(r, p) : shared_ptr<T>();
}

template <class T, class U>
shared_ptr<T> reinterpret_pointer_cast(shared_ptr<U> const &r) noexcept {
  (void)reinterpret_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = reinterpret_cast<E *>(r.get());
  return shared_ptr<T>(r, p);
}

template <class T, class U> shared_ptr<T> static_pointer_cast(shared_ptr<U> &&r) noexcept {
  (void)static_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = static_cast<E *>(r.get());
  return shared_ptr<T>(std::move(r), p);
}

template <class T, class U> shared_ptr<T> const_pointer_cast(shared_ptr<U> &&r) noexcept {
  (void)const_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = const_cast<E *>(r.get());
  return shared_ptr<T>(std::move(r), p);
}

template <class T, class U> shared_ptr<T> dynamic_pointer_cast(shared_ptr<U> &&r) noexcept {
  (void)dynamic_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = dynamic_cast<E *>(r.get());
  return p ? shared_ptr<T>(std::move(r), p) : shared_ptr<T>();
}

template <class T, class U>
shared_ptr<T> reinterpret_pointer_cast(shared_ptr<U> &&r) noexcept {
  (void)reinterpret_cast<T *>(static_cast<U *>(0));

  typedef typename shared_ptr<T>::element_type E;

  E *p = reinterpret_cast<E *>(r.get());
  return shared_ptr<T>(std::move(r), p);
}

// get_pointer() enables boost::mem_fn to recognize shared_ptr

template <class T>
inline typename shared_ptr<T>::element_type *get_pointer(shared_ptr<T> const &p) noexcept {
  return p.get();
}

// operator<<

template <class E, class T, class Y>
std::basic_ostream<E, T> &operator<<(std::basic_ostream<E, T> &os, shared_ptr<Y> const &p) {
  os << p.get();
  return os;
}

// get_deleter

namespace detail {

template <class D, class T> D *basic_get_deleter(shared_ptr<T> const &p) noexcept {
  return static_cast<D *>(p._internal_get_deleter(typeid(D)));
}

template <class D, class T> D *basic_get_local_deleter(D *, shared_ptr<T> const &p) noexcept;

template <class D, class T>
D const *basic_get_local_deleter(D const *, shared_ptr<T> const &p) noexcept;

class esft2_deleter_wrapper {
private:
  shared_ptr<void const volatile> deleter_;

public:
  esft2_deleter_wrapper() noexcept {}

  template <class T> void set_deleter(shared_ptr<T> const &deleter) noexcept {
    deleter_ = deleter;
  }

  template <typename D> D *get_deleter() const noexcept {
    return boost::detail::basic_get_deleter<D>(deleter_);
  }

  template <class T> void operator()(T *) noexcept {
    assert(deleter_.use_count() <= 1);
    deleter_.reset();
  }
};

} // namespace detail

template <class D, class T> D *get_deleter(shared_ptr<T> const &p) noexcept {
  D *d = boost::detail::basic_get_deleter<D>(p);

  if (d == 0) { d = boost::detail::basic_get_local_deleter(d, p); }

  if (d == 0) {
    boost::detail::esft2_deleter_wrapper *del_wrapper =
        boost::detail::basic_get_deleter<boost::detail::esft2_deleter_wrapper>(p);
    // The following get_deleter method call is fully qualified because
    // older versions of gcc (2.95, 3.2.3) fail to compile it when written
    // del_wrapper->get_deleter<D>()
    if (del_wrapper) d = del_wrapper->::boost::detail::esft2_deleter_wrapper::get_deleter<D>();
  }

  return d;
}

// atomic access

template <class T> inline bool atomic_is_lock_free(shared_ptr<T> const * /*p*/) noexcept {
  return false;
}

template <class T> shared_ptr<T> atomic_load(shared_ptr<T> const *p) noexcept {
  boost::detail::spinlock_pool<2>::scoped_lock lock(p);
  return *p;
}

template <class T, class M>
inline shared_ptr<T> atomic_load_explicit(shared_ptr<T> const *p,
                                          /*memory_order mo*/ M) noexcept {
  return atomic_load(p);
}

template <class T> void atomic_store(shared_ptr<T> *p, shared_ptr<T> r) noexcept {
  boost::detail::spinlock_pool<2>::scoped_lock lock(p);
  p->swap(r);
}

template <class T, class M>
inline void atomic_store_explicit(shared_ptr<T> *p,
                                  shared_ptr<T> r,
                                  /*memory_order mo*/ M) noexcept {
  atomic_store(p, r); // std::move( r )
}

template <class T> shared_ptr<T> atomic_exchange(shared_ptr<T> *p, shared_ptr<T> r) noexcept {
  boost::detail::spinlock &sp = boost::detail::spinlock_pool<2>::spinlock_for(p);

  sp.lock();
  p->swap(r);
  sp.unlock();

  return r; // return std::move( r )
}

template <class T, class M>
shared_ptr<T> inline atomic_exchange_explicit(shared_ptr<T> *p,
                                              shared_ptr<T> r,
                                              /*memory_order mo*/ M) noexcept {
  return atomic_exchange(p, r); // std::move( r )
}

template <class T>
bool atomic_compare_exchange(shared_ptr<T> *p, shared_ptr<T> *v, shared_ptr<T> w) noexcept {
  boost::detail::spinlock &sp = boost::detail::spinlock_pool<2>::spinlock_for(p);

  sp.lock();

  if (p->_internal_equiv(*v)) {
    p->swap(w);

    sp.unlock();

    return true;
  } else {
    shared_ptr<T> tmp(*p);

    sp.unlock();

    tmp.swap(*v);
    return false;
  }
}

template <class T, class M>
inline bool atomic_compare_exchange_explicit(shared_ptr<T> *p,
                                             shared_ptr<T> *v,
                                             shared_ptr<T> w,
                                             /*memory_order success*/ M,
                                             /*memory_order failure*/ M) noexcept {
  return atomic_compare_exchange(p, v, w); // std::move( w )
}

// hash_value

template <class T> struct hash;

template <class T> std::size_t hash_value(boost::shared_ptr<T> const &p) noexcept {
  return boost::hash<typename boost::shared_ptr<T>::element_type *>()(p.get());
}

} // namespace boost

// std::hash

namespace std {

template <class T> struct hash<::boost::shared_ptr<T>> {
  std::size_t operator()(::boost::shared_ptr<T> const &p) const noexcept {
    return std::hash<typename ::boost::shared_ptr<T>::element_type *>()(p.get());
  }
};

} // namespace std

/* @file_start smart_ptr/detail/local_sp_deleter.hpp */

namespace boost {

namespace detail {

template <class D> class local_sp_deleter : public local_counted_impl_em {
private:
  D d_;

public:
  local_sp_deleter() : d_() {}

  explicit local_sp_deleter(D const &d) noexcept : d_(d) {}

  explicit local_sp_deleter(D &&d) noexcept : d_(std::move(d)) {}

  D &deleter() noexcept { return d_; }

  template <class Y> void operator()(Y *p) noexcept { d_(p); }

  void operator()(std::nullptr_t p) noexcept { d_(p); }
};

template <> class local_sp_deleter<void> {};

template <class D> D *get_local_deleter(local_sp_deleter<D> *p) noexcept {
  return &p->deleter();
}

inline void *get_local_deleter(local_sp_deleter<void> * /*p*/) noexcept { return 0; }

} // namespace detail

} // namespace boost

/* @file_end smart_ptr/detail/local_sp_deleter.hpp */
namespace boost {

namespace detail {

template <class D, class T> D *basic_get_local_deleter(D *, shared_ptr<T> const &p) noexcept {
  return static_cast<D *>(p._internal_get_local_deleter(typeid(local_sp_deleter<D>)));
}

template <class D, class T>
D const *basic_get_local_deleter(D const *, shared_ptr<T> const &p) noexcept {
  return static_cast<D *>(p._internal_get_local_deleter(typeid(local_sp_deleter<D>)));
}

} // namespace detail

#if defined(__cpp_deduction_guides)

template <class T> shared_ptr(weak_ptr<T>) -> shared_ptr<T>;
template <class T, class D> shared_ptr(std::unique_ptr<T, D>) -> shared_ptr<T>;

#endif

} // namespace boost

/* @file_end smart_ptr/shared_ptr.hpp */

/* @file_start smart_ptr/atomic_shared_ptr.hpp */

#include <cstring>

namespace boost {

template <class T> class atomic_shared_ptr {
private:
  boost::shared_ptr<T> p_;

  mutable boost::detail::spinlock l_;

  atomic_shared_ptr(const atomic_shared_ptr &);

  atomic_shared_ptr &operator=(const atomic_shared_ptr &);

private:
  bool compare_exchange(shared_ptr<T> &v, shared_ptr<T> w) noexcept {
    l_.lock();

    if (p_._internal_equiv(v)) {
      p_.swap(w);

      l_.unlock();
      return true;
    } else {
      shared_ptr<T> tmp(p_);

      l_.unlock();

      tmp.swap(v);
      return false;
    }
  }

public:
  constexpr atomic_shared_ptr() noexcept : l_ BOOST_DETAIL_SPINLOCK_INIT {}

  atomic_shared_ptr(shared_ptr<T> p) noexcept
      : p_(std::move(p)), l_ BOOST_DETAIL_SPINLOCK_INIT {}

  atomic_shared_ptr &operator=(shared_ptr<T> r) noexcept {
    boost::detail::spinlock::scoped_lock lock(l_);
    p_.swap(r);

    return *this;
  }

  constexpr bool is_lock_free() const noexcept { return false; }

  shared_ptr<T> load() const noexcept {
    boost::detail::spinlock::scoped_lock lock(l_);
    return p_;
  }

  template <class M> shared_ptr<T> load(M) const noexcept {
    boost::detail::spinlock::scoped_lock lock(l_);
    return p_;
  }

  operator shared_ptr<T>() const noexcept {
    boost::detail::spinlock::scoped_lock lock(l_);
    return p_;
  }

  void store(shared_ptr<T> r) noexcept {
    boost::detail::spinlock::scoped_lock lock(l_);
    p_.swap(r);
  }

  template <class M> void store(shared_ptr<T> r, M) noexcept {
    boost::detail::spinlock::scoped_lock lock(l_);
    p_.swap(r);
  }

  shared_ptr<T> exchange(shared_ptr<T> r) noexcept {
    {
      boost::detail::spinlock::scoped_lock lock(l_);
      p_.swap(r);
    }

    return std::move(r);
  }

  template <class M> shared_ptr<T> exchange(shared_ptr<T> r, M) noexcept {
    {
      boost::detail::spinlock::scoped_lock lock(l_);
      p_.swap(r);
    }

    return std::move(r);
  }

  template <class M>
  bool compare_exchange_weak(shared_ptr<T> &v, const shared_ptr<T> &w, M, M) noexcept {
    return compare_exchange(v, w);
  }

  template <class M>
  bool compare_exchange_weak(shared_ptr<T> &v, const shared_ptr<T> &w, M) noexcept {
    return compare_exchange(v, w);
  }

  bool compare_exchange_weak(shared_ptr<T> &v, const shared_ptr<T> &w) noexcept {
    return compare_exchange(v, w);
  }

  template <class M>
  bool compare_exchange_strong(shared_ptr<T> &v, const shared_ptr<T> &w, M, M) noexcept {
    return compare_exchange(v, w);
  }

  template <class M>
  bool compare_exchange_strong(shared_ptr<T> &v, const shared_ptr<T> &w, M) noexcept {
    return compare_exchange(v, w);
  }

  bool compare_exchange_strong(shared_ptr<T> &v, const shared_ptr<T> &w) noexcept {
    return compare_exchange(v, w);
  }

  template <class M>
  bool compare_exchange_weak(shared_ptr<T> &v, shared_ptr<T> &&w, M, M) noexcept {
    return compare_exchange(v, std::move(w));
  }

  template <class M>
  bool compare_exchange_weak(shared_ptr<T> &v, shared_ptr<T> &&w, M) noexcept {
    return compare_exchange(v, std::move(w));
  }

  bool compare_exchange_weak(shared_ptr<T> &v, shared_ptr<T> &&w) noexcept {
    return compare_exchange(v, std::move(w));
  }

  template <class M>
  bool compare_exchange_strong(shared_ptr<T> &v, shared_ptr<T> &&w, M, M) noexcept {
    return compare_exchange(v, std::move(w));
  }

  template <class M>
  bool compare_exchange_strong(shared_ptr<T> &v, shared_ptr<T> &&w, M) noexcept {
    return compare_exchange(v, std::move(w));
  }

  bool compare_exchange_strong(shared_ptr<T> &v, shared_ptr<T> &&w) noexcept {
    return compare_exchange(v, std::move(w));
  }
};

} // namespace boost

/* @file_end smart_ptr/atomic_shared_ptr.hpp */

/* @file_start smart_ptr/local_shared_ptr.hpp */

namespace boost {

template <class T> class local_shared_ptr;

namespace detail {

template <class E, class Y>
inline void lsp_pointer_construct(boost::local_shared_ptr<E> * /*ppx*/,
                                  Y *p,
                                  boost::detail::local_counted_base *&pn) {
  boost::detail::sp_assert_convertible<Y, E>();

  typedef boost::detail::local_sp_deleter<boost::checked_deleter<Y>> D;

  boost::shared_ptr<E> p2(p, D());

  D *pd = static_cast<D *>(p2._internal_get_untyped_deleter());

  pd->pn_ = p2._internal_count();

  pn = pd;
}

template <class E, class Y>
inline void lsp_pointer_construct(boost::local_shared_ptr<E[]> * /*ppx*/,
                                  Y *p,
                                  boost::detail::local_counted_base *&pn) {
  boost::detail::sp_assert_convertible<Y[], E[]>();
  typedef boost::detail::local_sp_deleter<boost::checked_array_deleter<E>> D;
  boost::shared_ptr<E[]> p2(p, D());
  D *pd = static_cast<D *>(p2._internal_get_untyped_deleter());
  pd->pn_ = p2._internal_count();
  pn = pd;
}

template <class E, std::size_t N, class Y>
inline void lsp_pointer_construct(boost::local_shared_ptr<E[N]> * /*ppx*/,
                                  Y *p,
                                  boost::detail::local_counted_base *&pn) {
  boost::detail::sp_assert_convertible<Y[N], E[N]>();
  typedef boost::detail::local_sp_deleter<boost::checked_array_deleter<E>> D;
  boost::shared_ptr<E[N]> p2(p, D());
  D *pd = static_cast<D *>(p2._internal_get_untyped_deleter());
  pd->pn_ = p2._internal_count();
  pn = pd;
}

template <class E, class P, class D>
inline void lsp_deleter_construct(boost::local_shared_ptr<E> * /*ppx*/,
                                  P p,
                                  D const &d,
                                  boost::detail::local_counted_base *&pn) {
  typedef boost::detail::local_sp_deleter<D> D2;
  boost::shared_ptr<E> p2(p, D2(d));
  D2 *pd = static_cast<D2 *>(p2._internal_get_untyped_deleter());
  pd->pn_ = p2._internal_count();
  pn = pd;
}

template <class E, class P, class D, class A>
inline void lsp_allocator_construct(boost::local_shared_ptr<E> * /*ppx*/,
                                    P p,
                                    D const &d,
                                    A const &a,
                                    boost::detail::local_counted_base *&pn) {
  typedef boost::detail::local_sp_deleter<D> D2;
  boost::shared_ptr<E> p2(p, D2(d), a);
  D2 *pd = static_cast<D2 *>(p2._internal_get_untyped_deleter());
  pd->pn_ = p2._internal_count();
  pn = pd;
}

struct lsp_internal_constructor_tag {};

} // namespace detail

//
// local_shared_ptr
//
// as shared_ptr, but local to a thread.
// reference count manipulations are non-atomic.
//

template <class T> class local_shared_ptr {
private:
  typedef local_shared_ptr this_type;

public:
  typedef typename boost::detail::sp_element<T>::type element_type;

private:
  element_type *px;
  boost::detail::local_counted_base *pn;

  template <class Y> friend class local_shared_ptr;

public:
  // destructor

  ~local_shared_ptr() noexcept {
    if (pn) { pn->release(); }
  }

  // constructors

  constexpr local_shared_ptr() noexcept : px(0), pn(0) {}

  constexpr local_shared_ptr(std::nullptr_t) noexcept : px(0), pn(0) {}

  // internal constructor, used by make_shared
  constexpr local_shared_ptr(boost::detail::lsp_internal_constructor_tag,
                             element_type *px_,
                             boost::detail::local_counted_base *pn_) noexcept
      : px(px_), pn(pn_) {}

  template <class Y> explicit local_shared_ptr(Y *p) : px(p), pn(0) {
    boost::detail::lsp_pointer_construct(this, p, pn);
  }

  template <class Y, class D> local_shared_ptr(Y *p, D d) : px(p), pn(0) {
    boost::detail::lsp_deleter_construct(this, p, d, pn);
  }

  template <class D> local_shared_ptr(std::nullptr_t p, D d) : px(p), pn(0) {
    boost::detail::lsp_deleter_construct(this, p, d, pn);
  }

  template <class Y, class D, class A> local_shared_ptr(Y *p, D d, A a) : px(p), pn(0) {
    boost::detail::lsp_allocator_construct(this, p, d, a, pn);
  }

  template <class D, class A> local_shared_ptr(std::nullptr_t p, D d, A a) : px(p), pn(0) {
    boost::detail::lsp_allocator_construct(this, p, d, a, pn);
  }

  // construction from shared_ptr

  template <class Y>
  local_shared_ptr(
      shared_ptr<Y> const &r,
      typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty())
      : px(r.get()), pn(0) {
    boost::detail::sp_assert_convertible<Y, T>();

    if (r.use_count() != 0) {
      pn = new boost::detail::local_counted_impl(r._internal_count());
    }
  }

  template <class Y>
  local_shared_ptr(
      shared_ptr<Y> &&r,
      typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty())
      : px(r.get()), pn(0) {
    boost::detail::sp_assert_convertible<Y, T>();

    if (r.use_count() != 0) {
      pn = new boost::detail::local_counted_impl(r._internal_count());
      r.reset();
    }
  }

  // construction from unique_ptr

  template <class Y, class D>
  local_shared_ptr(
      std::unique_ptr<Y, D> &&r,
      typename boost::detail::sp_enable_if_convertible<Y, T>::type = boost::detail::sp_empty())
      : px(r.get()), pn(0) {
    boost::detail::sp_assert_convertible<Y, T>();

    if (px) {
      pn =
          new boost::detail::local_counted_impl(shared_ptr<T>(std::move(r))._internal_count());
    }
  }

  template <class Y, class D> local_shared_ptr(boost::movelib::unique_ptr<Y, D> r); // !
  //  : px( r.get() ), pn( new boost::detail::local_counted_impl( shared_ptr<T>(
  //  std::move(r) ) ) )
  //{
  //    boost::detail::sp_assert_convertible< Y, T >();
  //}

  // copy constructor

  local_shared_ptr(local_shared_ptr const &r) noexcept : px(r.px), pn(r.pn) {
    if (pn) { pn->add_ref(); }
  }

  // move constructor

  local_shared_ptr(local_shared_ptr &&r) noexcept : px(r.px), pn(r.pn) {
    r.px = 0;
    r.pn = 0;
  }

  // converting copy constructor

  template <class Y>
  local_shared_ptr(local_shared_ptr<Y> const &r,
                   typename boost::detail::sp_enable_if_convertible<Y, T>::type =
                       boost::detail::sp_empty()) noexcept
      : px(r.px), pn(r.pn) {
    boost::detail::sp_assert_convertible<Y, T>();

    if (pn) { pn->add_ref(); }
  }

  // converting move constructor

  template <class Y>
  local_shared_ptr(local_shared_ptr<Y> &&r,
                   typename boost::detail::sp_enable_if_convertible<Y, T>::type =
                       boost::detail::sp_empty()) noexcept
      : px(r.px), pn(r.pn) {
    boost::detail::sp_assert_convertible<Y, T>();

    r.px = 0;
    r.pn = 0;
  }

  // aliasing

  template <class Y>
  local_shared_ptr(local_shared_ptr<Y> const &r, element_type *p) noexcept : px(p), pn(r.pn) {
    if (pn) { pn->add_ref(); }
  }

  template <class Y>
  local_shared_ptr(local_shared_ptr<Y> &&r, element_type *p) noexcept : px(p), pn(r.pn) {
    r.px = 0;
    r.pn = 0;
  }

  // assignment

  local_shared_ptr &operator=(local_shared_ptr const &r) noexcept {
    local_shared_ptr(r).swap(*this);
    return *this;
  }

  template <class Y> local_shared_ptr &operator=(local_shared_ptr<Y> const &r) noexcept {
    local_shared_ptr(r).swap(*this);
    return *this;
  }

  local_shared_ptr &operator=(local_shared_ptr &&r) noexcept {
    local_shared_ptr(std::move(r)).swap(*this);
    return *this;
  }

  template <class Y> local_shared_ptr &operator=(local_shared_ptr<Y> &&r) noexcept {
    local_shared_ptr(std::move(r)).swap(*this);
    return *this;
  }

  local_shared_ptr &operator=(std::nullptr_t) noexcept {
    local_shared_ptr().swap(*this);
    return *this;
  }

  template <class Y, class D> local_shared_ptr &operator=(std::unique_ptr<Y, D> &&r) {
    local_shared_ptr(std::move(r)).swap(*this);
    return *this;
  }

  template <class Y, class D>
  local_shared_ptr &operator=(boost::movelib::unique_ptr<Y, D> r); // !

  // reset

  void reset() noexcept { local_shared_ptr().swap(*this); }

  template <class Y>
  void reset(Y *p) // Y must be complete
  {
    local_shared_ptr(p).swap(*this);
  }

  template <class Y, class D> void reset(Y *p, D d) { local_shared_ptr(p, d).swap(*this); }

  template <class Y, class D, class A> void reset(Y *p, D d, A a) {
    local_shared_ptr(p, d, a).swap(*this);
  }

  template <class Y> void reset(local_shared_ptr<Y> const &r, element_type *p) noexcept {
    local_shared_ptr(r, p).swap(*this);
  }

  template <class Y> void reset(local_shared_ptr<Y> &&r, element_type *p) noexcept {
    local_shared_ptr(std::move(r), p).swap(*this);
  }

  // accessors

  typename boost::detail::sp_dereference<T>::type operator*() const noexcept { return *px; }

  typename boost::detail::sp_member_access<T>::type operator->() const noexcept { return px; }

  typename boost::detail::sp_array_access<T>::type
  operator[](std::ptrdiff_t i) const noexcept {
    assert(px != 0);
    assert(i >= 0 && (i < boost::detail::sp_extent<T>::value ||
                      boost::detail::sp_extent<T>::value == 0));

    return static_cast<typename boost::detail::sp_array_access<T>::type>(px[i]);
  }

  element_type *get() const noexcept { return px; }

  // implicit conversion to "bool"

  explicit operator bool() const noexcept { return px != 0; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  long local_use_count() const noexcept { return pn ? pn->local_use_count() : 0; }

  // conversions to shared_ptr, weak_ptr

  template <class Y, class E = typename boost::detail::sp_enable_if_convertible<T, Y>::type>
  operator shared_ptr<Y>() const noexcept {
    boost::detail::sp_assert_convertible<T, Y>();

    if (pn) {
      return shared_ptr<Y>(
          boost::detail::sp_internal_constructor_tag(), px, pn->local_cb_get_shared_count());
    } else {
      return shared_ptr<Y>();
    }
  }

  template <class Y, class E = typename boost::detail::sp_enable_if_convertible<T, Y>::type>
  operator weak_ptr<Y>() const noexcept {
    boost::detail::sp_assert_convertible<T, Y>();

    if (pn) {
      return shared_ptr<Y>(
          boost::detail::sp_internal_constructor_tag(), px, pn->local_cb_get_shared_count());
    } else {
      return weak_ptr<Y>();
    }
  }

  // swap

  void swap(local_shared_ptr &r) noexcept {
    std::swap(px, r.px);
    std::swap(pn, r.pn);
  }

  // owner_before

  template <class Y> bool owner_before(local_shared_ptr<Y> const &r) const noexcept {
    return std::less<boost::detail::local_counted_base *>()(pn, r.pn);
  }

  // owner_equals

  template <class Y> bool owner_equals(local_shared_ptr<Y> const &r) const noexcept {
    return pn == r.pn;
  }
};

template <class T, class U>
inline bool operator==(local_shared_ptr<T> const &a, local_shared_ptr<U> const &b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(local_shared_ptr<T> const &a, local_shared_ptr<U> const &b) noexcept {
  return a.get() != b.get();
}

template <class T>
inline bool operator==(local_shared_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() == 0;
}

template <class T>
inline bool operator==(std::nullptr_t, local_shared_ptr<T> const &p) noexcept {
  return p.get() == 0;
}

template <class T>
inline bool operator!=(local_shared_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() != 0;
}

template <class T>
inline bool operator!=(std::nullptr_t, local_shared_ptr<T> const &p) noexcept {
  return p.get() != 0;
}

template <class T, class U>
inline bool operator==(local_shared_ptr<T> const &a, shared_ptr<U> const &b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(local_shared_ptr<T> const &a, shared_ptr<U> const &b) noexcept {
  return a.get() != b.get();
}

template <class T, class U>
inline bool operator==(shared_ptr<T> const &a, local_shared_ptr<U> const &b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(shared_ptr<T> const &a, local_shared_ptr<U> const &b) noexcept {
  return a.get() != b.get();
}

template <class T, class U>
inline bool operator<(local_shared_ptr<T> const &a, local_shared_ptr<U> const &b) noexcept {
  return a.owner_before(b);
}

template <class T> inline void swap(local_shared_ptr<T> &a, local_shared_ptr<T> &b) noexcept {
  a.swap(b);
}

template <class T, class U>
local_shared_ptr<T> static_pointer_cast(local_shared_ptr<U> const &r) noexcept {
  (void)static_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = static_cast<E *>(r.get());
  return local_shared_ptr<T>(r, p);
}

template <class T, class U>
local_shared_ptr<T> const_pointer_cast(local_shared_ptr<U> const &r) noexcept {
  (void)const_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = const_cast<E *>(r.get());
  return local_shared_ptr<T>(r, p);
}

template <class T, class U>
local_shared_ptr<T> dynamic_pointer_cast(local_shared_ptr<U> const &r) noexcept {
  (void)dynamic_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = dynamic_cast<E *>(r.get());
  return p ? local_shared_ptr<T>(r, p) : local_shared_ptr<T>();
}

template <class T, class U>
local_shared_ptr<T> reinterpret_pointer_cast(local_shared_ptr<U> const &r) noexcept {
  (void)reinterpret_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = reinterpret_cast<E *>(r.get());
  return local_shared_ptr<T>(r, p);
}

template <class T, class U>
local_shared_ptr<T> static_pointer_cast(local_shared_ptr<U> &&r) noexcept {
  (void)static_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = static_cast<E *>(r.get());
  return local_shared_ptr<T>(std::move(r), p);
}

template <class T, class U>
local_shared_ptr<T> const_pointer_cast(local_shared_ptr<U> &&r) noexcept {
  (void)const_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = const_cast<E *>(r.get());
  return local_shared_ptr<T>(std::move(r), p);
}

template <class T, class U>
local_shared_ptr<T> dynamic_pointer_cast(local_shared_ptr<U> &&r) noexcept {
  (void)dynamic_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = dynamic_cast<E *>(r.get());
  return p ? local_shared_ptr<T>(std::move(r), p) : local_shared_ptr<T>();
}

template <class T, class U>
local_shared_ptr<T> reinterpret_pointer_cast(local_shared_ptr<U> &&r) noexcept {
  (void)reinterpret_cast<T *>(static_cast<U *>(0));

  typedef typename local_shared_ptr<T>::element_type E;

  E *p = reinterpret_cast<E *>(r.get());
  return local_shared_ptr<T>(std::move(r), p);
}

// get_pointer() enables boost::mem_fn to recognize local_shared_ptr

template <class T>
inline typename local_shared_ptr<T>::element_type *
get_pointer(local_shared_ptr<T> const &p) noexcept {
  return p.get();
}

// operator<<

template <class E, class T, class Y>
std::basic_ostream<E, T> &operator<<(std::basic_ostream<E, T> &os,
                                     local_shared_ptr<Y> const &p) {
  os << p.get();
  return os;
}

// get_deleter

template <class D, class T> D *get_deleter(local_shared_ptr<T> const &p) noexcept {
  return get_deleter<D>(shared_ptr<T>(p));
}

// hash_value

template <class T> struct hash;

template <class T> std::size_t hash_value(local_shared_ptr<T> const &p) noexcept {
  return boost::hash<typename local_shared_ptr<T>::element_type *>()(p.get());
}

} // namespace boost

// std::hash

namespace std {

template <class T> struct hash<::boost::local_shared_ptr<T>> {
  std::size_t operator()(::boost::local_shared_ptr<T> const &p) const noexcept {
    return std::hash<typename ::boost::local_shared_ptr<T>::element_type *>()(p.get());
  }
};

} // namespace std

/* @file_end smart_ptr/local_shared_ptr.hpp */

/* @file_start smart_ptr/shared_array.hpp */

// for broken compiler workarounds

#include <memory>     // TR1 cyclic inclusion fix
#include <algorithm>  // for std::swap
#include <cstddef>    // for std::ptrdiff_t
#include <functional> // for std::less

namespace boost {

//
//  shared_array
//
//  shared_array extends shared_ptr to arrays.
//  The array pointed to is deleted when the last shared_array pointing to it
//  is destroyed or reset.
//

template <class T> class shared_array {
private:
  // Borland 5.5.1 specific workarounds
  typedef checked_array_deleter<T> deleter;
  typedef shared_array<T> this_type;

public:
  typedef T element_type;

  shared_array() noexcept : px(0), pn() {}

  shared_array(std::nullptr_t) noexcept : px(0), pn() {}

  template <class Y> explicit shared_array(Y *p) : px(p), pn(p, checked_array_deleter<Y>()) {
    boost::detail::sp_assert_convertible<Y[], T[]>();
  }

  //
  // Requirements: D's copy constructor must not throw
  //
  // shared_array will release p by calling d(p)
  //

  template <class Y, class D> shared_array(Y *p, D d) : px(p), pn(p, d) {
    boost::detail::sp_assert_convertible<Y[], T[]>();
  }

  // As above, but with allocator. A's copy constructor shall not throw.

  template <class Y, class D, class A> shared_array(Y *p, D d, A a) : px(p), pn(p, d, a) {
    boost::detail::sp_assert_convertible<Y[], T[]>();
  }

  //  generated copy constructor, destructor are fine...

  // ... except in C++0x, move disables the implicit copy

  shared_array(shared_array const &r) noexcept : px(r.px), pn(r.pn) {}

  shared_array(shared_array &&r) noexcept : px(r.px), pn() {
    pn.swap(r.pn);
    r.px = 0;
  }

  // conversion

  template <class Y>
  shared_array(shared_array<Y> const &r,
               typename boost::detail::sp_enable_if_convertible<Y[], T[]>::type =
                   boost::detail::sp_empty()) noexcept
      : px(r.px), pn(r.pn) {
    boost::detail::sp_assert_convertible<Y[], T[]>();
  }

  // aliasing

  template <class Y>
  shared_array(shared_array<Y> const &r, element_type *p) noexcept : px(p), pn(r.pn) {}

  // assignment

  shared_array &operator=(shared_array const &r) noexcept {
    this_type(r).swap(*this);
    return *this;
  }

#if !defined(_MSC_VER) || (_MSC_VER >= 1400)

  template <class Y> shared_array &operator=(shared_array<Y> const &r) noexcept {
    this_type(r).swap(*this);
    return *this;
  }

#endif

  shared_array &operator=(shared_array &&r) noexcept {
    this_type(static_cast<shared_array &&>(r)).swap(*this);
    return *this;
  }

  template <class Y> shared_array &operator=(shared_array<Y> &&r) noexcept {
    this_type(static_cast<shared_array<Y> &&>(r)).swap(*this);
    return *this;
  }

  void reset() noexcept { this_type().swap(*this); }

  template <class Y>
  void reset(Y *p)             // Y must be complete
  {
    assert(p == 0 || p != px); // catch self-reset errors
    this_type(p).swap(*this);
  }

  template <class Y, class D> void reset(Y *p, D d) { this_type(p, d).swap(*this); }

  template <class Y, class D, class A> void reset(Y *p, D d, A a) {
    this_type(p, d, a).swap(*this);
  }

  template <class Y> void reset(shared_array<Y> const &r, element_type *p) noexcept {
    this_type(r, p).swap(*this);
  }

  T &operator[](std::ptrdiff_t i) const noexcept {
    assert(px != 0);
    assert(i >= 0);
    return px[i];
  }

  T *get() const noexcept { return px; }

  // implicit conversion to "bool"
  explicit operator bool() const noexcept { return px != 0; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  bool unique() const noexcept { return pn.unique(); }

  long use_count() const noexcept { return pn.use_count(); }

  void swap(shared_array<T> &other) noexcept {
    std::swap(px, other.px);
    pn.swap(other.pn);
  }

  void *_internal_get_deleter(std::type_info const &ti) const noexcept {
    return pn.get_deleter(ti);
  }

private:
  template <class Y> friend class shared_array;

  T *px;                   // contained pointer
  detail::shared_count pn; // reference counter

};                         // shared_array

template <class T>
inline bool operator==(shared_array<T> const &a, shared_array<T> const &b) noexcept {
  return a.get() == b.get();
}

template <class T>
inline bool operator!=(shared_array<T> const &a, shared_array<T> const &b) noexcept {
  return a.get() != b.get();
}

template <class T> inline bool operator==(shared_array<T> const &p, std::nullptr_t) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator==(std::nullptr_t, shared_array<T> const &p) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator!=(shared_array<T> const &p, std::nullptr_t) noexcept {
  return p.get() != 0;
}

template <class T> inline bool operator!=(std::nullptr_t, shared_array<T> const &p) noexcept {
  return p.get() != 0;
}

template <class T>
inline bool operator<(shared_array<T> const &a, shared_array<T> const &b) noexcept {
  return std::less<T *>()(a.get(), b.get());
}

template <class T> void swap(shared_array<T> &a, shared_array<T> &b) noexcept { a.swap(b); }

template <class D, class T> D *get_deleter(shared_array<T> const &p) noexcept {
  return static_cast<D *>(p._internal_get_deleter(typeid(D)));
}

} // namespace boost

/* @file_end smart_ptr/shared_array.hpp */

/* @file_start smart_ptr/intrusive_ptr.hpp */

#include <iosfwd> // for std::basic_ostream

namespace boost {

//
//  intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != 0)
//
//  The object is responsible for destroying itself.
//

template <class T> class intrusive_ptr {
private:
  typedef intrusive_ptr this_type;

public:
  typedef T element_type;

  constexpr intrusive_ptr() noexcept : px(0) {}

  intrusive_ptr(T *p, bool add_ref = true) : px(p) {
    if (px != 0 && add_ref) intrusive_ptr_add_ref(px);
  }

  template <class U>
  intrusive_ptr(
      intrusive_ptr<U> const &rhs,
      typename boost::detail::sp_enable_if_convertible<U, T>::type = boost::detail::sp_empty())
      : px(rhs.get()) {
    if (px != 0) intrusive_ptr_add_ref(px);
  }

  intrusive_ptr(intrusive_ptr const &rhs) : px(rhs.px) {
    if (px != 0) intrusive_ptr_add_ref(px);
  }

  ~intrusive_ptr() {
    if (px != 0) intrusive_ptr_release(px);
  }

  template <class U> intrusive_ptr &operator=(intrusive_ptr<U> const &rhs) {
    this_type(rhs).swap(*this);
    return *this;
  }

  // Move support

  intrusive_ptr(intrusive_ptr &&rhs) noexcept : px(rhs.px) { rhs.px = 0; }

  intrusive_ptr &operator=(intrusive_ptr &&rhs) noexcept {
    this_type(static_cast<intrusive_ptr &&>(rhs)).swap(*this);
    return *this;
  }

  template <class U> friend class intrusive_ptr;

  template <class U>
  intrusive_ptr(
      intrusive_ptr<U> &&rhs,
      typename boost::detail::sp_enable_if_convertible<U, T>::type = boost::detail::sp_empty())
      : px(rhs.px) {
    rhs.px = 0;
  }

  template <class U> intrusive_ptr &operator=(intrusive_ptr<U> &&rhs) noexcept {
    this_type(static_cast<intrusive_ptr<U> &&>(rhs)).swap(*this);
    return *this;
  }

  intrusive_ptr &operator=(intrusive_ptr const &rhs) {
    this_type(rhs).swap(*this);
    return *this;
  }

  intrusive_ptr &operator=(T *rhs) {
    this_type(rhs).swap(*this);
    return *this;
  }

  void reset() { this_type().swap(*this); }

  void reset(T *rhs) { this_type(rhs).swap(*this); }

  void reset(T *rhs, bool add_ref) { this_type(rhs, add_ref).swap(*this); }

  T *get() const noexcept { return px; }

  T *detach() noexcept {
    T *ret = px;
    px = 0;
    return ret;
  }

  T &operator*() const noexcept {
    assert(px != 0);
    return *px;
  }

  T *operator->() const noexcept {
    assert(px != 0);
    return px;
  }

  // implicit conversion to "bool"
  explicit operator bool() const noexcept { return px != 0; }

  // operator! is redundant, but some compilers need it
  bool operator!() const noexcept { return px == 0; }

  void swap(intrusive_ptr &rhs) noexcept {
    T *tmp = px;
    px = rhs.px;
    rhs.px = tmp;
  }

private:
  T *px;
};

template <class T, class U>
inline bool operator==(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() == b.get();
}

template <class T, class U>
inline bool operator!=(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() != b.get();
}

template <class T, class U> inline bool operator==(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() == b;
}

template <class T, class U> inline bool operator!=(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() != b;
}

template <class T, class U> inline bool operator==(T *a, intrusive_ptr<U> const &b) noexcept {
  return a == b.get();
}

template <class T, class U> inline bool operator!=(T *a, intrusive_ptr<U> const &b) noexcept {
  return a != b.get();
}

#if defined(__GNUC__) && __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template <class T>
inline bool operator!=(intrusive_ptr<T> const &a, intrusive_ptr<T> const &b) noexcept {
  return a.get() != b.get();
}

#endif

template <class T> inline bool operator==(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator==(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() == 0;
}

template <class T> inline bool operator!=(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() != 0;
}

template <class T> inline bool operator!=(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() != 0;
}

template <class T>
inline bool operator<(intrusive_ptr<T> const &a, intrusive_ptr<T> const &b) noexcept {
  return std::less<T *>()(a.get(), b.get());
}

template <class T> void swap(intrusive_ptr<T> &lhs, intrusive_ptr<T> &rhs) noexcept {
  lhs.swap(rhs);
}

// mem_fn support

template <class T> T *get_pointer(intrusive_ptr<T> const &p) noexcept { return p.get(); }

// pointer casts

template <class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const &p) {
  return static_cast<T *>(p.get());
}

template <class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const &p) {
  return const_cast<T *>(p.get());
}

template <class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const &p) {
  return dynamic_cast<T *>(p.get());
}

template <class T, class U>
intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> &&p) noexcept {
  return intrusive_ptr<T>(static_cast<T *>(p.detach()), false);
}

template <class T, class U>
intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> &&p) noexcept {
  return intrusive_ptr<T>(const_cast<T *>(p.detach()), false);
}

template <class T, class U>
intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> &&p) noexcept {
  T *p2 = dynamic_cast<T *>(p.get());

  intrusive_ptr<T> r(p2, false);

  if (p2) p.detach();

  return r;
}

// operator<<

template <class E, class T, class Y>
std::basic_ostream<E, T> &operator<<(std::basic_ostream<E, T> &os, intrusive_ptr<Y> const &p) {
  os << p.get();
  return os;
}

// hash_value

template <class T> struct hash;

template <class T> std::size_t hash_value(boost::intrusive_ptr<T> const &p) noexcept {
  return boost::hash<T *>()(p.get());
}

} // namespace boost

// std::hash

namespace std {

template <class T> struct hash<::boost::intrusive_ptr<T>> {
  std::size_t operator()(::boost::intrusive_ptr<T> const &p) const noexcept {
    return std::hash<T *>()(p.get());
  }
};

} // namespace std

/* @file_end smart_ptr/intrusive_ptr.hpp */

/* @file_start smart_ptr/weak_ptr.hpp */

namespace boost {

template <class T> class weak_ptr {
private:
  // Borland 5.5.1 specific workarounds
  typedef weak_ptr<T> this_type;

public:
  typedef typename boost::detail::sp_element<T>::type element_type;

  constexpr weak_ptr() noexcept : px(0), pn() {}

  //  generated copy constructor, assignment, destructor are fine...

  // ... except in C++0x, move disables the implicit copy

  weak_ptr(weak_ptr const &r) noexcept : px(r.px), pn(r.pn) {}

  weak_ptr &operator=(weak_ptr const &r) noexcept {
    px = r.px;
    pn = r.pn;
    return *this;
  }

  //
  //  The "obvious" converting constructor implementation:
  //
  //  template<class Y>
  //  weak_ptr(weak_ptr<Y> const & r): px(r.px), pn(r.pn)
  //  {
  //  }
  //
  //  has a serious problem.
  //
  //  r.px may already have been invalidated. The px(r.px)
  //  conversion may require access to *r.px (virtual inheritance).
  //
  //  It is not possible to avoid spurious access violations since
  //  in multithreaded programs r.px may be invalidated at any point.
  //

  template <class Y>
  weak_ptr(weak_ptr<Y> const &r,
           typename boost::detail::sp_enable_if_convertible<Y, T>::type =
               boost::detail::sp_empty()) noexcept
      : px(r.lock().get()), pn(r.pn) {
    boost::detail::sp_assert_convertible<Y, T>();
  }

  template <class Y>
  weak_ptr(weak_ptr<Y> &&r,
           typename boost::detail::sp_enable_if_convertible<Y, T>::type =
               boost::detail::sp_empty()) noexcept
      : px(r.lock().get()), pn(static_cast<boost::detail::weak_count &&>(r.pn)) {
    boost::detail::sp_assert_convertible<Y, T>();
    r.px = 0;
  }

  // for better efficiency in the T == Y case
  weak_ptr(weak_ptr &&r) noexcept
      : px(r.px), pn(static_cast<boost::detail::weak_count &&>(r.pn)) {
    r.px = 0;
  }

  // for better efficiency in the T == Y case
  weak_ptr &operator=(weak_ptr &&r) noexcept {
    this_type(static_cast<weak_ptr &&>(r)).swap(*this);
    return *this;
  }

  template <class Y>
  weak_ptr(shared_ptr<Y> const &r,
           typename boost::detail::sp_enable_if_convertible<Y, T>::type =
               boost::detail::sp_empty()) noexcept
      : px(r.px), pn(r.pn) {
    boost::detail::sp_assert_convertible<Y, T>();
  }

  // aliasing
  template <class Y>
  weak_ptr(shared_ptr<Y> const &r, element_type *p) noexcept : px(p), pn(r.pn) {}

  template <class Y>
  weak_ptr(weak_ptr<Y> const &r, element_type *p) noexcept : px(p), pn(r.pn) {}

  template <class Y>
  weak_ptr(weak_ptr<Y> &&r, element_type *p) noexcept : px(p), pn(std::move(r.pn)) {}

#if !defined(_MSC_VER) || (_MSC_VER >= 1300)

  template <class Y> weak_ptr &operator=(weak_ptr<Y> const &r) noexcept {
    boost::detail::sp_assert_convertible<Y, T>();

    px = r.lock().get();
    pn = r.pn;

    return *this;
  }

  template <class Y> weak_ptr &operator=(weak_ptr<Y> &&r) noexcept {
    this_type(static_cast<weak_ptr<Y> &&>(r)).swap(*this);
    return *this;
  }

  template <class Y> weak_ptr &operator=(shared_ptr<Y> const &r) noexcept {
    boost::detail::sp_assert_convertible<Y, T>();

    px = r.px;
    pn = r.pn;

    return *this;
  }

#endif

  shared_ptr<T> lock() const noexcept {
    return shared_ptr<T>(*this, boost::detail::sp_nothrow_tag());
  }

  long use_count() const noexcept { return pn.use_count(); }

  bool expired() const noexcept { return pn.use_count() == 0; }

  bool _empty() const noexcept // extension, not in std::weak_ptr
  {
    return pn.empty();
  }

  bool empty() const noexcept // extension, not in std::weak_ptr
  {
    return pn.empty();
  }

  void reset() noexcept { this_type().swap(*this); }

  void swap(this_type &other) noexcept {
    std::swap(px, other.px);
    pn.swap(other.pn);
  }

  template <class Y> bool owner_before(weak_ptr<Y> const &rhs) const noexcept {
    return pn < rhs.pn;
  }

  template <class Y> bool owner_before(shared_ptr<Y> const &rhs) const noexcept {
    return pn < rhs.pn;
  }

  template <class Y> bool owner_equals(weak_ptr<Y> const &rhs) const noexcept {
    return pn == rhs.pn;
  }

  template <class Y> bool owner_equals(shared_ptr<Y> const &rhs) const noexcept {
    return pn == rhs.pn;
  }

  std::size_t owner_hash_value() const noexcept { return pn.hash_value(); }

  // Tasteless as this may seem, making all members public allows member
  // templates to work in the absence of member template friends. (Matthew
  // Langston)

private:
  template <class Y> friend class weak_ptr;

  template <class Y> friend class shared_ptr;

  element_type *px;             // contained pointer
  boost::detail::weak_count pn; // reference counter

};                              // weak_ptr

template <class T, class U>
inline bool operator<(weak_ptr<T> const &a, weak_ptr<U> const &b) noexcept {
  return a.owner_before(b);
}

template <class T> void swap(weak_ptr<T> &a, weak_ptr<T> &b) noexcept { a.swap(b); }

#if defined(__cpp_deduction_guides)

template <class T> weak_ptr(shared_ptr<T>) -> weak_ptr<T>;

#endif

// hash_value

template <class T> std::size_t hash_value(boost::weak_ptr<T> const &p) noexcept {
  return p.owner_hash_value();
}

} // namespace boost

// std::hash, std::equal_to

namespace std {

template <class T> struct hash<::boost::weak_ptr<T>> {
  std::size_t operator()(::boost::weak_ptr<T> const &p) const noexcept {
    return p.owner_hash_value();
  }
};

template <class T> struct equal_to<::boost::weak_ptr<T>> {
  bool operator()(::boost::weak_ptr<T> const &a,
                  ::boost::weak_ptr<T> const &b) const noexcept {
    return a.owner_equals(b);
  }
};

} // namespace std

/* @file_end smart_ptr/weak_ptr.hpp */

/* @file_start boost/type_triats/enable_if.hpp */

namespace boost {

template <bool B, class T = void> struct enable_if_ {
  typedef T type;
};

template <class T> struct enable_if_<false, T> {};

template <bool B, class T = void> using enable_if_t = typename enable_if_<B, T>::type;
} // namespace boost

/* @file_end boost/type_triats/enable_if.hpp */

/* @file_start smart_ptr/enable_shared_from.hpp */

/* @file_start smart_ptr/enable_shared_from_this.hpp */

namespace boost {

template <class T> class enable_shared_from_this {
protected:
  constexpr enable_shared_from_this() noexcept {}

  constexpr enable_shared_from_this(enable_shared_from_this const &) noexcept {}

  enable_shared_from_this &operator=(enable_shared_from_this const &) noexcept {
    return *this;
  }

  ~enable_shared_from_this() noexcept // ~weak_ptr<T> newer throws, so
  // this call also must not throw
  {}

public:
  shared_ptr<T> shared_from_this() {
    shared_ptr<T> p(weak_this_);
    assert(p.get() == this);
    return p;
  }

  shared_ptr<T const> shared_from_this() const {
    shared_ptr<T const> p(weak_this_);
    assert(p.get() == this);
    return p;
  }

  weak_ptr<T> weak_from_this() noexcept { return weak_this_; }

  weak_ptr<T const> weak_from_this() const noexcept { return weak_this_; }

public: // actually private, but avoids compiler template friendship issues
  // Note: invoked automatically by shared_ptr; do not call
  template <class X, class Y>
  void _internal_accept_owner(shared_ptr<X> const *ppx, Y *py) const noexcept {
    if (weak_this_.expired()) { weak_this_ = shared_ptr<T>(*ppx, py); }
  }

private:
  mutable weak_ptr<T> weak_this_;
};

} // namespace boost

/* @file_end smart_ptr/enable_shared_from_this.hpp */

namespace boost {

class enable_shared_from : public enable_shared_from_this<enable_shared_from> {
private:
  using enable_shared_from_this<enable_shared_from>::shared_from_this;
  using enable_shared_from_this<enable_shared_from>::weak_from_this;
};

template <class T> shared_ptr<T> shared_from(T *p) {
  return shared_ptr<T>(p->enable_shared_from_this<enable_shared_from>::shared_from_this(), p);
}

template <class T> weak_ptr<T> weak_from(T *p) noexcept {
  return weak_ptr<T>(p->enable_shared_from_this<enable_shared_from>::weak_from_this(), p);
}

} // namespace boost

/* @file_start enable_shared_from_raw.hpp */

namespace boost {
template <typename T> boost::shared_ptr<T> shared_from_raw(T *);

template <typename T> boost::weak_ptr<T> weak_from_raw(T *);

namespace detail {
template <class X, class Y>
inline void sp_enable_shared_from_this(boost::shared_ptr<X> *ppx,
                                       Y const *py,
                                       boost::enable_shared_from_raw const *pe);

} // namespace detail

class enable_shared_from_raw {
protected:
  enable_shared_from_raw() {}

  enable_shared_from_raw(enable_shared_from_raw const &) {}

  enable_shared_from_raw &operator=(enable_shared_from_raw const &) { return *this; }

  ~enable_shared_from_raw() {
    assert(shared_this_.use_count() <= 1); // make sure no dangling shared_ptr objects exist
  }

private:
  void init_if_expired() const {
    if (weak_this_.expired()) {
      shared_this_.reset(static_cast<void *>(0), detail::esft2_deleter_wrapper());
      weak_this_ = shared_this_;
    }
  }

  void init_if_empty() const {
    if (weak_this_._empty()) {
      shared_this_.reset(static_cast<void *>(0), detail::esft2_deleter_wrapper());
      weak_this_ = shared_this_;
    }
  }

private:
  template <class Y> friend class shared_ptr;

  template <typename T> friend boost::shared_ptr<T> shared_from_raw(T *);

  template <typename T> friend boost::weak_ptr<T> weak_from_raw(T *);

  template <class X, class Y>
  friend inline void detail::sp_enable_shared_from_this(
      boost::shared_ptr<X> *ppx, Y const *py, boost::enable_shared_from_raw const *pe);

  shared_ptr<void const volatile> shared_from_this() const {
    init_if_expired();
    return shared_ptr<void const volatile>(weak_this_);
  }

  shared_ptr<void const volatile> shared_from_this() const volatile {
    return const_cast<enable_shared_from_raw const *>(this)->shared_from_this();
  }

  weak_ptr<void const volatile> weak_from_this() const {
    init_if_empty();
    return weak_this_;
  }

  weak_ptr<void const volatile> weak_from_this() const volatile {
    return const_cast<enable_shared_from_raw const *>(this)->weak_from_this();
  }

  // Note: invoked automatically by shared_ptr; do not call
  template <class X, class Y> void _internal_accept_owner(shared_ptr<X> *ppx, Y *) const {
    assert(ppx != 0);

    if (weak_this_.expired()) {
      weak_this_ = *ppx;
    } else if (shared_this_.use_count() != 0) {
      assert(ppx->unique()); // no weak_ptrs should exist either, but
      // there's no way to check that

      detail::esft2_deleter_wrapper *pd =
          boost::get_deleter<detail::esft2_deleter_wrapper>(shared_this_);
      assert(pd != 0);

      pd->set_deleter(*ppx);

      ppx->reset(shared_this_, ppx->get());
      shared_this_.reset();
    }
  }

  mutable weak_ptr<void const volatile> weak_this_;

private:
  mutable shared_ptr<void const volatile> shared_this_;
};

template <typename T> boost::shared_ptr<T> shared_from_raw(T *p) {
  assert(p != 0);
  return boost::shared_ptr<T>(p->enable_shared_from_raw::shared_from_this(), p);
}

template <typename T> boost::weak_ptr<T> weak_from_raw(T *p) {
  assert(p != 0);
  boost::weak_ptr<T> result(p->enable_shared_from_raw::weak_from_this(), p);
  return result;
}

namespace detail {
template <class X, class Y>
inline void sp_enable_shared_from_this(boost::shared_ptr<X> *ppx,
                                       Y const *py,
                                       boost::enable_shared_from_raw const *pe) {
  if (pe != 0) { pe->_internal_accept_owner(ppx, const_cast<Y *>(py)); }
}
} // namespace detail

} // namespace boost

/* @file_end enable_shared_from_raw.hpp */

/* @file_end smart_ptr/enable_shared_from.hpp */

/* @file_start boost/core/first_scalar.hpp */

#include <cstddef>

namespace boost {
namespace detail {

template <class T> struct make_scalar {
  typedef T type;
};

template <class T, std::size_t N> struct make_scalar<T[N]> {
  typedef typename make_scalar<T>::type type;
};

} // namespace detail

template <class T> constexpr inline T *first_scalar(T *p) noexcept { return p; }

template <class T, std::size_t N>
constexpr inline typename detail::make_scalar<T>::type *first_scalar(T (*p)[N]) noexcept {
  return boost::first_scalar(&(*p)[0]);
}

} // namespace boost

/* @file_end boost/core/first_scalr.hpp */

/* @file_start boost/core/alloc_construct.hpp */

namespace boost {

template <class A, class T> inline void alloc_destroy(A &a, T *p) {
  boost::allocator_destroy(a, p);
}

template <class A, class T> inline void alloc_destroy_n(A &a, T *p, std::size_t n) {
  boost::allocator_destroy_n(a, p, n);
}

template <class A, class T> inline void alloc_construct(A &a, T *p) {
  boost::allocator_construct(a, p);
}

template <class A, class T, class U, class... V>
inline void alloc_construct(A &a, T *p, U &&u, V &&...v) {
  boost::allocator_construct(a, p, std::forward<U>(u), std::forward<V>(v)...);
}

template <class A, class T> inline void alloc_construct_n(A &a, T *p, std::size_t n) {
  boost::allocator_construct_n(a, p, n);
}

template <class A, class T>
inline void alloc_construct_n(A &a, T *p, std::size_t n, const T *l, std::size_t m) {
  boost::allocator_construct_n(a, p, n, l, m);
}

template <class A, class T, class I>
inline void alloc_construct_n(A &a, T *p, std::size_t n, I b) {
  boost::allocator_construct_n(a, p, n, b);
}

} // namespace boost

/* @file_end boost/core/alloc_construct.hpp */

/* @file_start smart_ptr/make_shared.hpp */

/* @file_start smart_ptr/allocate_shared_array.hpp */

namespace boost {
namespace detail {

template <class T> struct sp_array_element {
  typedef typename boost::remove_cv<typename boost::remove_extent<T>::type>::type type;
};

template <class T> struct sp_array_count {
  enum { value = 1 };
};

template <class T, std::size_t N> struct sp_array_count<T[N]> {
  enum { value = N * sp_array_count<T>::value };
};

template <std::size_t N, std::size_t M> struct sp_max_size {
  enum { value = N < M ? M : N };
};

template <std::size_t N, std::size_t M> struct sp_align_up {
  enum { value = (N + M - 1) & ~(M - 1) };
};

template <class T> constexpr inline std::size_t sp_objects(std::size_t size) noexcept {
  return (size + sizeof(T) - 1) / sizeof(T);
}

template <class A> class sp_array_state {
public:
  typedef A type;

  template <class U>
  sp_array_state(const U &_allocator, std::size_t _size) noexcept
      : allocator_(_allocator), size_(_size) {}

  A &allocator() noexcept { return allocator_; }

  std::size_t size() const noexcept { return size_; }

private:
  A allocator_;
  std::size_t size_;
};

template <class A, std::size_t N> class sp_size_array_state {
public:
  typedef A type;

  template <class U>
  sp_size_array_state(const U &_allocator, std::size_t) noexcept : allocator_(_allocator) {}

  A &allocator() noexcept { return allocator_; }

  constexpr std::size_t size() const noexcept { return N; }

private:
  A allocator_;
};

template <class T, class U> struct sp_array_alignment {
  enum {
    value = sp_max_size<boost::alignment_of<T>::value, boost::alignment_of<U>::value>::value
  };
};

template <class T, class U> struct sp_array_offset {
  enum { value = sp_align_up<sizeof(T), sp_array_alignment<T, U>::value>::value };
};

template <class U, class T> inline U *sp_array_start(T *base) noexcept {
  enum { size = sp_array_offset<T, U>::value };
  return reinterpret_cast<U *>(reinterpret_cast<char *>(base) + size);
}

template <class A, class T> class sp_array_creator {
  typedef typename A::value_type element;

  enum { offset = sp_array_offset<T, element>::value };

  typedef
      typename boost::type_with_alignment<sp_array_alignment<T, element>::value>::type type;

public:
  template <class U>
  sp_array_creator(const U &other, std::size_t size) noexcept
      : other_(other), size_(sp_objects<type>(offset + sizeof(element) * size)) {}

  T *create() { return reinterpret_cast<T *>(other_.allocate(size_)); }

  void destroy(T *base) { other_.deallocate(reinterpret_cast<type *>(base), size_); }

private:
  typename boost::allocator_rebind<A, type>::type other_;
  std::size_t size_;
};

template <class T> class SYMBOL_VISIBLE sp_array_base : public sp_counted_base {
  typedef typename T::type allocator;

public:
  typedef typename allocator::value_type type;

  template <class A>
  sp_array_base(const A &other, type *start, std::size_t size) : state_(other, size) {
    boost::alloc_construct_n(state_.allocator(),
                             boost::first_scalar(start),
                             state_.size() * sp_array_count<type>::value);
  }

  template <class A, class U>
  sp_array_base(const A &other, type *start, std::size_t size, const U &list)
      : state_(other, size) {
    enum { count = sp_array_count<type>::value };
    boost::alloc_construct_n(state_.allocator(),
                             boost::first_scalar(start),
                             state_.size() * count,
                             boost::first_scalar(&list),
                             count);
  }

  T &state() noexcept { return state_; }

  void dispose() noexcept override {
    boost::alloc_destroy_n(state_.allocator(),
                           boost::first_scalar(sp_array_start<type>(this)),
                           state_.size() * sp_array_count<type>::value);
  }

  void destroy() noexcept override {
    sp_array_creator<allocator, sp_array_base> other(state_.allocator(), state_.size());
    this->~sp_array_base();
    other.destroy(this);
  }

  void *get_deleter(const std::type_info &) noexcept override { return 0; }

  void *get_local_deleter(const std::type_info &) noexcept override { return 0; }

  void *get_untyped_deleter() noexcept override { return 0; }

private:
  T state_;
};

template <class A, class T> struct sp_array_result {
public:
  template <class U>
  sp_array_result(const U &other, std::size_t size)
      : creator_(other, size), result_(creator_.create()) {}

  ~sp_array_result() {
    if (result_) { creator_.destroy(result_); }
  }

  T *get() const noexcept { return result_; }

  void release() noexcept { result_ = 0; }

private:
  sp_array_result(const sp_array_result &);

  sp_array_result &operator=(const sp_array_result &);

  sp_array_creator<A, T> creator_;
  T *result_;
};

} // namespace detail

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T>>::type
allocate_shared(const A &allocator, std::size_t count) {
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::sp_array_state<other> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count);
  result.release();
  return shared_ptr<T>(detail::sp_internal_constructor_tag(),
                       start,
                       detail::shared_count(static_cast<detail::sp_counted_base *>(node)));
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T>>::type
allocate_shared(const A &allocator) {
  enum { count = extent<T>::value };
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::sp_size_array_state<other, extent<T>::value> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count);
  result.release();
  return shared_ptr<T>(detail::sp_internal_constructor_tag(),
                       start,
                       detail::shared_count(static_cast<detail::sp_counted_base *>(node)));
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T>>::type allocate_shared(
    const A &allocator, std::size_t count, const typename remove_extent<T>::type &value) {
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::sp_array_state<other> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count, value);
  result.release();
  return shared_ptr<T>(detail::sp_internal_constructor_tag(),
                       start,
                       detail::shared_count(static_cast<detail::sp_counted_base *>(node)));
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T>>::type
allocate_shared(const A &allocator, const typename remove_extent<T>::type &value) {
  enum { count = extent<T>::value };
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::sp_size_array_state<other, extent<T>::value> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count, value);
  result.release();
  return shared_ptr<T>(detail::sp_internal_constructor_tag(),
                       start,
                       detail::shared_count(static_cast<detail::sp_counted_base *>(node)));
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T>>::type
allocate_shared_noinit(const A &allocator, std::size_t count) {
  return boost::allocate_shared<T>(boost::noinit_adapt(allocator), count);
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T>>::type
allocate_shared_noinit(const A &allocator) {
  return boost::allocate_shared<T>(boost::noinit_adapt(allocator));
}

} // namespace boost

/* @file_end smart_ptr/allocate_shared_array.hpp */

/* @file_start smart_ptr/make_shared_array.hpp */

namespace boost {

template <class T>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T>>::type make_shared() {
  return boost::allocate_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>());
}

template <class T>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T>>::type
make_shared(const typename remove_extent<T>::type &value) {
  return boost::allocate_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), value);
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T>>::type
make_shared(std::size_t size) {
  return boost::allocate_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), size);
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T>>::type
make_shared(std::size_t size, const typename remove_extent<T>::type &value) {
  return boost::allocate_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), size, value);
}

template <class T>
inline typename enable_if_<is_bounded_array<T>::value, shared_ptr<T>>::type
make_shared_noinit() {
  return boost::allocate_shared_noinit<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>());
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, shared_ptr<T>>::type
make_shared_noinit(std::size_t size) {
  return boost::allocate_shared_noinit<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), size);
}

} // namespace boost

/* @file_end smart_ptr/make_shared_array.hpp */

/* @file_end smart_ptr/make_shared.hpp */

/* @file_start smart_ptr/allocate_local_shared_array.hpp */

namespace boost {
namespace detail {

class SYMBOL_VISIBLE lsp_array_base : public local_counted_base {
public:
  void set(sp_counted_base *base) noexcept { count_ = shared_count(base); }

  void local_cb_destroy() noexcept override { shared_count().swap(count_); }

  shared_count local_cb_get_shared_count() const noexcept override { return count_; }

private:
  shared_count count_;
};

template <class A> class lsp_array_state : public sp_array_state<A> {
public:
  template <class U>
  lsp_array_state(const U &other, std::size_t size) noexcept
      : sp_array_state<A>(other, size) {}

  lsp_array_base &base() noexcept { return base_; }

private:
  lsp_array_base base_;
};

template <class A, std::size_t N>
class lsp_size_array_state : public sp_size_array_state<A, N> {
public:
  template <class U>
  lsp_size_array_state(const U &other, std::size_t size) noexcept
      : sp_size_array_state<A, N>(other, size) {}

  lsp_array_base &base() noexcept { return base_; }

private:
  lsp_array_base base_;
};

} // namespace detail

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, local_shared_ptr<T>>::type
allocate_local_shared(const A &allocator, std::size_t count) {
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::lsp_array_state<other> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count);
  detail::lsp_array_base &local = node->state().base();
  local.set(node);
  result.release();
  return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start, &local);
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, local_shared_ptr<T>>::type
allocate_local_shared(const A &allocator) {
  enum { count = extent<T>::value };
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::lsp_size_array_state<other, count> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count);
  detail::lsp_array_base &local = node->state().base();
  local.set(node);
  result.release();
  return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start, &local);
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, local_shared_ptr<T>>::type
allocate_local_shared(const A &allocator,
                      std::size_t count,
                      const typename remove_extent<T>::type &value) {
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::lsp_array_state<other> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count, value);
  detail::lsp_array_base &local = node->state().base();
  local.set(node);
  result.release();
  return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start, &local);
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, local_shared_ptr<T>>::type
allocate_local_shared(const A &allocator, const typename remove_extent<T>::type &value) {
  enum { count = extent<T>::value };
  typedef typename detail::sp_array_element<T>::type element;
  typedef typename allocator_rebind<A, element>::type other;
  typedef detail::lsp_size_array_state<other, count> state;
  typedef detail::sp_array_base<state> base;
  detail::sp_array_result<other, base> result(allocator, count);
  base *node = result.get();
  element *start = detail::sp_array_start<element>(node);
  ::new (static_cast<void *>(node)) base(allocator, start, count, value);
  detail::lsp_array_base &local = node->state().base();
  local.set(node);
  result.release();
  return local_shared_ptr<T>(detail::lsp_internal_constructor_tag(), start, &local);
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value, local_shared_ptr<T>>::type
allocate_local_shared_noinit(const A &allocator, std::size_t count) {
  return boost::allocate_local_shared<T>(boost::noinit_adapt(allocator), count);
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value, local_shared_ptr<T>>::type
allocate_local_shared_noinit(const A &allocator) {
  return boost::allocate_local_shared<T>(boost::noinit_adapt(allocator));
}

} // namespace boost

/* @file_end smart_ptr/allocate_local_shared_array.hpp */

/* @file_start smart_ptr/make_local_shared_array.hpp */

namespace boost {

template <class T>
inline typename enable_if_<is_bounded_array<T>::value, local_shared_ptr<T>>::type
make_local_shared() {
  return boost::allocate_local_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>());
}

template <class T>
inline typename enable_if_<is_bounded_array<T>::value, local_shared_ptr<T>>::type
make_local_shared(const typename remove_extent<T>::type &value) {
  return boost::allocate_local_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), value);
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, local_shared_ptr<T>>::type
make_local_shared(std::size_t size) {
  return boost::allocate_local_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), size);
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, local_shared_ptr<T>>::type
make_local_shared(std::size_t size, const typename remove_extent<T>::type &value) {
  return boost::allocate_local_shared<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), size, value);
}

template <class T>
inline typename enable_if_<is_bounded_array<T>::value, local_shared_ptr<T>>::type
make_local_shared_noinit() {
  return boost::allocate_local_shared_noinit<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>());
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, local_shared_ptr<T>>::type
make_local_shared_noinit(std::size_t size) {
  return boost::allocate_local_shared_noinit<T>(
      boost::default_allocator<typename detail::sp_array_element<T>::type>(), size);
}

} // namespace boost

/* @file_end smart_ptr/make_local_shared_array.hpp */

/* @file_start smart_ptr/make_shared_object.hpp */

namespace boost {

namespace detail {

template <std::size_t N, std::size_t A> struct sp_aligned_storage {
  union type {
    char data_[N];
    typename boost::type_with_alignment<A>::type align_;
  };
};

template <class T> class sp_ms_deleter {
private:
  typedef typename sp_aligned_storage<sizeof(T), ::boost::alignment_of<T>::value>::type
      storage_type;

  bool initialized_;
  storage_type storage_;

private:
  void destroy() noexcept {
    if (initialized_) {
#if defined(__GNUC__)
      // fixes incorrect aliasing warning
      T *p = reinterpret_cast<T *>(storage_.data_);
      p->~T();
#else
      reinterpret_cast<T *>(storage_.data_)->~T();
#endif

      initialized_ = false;
    }
  }

public:
  sp_ms_deleter() noexcept : initialized_(false) {}

  template <class A> explicit sp_ms_deleter(A const &) noexcept : initialized_(false) {}

  // optimization: do not copy storage_
  sp_ms_deleter(sp_ms_deleter const &) noexcept : initialized_(false) {}

  ~sp_ms_deleter() noexcept { destroy(); }

  void operator()(T *) noexcept { destroy(); }

  static void operator_fn(T *) noexcept // operator() can't be static
  {}

  void *address() noexcept { return storage_.data_; }

  void set_initialized() noexcept { initialized_ = true; }
};

template <class T, class A> class sp_as_deleter {
private:
  typedef typename sp_aligned_storage<sizeof(T), ::boost::alignment_of<T>::value>::type
      storage_type;

  storage_type storage_;
  A a_;
  bool initialized_;

private:
  void destroy() noexcept {
    if (initialized_) {
      T *p = reinterpret_cast<T *>(storage_.data_);
      std::allocator_traits<A>::destroy(a_, p);
      initialized_ = false;
    }
  }

public:
  sp_as_deleter(A const &a) noexcept : a_(a), initialized_(false) {}

  // optimization: do not copy storage_
  sp_as_deleter(sp_as_deleter const &r) noexcept : a_(r.a_), initialized_(false) {}

  ~sp_as_deleter() noexcept { destroy(); }

  void operator()(T *) noexcept { destroy(); }

  static void operator_fn(T *) noexcept // operator() can't be static
  {}

  void *address() noexcept { return storage_.data_; }

  void set_initialized() noexcept { initialized_ = true; }
};

template <class T> struct sp_if_not_array {
  typedef boost::shared_ptr<T> type;
};

template <class T> struct sp_if_not_array<T[]> {};

template <class T, std::size_t N> struct sp_if_not_array<T[N]> {};

} // namespace detail

// _noinit versions

template <class T> typename boost::detail::sp_if_not_array<T>::type make_shared_noinit() {
  boost::shared_ptr<T> pt(static_cast<T *>(0),
                          boost::detail::sp_inplace_tag<boost::detail::sp_ms_deleter<T>>());

  boost::detail::sp_ms_deleter<T> *pd =
      static_cast<boost::detail::sp_ms_deleter<T> *>(pt._internal_get_untyped_deleter());

  void *pv = pd->address();

  ::new (pv) T;
  pd->set_initialized();

  T *pt2 = static_cast<T *>(pv);

  boost::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
  return boost::shared_ptr<T>(pt, pt2);
}

template <class T, class A>
typename boost::detail::sp_if_not_array<T>::type allocate_shared_noinit(A const &a) {
  boost::shared_ptr<T> pt(static_cast<T *>(0),
                          boost::detail::sp_inplace_tag<boost::detail::sp_ms_deleter<T>>(),
                          a);

  boost::detail::sp_ms_deleter<T> *pd =
      static_cast<boost::detail::sp_ms_deleter<T> *>(pt._internal_get_untyped_deleter());

  void *pv = pd->address();

  ::new (pv) T;
  pd->set_initialized();

  T *pt2 = static_cast<T *>(pv);

  boost::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
  return boost::shared_ptr<T>(pt, pt2);
}

// Variadic templates, rvalue reference

template <class T, class... Args>
typename boost::detail::sp_if_not_array<T>::type make_shared(Args &&...args) {
  boost::shared_ptr<T> pt(static_cast<T *>(0),
                          boost::detail::sp_inplace_tag<boost::detail::sp_ms_deleter<T>>());

  boost::detail::sp_ms_deleter<T> *pd =
      static_cast<boost::detail::sp_ms_deleter<T> *>(pt._internal_get_untyped_deleter());

  void *pv = pd->address();

  ::new (pv) T(boost::detail::sp_forward<Args>(args)...);
  pd->set_initialized();

  T *pt2 = static_cast<T *>(pv);

  boost::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
  return boost::shared_ptr<T>(pt, pt2);
}

template <class T, class A, class... Args>
typename boost::detail::sp_if_not_array<T>::type allocate_shared(A const &a, Args &&...args) {
  typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;
  A2 a2(a);

  typedef boost::detail::sp_as_deleter<T, A2> D;

  boost::shared_ptr<T> pt(static_cast<T *>(0), boost::detail::sp_inplace_tag<D>(), a2);

  D *pd = static_cast<D *>(pt._internal_get_untyped_deleter());
  void *pv = pd->address();

  std::allocator_traits<A2>::construct(
      a2, static_cast<T *>(pv), boost::detail::sp_forward<Args>(args)...);

  pd->set_initialized();

  T *pt2 = static_cast<T *>(pv);

  boost::detail::sp_enable_shared_from_this(&pt, pt2, pt2);
  return boost::shared_ptr<T>(pt, pt2);
}

} // namespace boost

/* @file_end smart_ptr/make_shared_object.hpp */

/* @file_start smart_ptr/make_local_shared_object.hpp */

namespace boost {

namespace detail {

// lsp_if_not_array

template <class T> struct lsp_if_not_array {
  typedef boost::local_shared_ptr<T> type;
};

template <class T> struct lsp_if_not_array<T[]> {};

template <class T, std::size_t N> struct lsp_if_not_array<T[N]> {};

// lsp_ms_deleter

template <class T, class A> class lsp_ms_deleter : public local_counted_impl_em {
private:
  typedef typename sp_aligned_storage<sizeof(T), ::boost::alignment_of<T>::value>::type
      storage_type;

  storage_type storage_;
  A a_;
  bool initialized_;

private:
  void destroy() noexcept {
    if (initialized_) {
      T *p = reinterpret_cast<T *>(storage_.data_);
      std::allocator_traits<A>::destroy(a_, p);
      initialized_ = false;
    }
  }

public:
  explicit lsp_ms_deleter(A const &a) noexcept : a_(a), initialized_(false) {}

  // optimization: do not copy storage_
  lsp_ms_deleter(lsp_ms_deleter const &r) noexcept : a_(r.a_), initialized_(false) {}

  ~lsp_ms_deleter() noexcept { destroy(); }

  void operator()(T *) noexcept { destroy(); }

  static void operator_fn(T *) noexcept {} // operator() can't be static

  void *address() noexcept { return storage_.data_; }

  void set_initialized() noexcept { initialized_ = true; }
};

} // namespace detail

template <class T, class A, class... Args>
typename boost::detail::lsp_if_not_array<T>::type allocate_local_shared(A const &a,
                                                                        Args &&...args) {
  typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;

  A2 a2(a);

  typedef boost::detail::lsp_ms_deleter<T, A2> D;

  boost::shared_ptr<T> pt(static_cast<T *>(0), boost::detail::sp_inplace_tag<D>(), a2);

  D *pd = static_cast<D *>(pt._internal_get_untyped_deleter());
  void *pv = pd->address();

  std::allocator_traits<A2>::construct(a2, static_cast<T *>(pv), std::forward<Args>(args)...);

  pd->set_initialized();

  T *pt2 = static_cast<T *>(pv);
  boost::detail::sp_enable_shared_from_this(&pt, pt2, pt2);

  pd->pn_ = pt._internal_count();

  return boost::local_shared_ptr<T>(boost::detail::lsp_internal_constructor_tag(), pt2, pd);
}

template <class T, class A>
typename boost::detail::lsp_if_not_array<T>::type allocate_local_shared_noinit(A const &a) {
  typedef typename std::allocator_traits<A>::template rebind_alloc<T> A2;

  A2 a2(a);

  typedef boost::detail::lsp_ms_deleter<T, std::allocator<T>> D;

  boost::shared_ptr<T> pt(static_cast<T *>(0), boost::detail::sp_inplace_tag<D>(), a2);

  D *pd = static_cast<D *>(pt._internal_get_untyped_deleter());
  void *pv = pd->address();

  ::new (pv) T;

  pd->set_initialized();

  T *pt2 = static_cast<T *>(pv);
  boost::detail::sp_enable_shared_from_this(&pt, pt2, pt2);

  pd->pn_ = pt._internal_count();

  return boost::local_shared_ptr<T>(boost::detail::lsp_internal_constructor_tag(), pt2, pd);
}

template <class T, class... Args>
typename boost::detail::lsp_if_not_array<T>::type make_local_shared(Args &&...args) {
  typedef typename boost::remove_const<T>::type T2;
  return boost::allocate_local_shared<T2>(std::allocator<T2>(), std::forward<Args>(args)...);
}

template <class T>
typename boost::detail::lsp_if_not_array<T>::type make_local_shared_noinit() {
  typedef typename boost::remove_const<T>::type T2;
  return boost::allocate_shared_noinit<T2>(std::allocator<T2>());
}

} // namespace boost

/* @file_end smart_ptr/make_local_shared_object.hpp */

/* @file_start pointer_cast.hpp */

namespace boost {

// static_pointer_cast overload for raw pointers
template <class T, class U> inline T *static_pointer_cast(U *ptr) noexcept {
  return static_cast<T *>(ptr);
}

// dynamic_pointer_cast overload for raw pointers
template <class T, class U> inline T *dynamic_pointer_cast(U *ptr) noexcept {
  return dynamic_cast<T *>(ptr);
}

// const_pointer_cast overload for raw pointers
template <class T, class U> inline T *const_pointer_cast(U *ptr) noexcept {
  return const_cast<T *>(ptr);
}

// reinterpret_pointer_cast overload for raw pointers
template <class T, class U> inline T *reinterpret_pointer_cast(U *ptr) noexcept {
  return reinterpret_cast<T *>(ptr);
}

} // namespace boost

namespace boost {

// static_pointer_cast overload for std::shared_ptr
using std::static_pointer_cast;

// dynamic_pointer_cast overload for std::shared_ptr
using std::dynamic_pointer_cast;

// const_pointer_cast overload for std::shared_ptr
using std::const_pointer_cast;

// reinterpret_pointer_cast overload for std::shared_ptr
template <class T, class U>
std::shared_ptr<T> reinterpret_pointer_cast(const std::shared_ptr<U> &r) noexcept {
  (void)reinterpret_cast<T *>(static_cast<U *>(0));

  typedef typename std::shared_ptr<T>::element_type E;

  E *p = reinterpret_cast<E *>(r.get());
  return std::shared_ptr<T>(r, p);
}

// static_pointer_cast overload for std::unique_ptr
template <class T, class U>
std::unique_ptr<T> static_pointer_cast(std::unique_ptr<U> &&r) noexcept {
  (void)static_cast<T *>(static_cast<U *>(0));

  typedef typename std::unique_ptr<T>::element_type E;

  return std::unique_ptr<T>(static_cast<E *>(r.release()));
}

// dynamic_pointer_cast overload for std::unique_ptr
template <class T, class U>
std::unique_ptr<T> dynamic_pointer_cast(std::unique_ptr<U> &&r) noexcept {
  (void)dynamic_cast<T *>(static_cast<U *>(0));

  static_assert(boost::has_virtual_destructor<T>::value,
                "The target of dynamic_pointer_cast must have a virtual destructor.");

  T *p = dynamic_cast<T *>(r.get());
  if (p) r.release();
  return std::unique_ptr<T>(p);
}

// const_pointer_cast overload for std::unique_ptr
template <class T, class U>
std::unique_ptr<T> const_pointer_cast(std::unique_ptr<U> &&r) noexcept {
  (void)const_cast<T *>(static_cast<U *>(0));

  typedef typename std::unique_ptr<T>::element_type E;

  return std::unique_ptr<T>(const_cast<E *>(r.release()));
}

// reinterpret_pointer_cast overload for std::unique_ptr
template <class T, class U>
std::unique_ptr<T> reinterpret_pointer_cast(std::unique_ptr<U> &&r) noexcept {
  (void)reinterpret_cast<T *>(static_cast<U *>(0));

  typedef typename std::unique_ptr<T>::element_type E;

  return std::unique_ptr<T>(reinterpret_cast<E *>(r.release()));
}

} // namespace boost

/* @file_end pointer_cast.hpp */

/* @file_start pointer_to_other.hpp */

namespace boost {

// Defines the same pointer type (raw or smart) to another pointee type

template <class T, class U> struct pointer_to_other;

template <class T, class U, template <class> class Sp> struct pointer_to_other<Sp<T>, U> {
  typedef Sp<U> type;
};

template <class T, class T2, class U, template <class, class> class Sp>
struct pointer_to_other<Sp<T, T2>, U> {
  typedef Sp<U, T2> type;
};

template <class T, class T2, class T3, class U, template <class, class, class> class Sp>
struct pointer_to_other<Sp<T, T2, T3>, U> {
  typedef Sp<U, T2, T3> type;
};

template <class T, class U> struct pointer_to_other<T *, U> {
  typedef U *type;
};

} // namespace boost

/* @file_end pointer_to_other.hpp */

/* @file_start core/get_pointer.hpp */
#ifndef GET_POINTER_DWA20021219_HPP
#  define GET_POINTER_DWA20021219_HPP
namespace boost {

// get_pointer(p) extracts a ->* capable pointer from p

template <class T> T *get_pointer(T *p) { return p; }

// get_pointer(shared_ptr<T> const & p) has been moved to shared_ptr.hpp

template <class T> T *get_pointer(std::auto_ptr<T> const &p) { return p.get(); }

template <class T> T *get_pointer(std::unique_ptr<T> const &p) { return p.get(); }

template <class T> T *get_pointer(std::shared_ptr<T> const &p) { return p.get(); }

} // namespace boost

#endif
/* @file_end core/get_pointer.hpp */

/* @file_start smart_ptr/intrusive_ref_counter.hpp */

/* @file_start smart_ptr/detail/atomic_count.hpp */

//  typedef <implementation-defined> boost::detail::atomic_count;
//
//  atomic_count a(n);
//
//    (n is convertible to long)
//
//    Effects: Constructs an atomic_count with an initial value of n
//
//  a;
//
//    Returns: (long) the current value of a
//    Memory Ordering: acquire
//
//  ++a;
//
//    Effects: Atomically increments the value of a
//    Returns: (long) the new value of a
//    Memory Ordering: acquire/release
//
//  --a;
//
//    Effects: Atomically decrements the value of a
//    Returns: (long) the new value of a
//    Memory Ordering: acquire/release

#if defined(AC_DISABLE_THREADS)

/* @file_start atomic_count_nt.hpp */

namespace boost {

namespace detail {

class atomic_count {
public:
  explicit atomic_count(long v) : value_(v) {}

  long operator++() { return ++value_; }

  long operator--() { return --value_; }

  operator long() const { return value_; }

private:
  atomic_count(atomic_count const &);
  atomic_count &operator=(atomic_count const &);

  long value_;
};

} // namespace detail

} // namespace boost

/* @file_end atomic_count_nt.hpp */

#elif defined(AC_USE_STD_ATOMIC)

/* @file_start atomic_count_std_atomic.hpp */

#  include <atomic>
#  include <cstdint>

namespace boost {

namespace detail {

class atomic_count {
public:
  explicit atomic_count(long v) : value_(static_cast<std::int_least32_t>(v)) {}

  long operator++() { return value_.fetch_add(1, std::memory_order_acq_rel) + 1; }

  long operator--() { return value_.fetch_sub(1, std::memory_order_acq_rel) - 1; }

  operator long() const { return value_.load(std::memory_order_acquire); }

private:
  atomic_count(atomic_count const &);

  atomic_count &operator=(atomic_count const &);

  std::atomic_int_least32_t value_;
};

} // namespace detail

} // namespace boost

  /* @file_end atomic_count_std_atomic.hpp */

#elif defined(AC_USE_PTHREADS)

/* @file_start atomic_count_pt.hpp */

#  include <pthread.h>

//
//  The generic pthread_mutex-based implementation sometimes leads to
//    inefficiencies. Example: a class with two atomic_count members
//    can get away with a single mutex.
//
//  Users can detect this situation by checking AC_USE_PTHREADS.
//

namespace boost {

namespace detail {

class atomic_count {
private:
  class scoped_lock {
  public:
    scoped_lock(pthread_mutex_t &m) : m_(m) { assert(pthread_mutex_lock(&m_) == 0); }

    ~scoped_lock() { assert(pthread_mutex_unlock(&m_) == 0); }

  private:
    pthread_mutex_t &m_;
  };

public:
  explicit atomic_count(long v) : value_(v) { assert(pthread_mutex_init(&mutex_, 0) == 0); }

  ~atomic_count() { assert(pthread_mutex_destroy(&mutex_) == 0); }

  long operator++() {
    scoped_lock lock(mutex_);
    return ++value_;
  }

  long operator--() {
    scoped_lock lock(mutex_);
    return --value_;
  }

  operator long() const {
    scoped_lock lock(mutex_);
    return value_;
  }

private:
  atomic_count(atomic_count const &);
  atomic_count &operator=(atomic_count const &);

  mutable pthread_mutex_t mutex_;
  long value_;
};

} // namespace detail

} // namespace boost

  /* @file_end atomic_count_pt.hpp */

#endif

/* @file_end smart_ptr/detail/atomic_count.hpp */

#if defined(_MSC_VER)
#  pragma warning(push)
// This is a bogus MSVC warning, which is flagged by friend declarations of
// intrusive_ptr_add_ref and intrusive_ptr_release in intrusive_ref_counter:
// 'name' : the inline specifier cannot be used when a friend declaration refers
// to a specialization of a function template Note that there is no inline
// specifier in the declarations.
#  pragma warning(disable : 4396)
#endif

namespace boost {

namespace sp_adl_block {

/*!
 * \brief Thread unsafe reference counter policy for \c intrusive_ref_counter
 *
 * The policy instructs the \c intrusive_ref_counter base class to implement
 * a reference counter suitable for single threaded use only. Pointers to the
 * same object with this kind of reference counter must not be used by different
 * threads.
 */
struct thread_unsafe_counter {
  typedef unsigned int type;

  static unsigned int load(unsigned int const &counter) noexcept { return counter; }

  static void increment(unsigned int &counter) noexcept { ++counter; }

  static unsigned int decrement(unsigned int &counter) noexcept { return --counter; }
};

/*!
 * \brief Thread safe reference counter policy for \c intrusive_ref_counter
 *
 * The policy instructs the \c intrusive_ref_counter base class to implement
 * a thread-safe reference counter, if the target platform supports
 * multithreading.
 */
struct thread_safe_counter {
  typedef boost::detail::atomic_count type;

  static unsigned int load(boost::detail::atomic_count const &counter) noexcept {
    return static_cast<unsigned int>(static_cast<long>(counter));
  }

  static void increment(boost::detail::atomic_count &counter) noexcept { ++counter; }

  static unsigned int decrement(boost::detail::atomic_count &counter) noexcept {
    return static_cast<unsigned int>(--counter);
  }
};

template <typename DerivedT, typename CounterPolicyT = thread_safe_counter>
class intrusive_ref_counter;

template <typename DerivedT, typename CounterPolicyT>
void intrusive_ptr_add_ref(const intrusive_ref_counter<DerivedT, CounterPolicyT> *p) noexcept;

template <typename DerivedT, typename CounterPolicyT>
void intrusive_ptr_release(const intrusive_ref_counter<DerivedT, CounterPolicyT> *p) noexcept;

/*!
 * \brief A reference counter base class
 *
 * This base class can be used with user-defined classes to add support
 * for \c intrusive_ptr. The class contains a reference counter defined by the
 * \c CounterPolicyT. Upon releasing the last \c intrusive_ptr referencing the
 * object derived from the \c intrusive_ref_counter class, operator \c delete is
 * automatically called on the pointer to the object.
 *
 * The other template parameter, \c DerivedT, is the user's class that derives
 * from \c intrusive_ref_counter.
 */
template <typename DerivedT, typename CounterPolicyT> class intrusive_ref_counter {
private:
  //! Reference counter type
  typedef typename CounterPolicyT::type counter_type;
  //! Reference counter
  mutable counter_type m_ref_counter;

public:
  /*!
     * Default constructor
     *
     * \post <tt>use_count() == 0</tt>
     */
  intrusive_ref_counter() noexcept : m_ref_counter(0) {}

  /*!
     * Copy constructor
     *
     * \post <tt>use_count() == 0</tt>
     */
  intrusive_ref_counter(intrusive_ref_counter const &) noexcept : m_ref_counter(0) {}

  /*!
     * Assignment
     *
     * \post The reference counter is not modified after assignment
     */
  intrusive_ref_counter &operator=(intrusive_ref_counter const &) noexcept { return *this; }

  /*!
     * \return The reference counter
     */
  unsigned int use_count() const noexcept { return CounterPolicyT::load(m_ref_counter); }

protected:
  /*!
     * Destructor
     */
  ~intrusive_ref_counter() = default;

  friend void intrusive_ptr_add_ref<DerivedT, CounterPolicyT>(
      const intrusive_ref_counter<DerivedT, CounterPolicyT> *p) noexcept;

  friend void intrusive_ptr_release<DerivedT, CounterPolicyT>(
      const intrusive_ref_counter<DerivedT, CounterPolicyT> *p) noexcept;
};

template <typename DerivedT, typename CounterPolicyT>
inline void
intrusive_ptr_add_ref(const intrusive_ref_counter<DerivedT, CounterPolicyT> *p) noexcept {
  CounterPolicyT::increment(p->m_ref_counter);
}

template <typename DerivedT, typename CounterPolicyT>
inline void
intrusive_ptr_release(const intrusive_ref_counter<DerivedT, CounterPolicyT> *p) noexcept {
  if (CounterPolicyT::decrement(p->m_ref_counter) == 0)
    delete static_cast<const DerivedT *>(p);
}

} // namespace sp_adl_block

using sp_adl_block::intrusive_ref_counter;
using sp_adl_block::thread_safe_counter;
using sp_adl_block::thread_unsafe_counter;

} // namespace boost

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

/* @file_end smart_ptr/intrusive_ref_counter.hpp */

/* @file_start smart_ptr/owner_less.hpp */

namespace boost {

template <class T = void> struct owner_less {
  typedef bool result_type;
  typedef T first_argument_type;
  typedef T second_argument_type;

  template <class U, class V> bool operator()(U const &u, V const &v) const noexcept {
    return u.owner_before(v);
  }
};

} // namespace boost

/* @file_end smart_ptr/owner_less.hpp */

/* @file_start smart_ptr/owner_hash.hpp */

#include <cstddef>

namespace boost {

template <class T> struct owner_hash {
  typedef std::size_t result_type;
  typedef T argument_type;

  std::size_t operator()(T const &t) const noexcept { return t.owner_hash_value(); }
};

} // namespace boost

/* @file_end smart_ptr/owner_hash.hpp */

/* @file_start smart_ptr/owner_equal_to.hpp */

namespace boost {

template <class T = void> struct owner_equal_to {
  typedef bool result_type;
  typedef T first_argument_type;
  typedef T second_argument_type;

  template <class U, class V> bool operator()(U const &u, V const &v) const noexcept {
    return u.owner_equals(v);
  }
};

} // namespace boost

/* @file_end smart_ptr/owner_equal_to.hpp */

/* @file_start boost/type_traits/is_array.hpp */

// Some fixes for is_array are based on a newsgroup posting by Jonathan
// Lundquist.

#include <cstddef> // size_t

namespace boost {

template <class T> struct is_array : public false_type {};
template <class T, std::size_t N> struct is_array<T[N]> : public true_type {};
template <class T, std::size_t N> struct is_array<T const[N]> : public true_type {};
template <class T, std::size_t N> struct is_array<T volatile[N]> : public true_type {};
template <class T, std::size_t N> struct is_array<T const volatile[N]> : public true_type {};
template <class T> struct is_array<T[]> : public true_type {};
template <class T> struct is_array<T const[]> : public true_type{};
template <class T> struct is_array<T const volatile[]> : public true_type{};
template <class T> struct is_array<T volatile[]> : public true_type{};

} // namespace boost

/* @file_end boost/type_traits/is_array.hpp */

/* @file_start smart_ptr/make_unique.hpp */

namespace boost {

template <class T>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T>>::type make_unique() {
  return std::unique_ptr<T>(new T());
}

template <class T, class... Args>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T>>::type
make_unique(typename remove_reference<T>::type &&value) {
  return std::unique_ptr<T>(new T(std::move(value)));
}

template <class T>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T>>::type
make_unique_noinit() {
  return std::unique_ptr<T>(new T);
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, std::unique_ptr<T>>::type
make_unique(std::size_t size) {
  return std::unique_ptr<T>(new typename remove_extent<T>::type[size]());
}

template <class T>
inline typename enable_if_<is_unbounded_array<T>::value, std::unique_ptr<T>>::type
make_unique_noinit(std::size_t size) {
  return std::unique_ptr<T>(new typename remove_extent<T>::type[size]);
}

} // namespace boost

/* @file_end smart_ptr/make_unique.hpp */

/* @file_start boost/core/empty_value.hpp */

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4510)
#endif

namespace boost {

#if defined(__GNUC__) &&                                                                      \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ >= 40700)
#  define EMPTY_VALUE_BASE
#elif defined(_MSC_VER) && (_MSC_VER >= 1800)
#  define EMPTY_VALUE_BASE
#elif defined(__clang__) && !defined(__CUDACC__)
#  if __has_feature(is_empty) && __has_feature(is_final)
#    define EMPTY_VALUE_BASE
#  endif
#endif

template <class T> struct use_empty_value_base {
  enum {
#if defined(EMPTY_VALUE_BASE)
    value = __is_empty(T) && !__is_final(T)
#else
    value = false
#endif
  };
};

#undef EMPTY_VALUE_BASE

struct empty_init_t {};

namespace empty_ {

template <class T, unsigned N = 0, bool E = boost::use_empty_value_base<T>::value>
class empty_value {
public:
  typedef T type;

  empty_value() = default;

  constexpr empty_value(boost::empty_init_t) : value_() {}

  template <class U, class... Args>
  constexpr empty_value(boost::empty_init_t, U &&value, Args &&...args)
      : value_(std::forward<U>(value), std::forward<Args>(args)...) {}

  constexpr const T &get() const noexcept { return value_; }

private:
  T value_;
};

template <class T, unsigned N> class empty_value<T, N, true> : T {
public:
  typedef T type;

  empty_value() = default;

  constexpr empty_value(boost::empty_init_t) : T() {}

  template <class U, class... Args>
  constexpr empty_value(boost::empty_init_t, U &&value, Args &&...args)
      : T(std::forward<U>(value), std::forward<Args>(args)...) {}

  constexpr const T &get() const noexcept { return *this; }
};
} // namespace empty_

using empty_::empty_value;

/*cxx17 inline constexpr should be best*/ constexpr empty_init_t empty_init = empty_init_t();

} // namespace boost

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

/* @file_end boost/core/empty_value.hpp */

/* @file_start boost/type_traits/type_identity.hpp */

namespace boost {

template <class T> struct type_identity {
  typedef T type;
};

template <class T> using type_identity_t = typename type_identity<T>::type;
} // namespace boost

/* @file_end boost/type_traits/type_identity.hpp */

/* @file_start smart_ptr/allocate_unique.hpp */

namespace boost {
namespace detail {

template <class T> struct sp_alloc_size {
  static constexpr std::size_t value = 1;
};

template <class T> struct sp_alloc_size<T[]> {
  static constexpr std::size_t value = sp_alloc_size<T>::value;
};

template <class T, std::size_t N> struct sp_alloc_size<T[N]> {
  static constexpr std::size_t value = N * sp_alloc_size<T>::value;
};

template <class T> struct sp_alloc_result {
  typedef T type;
};

template <class T, std::size_t N> struct sp_alloc_result<T[N]> {
  typedef T type[];
};

template <class T> struct sp_alloc_value {
  typedef typename boost::remove_cv<typename boost::remove_extent<T>::type>::type type;
};

template <class T, class P> class sp_alloc_ptr {
public:
  typedef T element_type;

  sp_alloc_ptr() noexcept : p_() {}

  sp_alloc_ptr(std::size_t, P p) noexcept : p_(p) {}

  sp_alloc_ptr(std::nullptr_t) noexcept : p_() {}

  T &operator*() const { return *p_; }

  T *operator->() const noexcept { return boost::to_address(p_); }

  explicit operator bool() const noexcept { return !!p_; }

  bool operator!() const noexcept { return !p_; }

  P ptr() const noexcept { return p_; }

  static constexpr std::size_t size() noexcept { return 1; }

private:
  P p_;
};

template <class T, class P> class sp_alloc_ptr<T[], P> {
public:
  typedef T element_type;

  sp_alloc_ptr() noexcept : p_() {}

  sp_alloc_ptr(std::size_t n, P p) noexcept : p_(p), n_(n) {}

  sp_alloc_ptr(std::nullptr_t) noexcept : p_() {}

  T &operator[](std::size_t i) const { return p_[i]; }

  explicit operator bool() const noexcept { return !!p_; }

  bool operator!() const noexcept { return !p_; }

  P ptr() const noexcept { return p_; }

  std::size_t size() const noexcept { return n_; }

private:
  P p_;
  std::size_t n_;
};

template <class T, std::size_t N, class P> class sp_alloc_ptr<T[N], P> {
public:
  typedef T element_type;

  sp_alloc_ptr() noexcept : p_() {}

  sp_alloc_ptr(std::size_t, P p) noexcept : p_(p) {}

  sp_alloc_ptr(std::nullptr_t) noexcept : p_() {}

  T &operator[](std::size_t i) const { return p_[i]; }

  explicit operator bool() const noexcept { return !!p_; }

  bool operator!() const noexcept { return !p_; }

  P ptr() const noexcept { return p_; }

  static constexpr std::size_t size() noexcept { return N; }

private:
  P p_;
};

template <class T, class P>
inline bool operator==(const sp_alloc_ptr<T, P> &lhs, const sp_alloc_ptr<T, P> &rhs) {
  return lhs.ptr() == rhs.ptr();
}

template <class T, class P>
inline bool operator!=(const sp_alloc_ptr<T, P> &lhs, const sp_alloc_ptr<T, P> &rhs) {
  return !(lhs == rhs);
}

template <class T, class P>
inline bool operator==(const sp_alloc_ptr<T, P> &lhs, std::nullptr_t) noexcept {
  return !lhs.ptr();
}

template <class T, class P>
inline bool operator==(std::nullptr_t, const sp_alloc_ptr<T, P> &rhs) noexcept {
  return !rhs.ptr();
}

template <class T, class P>
inline bool operator!=(const sp_alloc_ptr<T, P> &lhs, std::nullptr_t) noexcept {
  return !!lhs.ptr();
}

template <class T, class P>
inline bool operator!=(std::nullptr_t, const sp_alloc_ptr<T, P> &rhs) noexcept {
  return !!rhs.ptr();
}

template <class A>
inline void sp_alloc_clear(A &a,
                           typename boost::allocator_pointer<A>::type p,
                           std::size_t,
                           boost::false_type) {
  boost::alloc_destroy(a, boost::to_address(p));
}

template <class A>
inline void sp_alloc_clear(A &a,
                           typename boost::allocator_pointer<A>::type p,
                           std::size_t n,
                           boost::true_type) {
#if defined(_MSC_VER) && _MSC_VER < 1800
  if (!p) { return; }
#endif
  boost::alloc_destroy_n(a,
                         boost::first_scalar(boost::to_address(p)),
                         n * sp_alloc_size<typename A::value_type>::value);
}

} // namespace detail

template <class T, class A>
class alloc_deleter
    : empty_value<
          typename allocator_rebind<A, typename detail::sp_alloc_value<T>::type>::type> {
  typedef
      typename allocator_rebind<A, typename detail::sp_alloc_value<T>::type>::type allocator;
  typedef empty_value<allocator> base;

public:
  typedef detail::sp_alloc_ptr<T, typename allocator_pointer<allocator>::type> pointer;

  explicit alloc_deleter(const allocator &a) noexcept : base(empty_init_t(), a) {}

  void operator()(pointer p) {
    detail::sp_alloc_clear(base::get(), p.ptr(), p.size(), is_array<T>());
    base::get().deallocate(p.ptr(), p.size());
  }
};

template <class T, class A> using alloc_noinit_deleter = alloc_deleter<T, noinit_adaptor<A>>;

namespace detail {

template <class T, class A> class sp_alloc_make {
public:
  typedef
      typename boost::allocator_rebind<A, typename sp_alloc_value<T>::type>::type allocator;

private:
  typedef boost::alloc_deleter<T, A> deleter;

public:
  typedef std::unique_ptr<typename sp_alloc_result<T>::type, deleter> type;

  sp_alloc_make(const A &a, std::size_t n) : a_(a), n_(n), p_(a_.allocate(n)) {}

  ~sp_alloc_make() {
    if (p_) { a_.deallocate(p_, n_); }
  }

  typename allocator::value_type *get() const noexcept { return boost::to_address(p_); }

  allocator &state() noexcept { return a_; }

  type release() noexcept {
    pointer p = p_;
    p_ = pointer();
    return type(typename deleter::pointer(n_, p), deleter(a_));
  }

private:
  typedef typename boost::allocator_pointer<allocator>::type pointer;

  allocator a_;
  std::size_t n_;
  pointer p_;
};

} // namespace detail

template <class T, class A>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc) {
  detail::sp_alloc_make<T, A> c(alloc, 1);
  boost::alloc_construct(c.state(), c.get());
  return c.release();
}

template <class T, class A, class... Args>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc, Args &&...args) {
  detail::sp_alloc_make<T, A> c(alloc, 1);
  boost::alloc_construct(c.state(), c.get(), std::forward<Args>(args)...);
  return c.release();
}

template <class T, class A>
inline typename enable_if_<!is_array<T>::value, std::unique_ptr<T, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc, typename type_identity<T>::type &&value) {
  detail::sp_alloc_make<T, A> c(alloc, 1);
  boost::alloc_construct(c.state(), c.get(), std::move(value));
  return c.release();
}

template <class T, class A>
inline typename enable_if_<!is_array<T>::value,
                           std::unique_ptr<T, alloc_deleter<T, noinit_adaptor<A>>>>::type
allocate_unique_noinit(const A &alloc) {
  return boost::allocate_unique<T, noinit_adaptor<A>>(alloc);
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
                           std::unique_ptr<T, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc, std::size_t size) {
  detail::sp_alloc_make<T, A> c(alloc, size);
  boost::alloc_construct_n(
      c.state(), boost::first_scalar(c.get()), size * detail::sp_alloc_size<T>::value);
  return c.release();
}

template <class T, class A>
inline typename enable_if_<
    is_bounded_array<T>::value,
    std::unique_ptr<typename detail::sp_alloc_result<T>::type, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc) {
  detail::sp_alloc_make<T, A> c(alloc, extent<T>::value);
  boost::alloc_construct_n(
      c.state(), boost::first_scalar(c.get()), detail::sp_alloc_size<T>::value);
  return c.release();
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
                           std::unique_ptr<T, alloc_deleter<T, noinit_adaptor<A>>>>::type
allocate_unique_noinit(const A &alloc, std::size_t size) {
  return boost::allocate_unique<T, noinit_adaptor<A>>(alloc, size);
}

template <class T, class A>
inline typename enable_if_<is_bounded_array<T>::value,
                           std::unique_ptr<typename detail::sp_alloc_result<T>::type,
                                           alloc_deleter<T, noinit_adaptor<A>>>>::type
allocate_unique_noinit(const A &alloc) {
  return boost::allocate_unique<T, noinit_adaptor<A>>(alloc);
}

template <class T, class A>
inline typename enable_if_<is_unbounded_array<T>::value,
                           std::unique_ptr<T, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc,
                std::size_t size,
                const typename remove_extent<T>::type &value) {
  detail::sp_alloc_make<T, A> c(alloc, size);
  boost::alloc_construct_n(c.state(),
                           boost::first_scalar(c.get()),
                           size * detail::sp_alloc_size<T>::value,
                           boost::first_scalar(&value),
                           detail::sp_alloc_size<typename remove_extent<T>::type>::value);
  return c.release();
}

template <class T, class A>
inline typename enable_if_<
    is_bounded_array<T>::value,
    std::unique_ptr<typename detail::sp_alloc_result<T>::type, alloc_deleter<T, A>>>::type
allocate_unique(const A &alloc, const typename remove_extent<T>::type &value) {
  detail::sp_alloc_make<T, A> c(alloc, extent<T>::value);
  boost::alloc_construct_n(c.state(),
                           boost::first_scalar(c.get()),
                           detail::sp_alloc_size<T>::value,
                           boost::first_scalar(&value),
                           detail::sp_alloc_size<typename remove_extent<T>::type>::value);
  return c.release();
}

template <class T, class U, class A>
inline typename allocator_pointer<
    typename allocator_rebind<A, typename detail::sp_alloc_value<T>::type>::type>::type
get_allocator_pointer(const std::unique_ptr<T, alloc_deleter<U, A>> &p) noexcept {
  return p.get().ptr();
}

} // namespace boost

/* @file_end smart_ptr/allocate_unique.hpp */

namespace boost {
namespace alignment {

namespace detail {

template <class T, class U> struct not_pointer {
  typedef U type;
};

template <class T, class U> struct not_pointer<T *, U> {};

constexpr inline bool is_alignment(std::size_t value) noexcept {
  return (value > 0) && ((value & (value - 1)) == 0);
}
} // namespace detail

inline bool is_aligned(const volatile void *ptr, std::size_t alignment) noexcept {
  assert(detail::is_alignment(alignment));
  return (reinterpret_cast<std::size_t>(ptr) & (alignment - 1)) == 0;
}

inline bool is_aligned(std::size_t alignment, const volatile void *ptr) noexcept {
  assert(detail::is_alignment(alignment));
  return (reinterpret_cast<std::size_t>(ptr) & (alignment - 1)) == 0;
}

template <class T>
constexpr inline typename detail::not_pointer<T, bool>::type
is_aligned(T value, std::size_t alignment) noexcept {
  return (value & (T(alignment) - 1)) == 0;
}

} // namespace alignment
} // namespace boost

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4619) // there is no warning number 'XXXX'
#  pragma warning(disable : 4324) // structure was padded due to __declspec(align())
#  pragma warning(                                                                            \
          disable : 4675) // "function":  resolved overload was found by argument-dependent lookup
#  pragma warning(                                                                            \
          disable : 4996) // "function": was declared deprecated (_CRT_SECURE_NO_DEPRECATE/_SCL_SECURE_NO_WARNINGS)
#  pragma warning(disable : 4714) // "function": marked as __forceinline not inlined
#  pragma warning(disable : 4127) // conditional expression is constant
#endif

#include <cassert>
#include <cstddef> //For std::nullptr_t and std::size_t

//Small meta-typetraits to support move

namespace boost {

namespace movelib {

template <class T> struct default_delete;

} // namespace movelib

namespace move_upmu {

//////////////////////////////////////
//              nat
//////////////////////////////////////
struct nat {};

//////////////////////////////////////
//            natify
//////////////////////////////////////
template <class T> struct natify {};

//////////////////////////////////////
//             if_c
//////////////////////////////////////
template <bool C, typename T1, typename T2> struct if_c {
  typedef T1 type;
};

template <typename T1, typename T2> struct if_c<false, T1, T2> {
  typedef T2 type;
};

//////////////////////////////////////
//             if_
//////////////////////////////////////
template <typename T1, typename T2, typename T3> struct if_ : if_c<0 != T1::value, T2, T3> {};

//enable_if_
template <bool B, class T = nat> struct enable_if_c {
  typedef T type;
};

//////////////////////////////////////
//          enable_if_c
//////////////////////////////////////
template <class T> struct enable_if_c<false, T> {};

//////////////////////////////////////
//           enable_if
//////////////////////////////////////
template <class Cond, class T = nat> struct enable_if : public enable_if_c<Cond::value, T> {};

//////////////////////////////////////
//          remove_reference
//////////////////////////////////////
template <class T> struct remove_reference {
  typedef T type;
};

template <class T> struct remove_reference<T &> {
  typedef T type;
};

template <class T> struct remove_reference<T &&> {
  typedef T type;
};

//////////////////////////////////////
//             remove_const
//////////////////////////////////////
template <class T> struct remove_const {
  typedef T type;
};

template <class T> struct remove_const<const T> {
  typedef T type;
};

//////////////////////////////////////
//             remove_volatile
//////////////////////////////////////
template <class T> struct remove_volatile {
  typedef T type;
};

template <class T> struct remove_volatile<volatile T> {
  typedef T type;
};

//////////////////////////////////////
//             remove_cv
//////////////////////////////////////
template <class T> struct remove_cv {
  typedef typename remove_volatile<typename remove_const<T>::type>::type type;
};

//////////////////////////////////////
//          remove_extent
//////////////////////////////////////
template <class T> struct remove_extent {
  typedef T type;
};

template <class T> struct remove_extent<T[]> {
  typedef T type;
};

template <class T, std::size_t N> struct remove_extent<T[N]> {
  typedef T type;
};

//////////////////////////////////////
//             extent
//////////////////////////////////////

template <class T, unsigned N = 0> struct extent {
  static const std::size_t value = 0;
};

template <class T> struct extent<T[], 0> {
  static const std::size_t value = 0;
};

template <class T, unsigned N> struct extent<T[], N> {
  static const std::size_t value = extent<T, N - 1>::value;
};

template <class T, std::size_t N> struct extent<T[N], 0> {
  static const std::size_t value = N;
};

template <class T, std::size_t I, unsigned N> struct extent<T[I], N> {
  static const std::size_t value = extent<T, N - 1>::value;
};

//////////////////////////////////////
//      add_lvalue_reference
//////////////////////////////////////
template <class T> struct add_lvalue_reference {
  typedef T &type;
};

template <class T> struct add_lvalue_reference<T &> {
  typedef T &type;
};

template <> struct add_lvalue_reference<void> {
  typedef void type;
};

template <> struct add_lvalue_reference<const void> {
  typedef const void type;
};

template <> struct add_lvalue_reference<volatile void> {
  typedef volatile void type;
};

template <> struct add_lvalue_reference<const volatile void> {
  typedef const volatile void type;
};

template <class T> struct add_const_lvalue_reference {
  typedef typename remove_reference<T>::type t_unreferenced;
  typedef const t_unreferenced t_unreferenced_const;
  typedef typename add_lvalue_reference<t_unreferenced_const>::type type;
};

//////////////////////////////////////
//             is_same
//////////////////////////////////////
template <class T, class U> struct is_same {
  static const bool value = false;
};

template <class T> struct is_same<T, T> {
  static const bool value = true;
};

//////////////////////////////////////
//             is_pointer
//////////////////////////////////////
template <class T> struct is_pointer {
  static const bool value = false;
};

template <class T> struct is_pointer<T *> {
  static const bool value = true;
};

//////////////////////////////////////
//             is_reference
//////////////////////////////////////
template <class T> struct is_reference {
  static const bool value = false;
};

template <class T> struct is_reference<T &> {
  static const bool value = true;
};

template <class T> struct is_reference<T &&> {
  static const bool value = true;
};

//////////////////////////////////////
//             is_lvalue_reference
//////////////////////////////////////
template <class T> struct is_lvalue_reference {
  static const bool value = false;
};

template <class T> struct is_lvalue_reference<T &> {
  static const bool value = true;
};

//////////////////////////////////////
//          is_array
//////////////////////////////////////
template <class T> struct is_array {
  static const bool value = false;
};

template <class T> struct is_array<T[]> {
  static const bool value = true;
};

template <class T, std::size_t N> struct is_array<T[N]> {
  static const bool value = true;
};

//////////////////////////////////////
//          has_pointer_type
//////////////////////////////////////
template <class T> struct has_pointer_type {
  struct two {
    char c[2];
  };
  template <class U> static two test(...);
  template <class U> static char test(typename U::pointer * = 0);
  static const bool value = sizeof(test<T>(0)) == 1;
};

//////////////////////////////////////
//             pointer_type
//////////////////////////////////////
template <class T, class D, bool = has_pointer_type<D>::value> struct pointer_type_imp {
  typedef typename D::pointer type;
};

template <class T, class D> struct pointer_type_imp<T, D, false> {
  typedef T *type;
};

template <class T, class D> struct pointer_type {
  typedef typename pointer_type_imp<typename remove_extent<T>::type,
                                    typename remove_reference<D>::type>::type type;
};

//////////////////////////////////////
//           is_convertible
//////////////////////////////////////
#if defined(_MSC_VER) && (_MSC_VER >= 1400)

//use intrinsic since in MSVC
//overaligned types can't go through ellipsis
template <class T, class U> struct is_convertible {
  static const bool value = __is_convertible_to(T, U);
};

#else

template <class T, class U> class is_convertible {
  typedef typename add_lvalue_reference<T>::type t_reference;
  typedef char true_t;
  class false_t {
    char dummy[2];
  };
  static false_t dispatch(...);
  static true_t dispatch(U);
  static t_reference trigger();

public:
  static const bool value = sizeof(dispatch(trigger())) == sizeof(true_t);
};

#endif

} // namespace move_upmu
} //namespace boost

//!\file
//! Describes the default deleter (destruction policy) of <tt>unique_ptr</tt>: <tt>default_delete</tt>.

namespace boost {
// @cond
namespace move_upd {

namespace bmupmu = ::boost::move_upmu;

////////////////////////////////////////
////        enable_def_del
////////////////////////////////////////

//compatible with a pointer type T*:
//When either Y* is convertible to T*
//Y is U[N] and T is U cv []
template <class U, class T>
struct def_del_compatible_cond : bmupmu::is_convertible<U *, T *> {};

template <class U, class T, std::size_t N>
struct def_del_compatible_cond<U[N], T[]> : def_del_compatible_cond<U[], T[]> {};

template <class U, class T, class Type = bmupmu::nat>
struct enable_def_del : bmupmu::enable_if_c<def_del_compatible_cond<U, T>::value, Type> {};

////////////////////////////////////////
////        enable_defdel_call
////////////////////////////////////////

//When 2nd is T[N], 1st(*)[N] shall be convertible to T(*)[N];
//When 2nd is T[],  1st(*)[] shall be convertible to T(*)[];
//Otherwise, 1st* shall be convertible to 2nd*.

template <class U, class T, class Type = bmupmu::nat>
struct enable_defdel_call : public enable_def_del<U, T, Type> {};

template <class U, class T, class Type>
struct enable_defdel_call<U, T[], Type> : public enable_def_del<U[], T[], Type> {};

template <class U, class T, class Type, std::size_t N>
struct enable_defdel_call<U, T[N], Type> : public enable_def_del<U[N], T[N], Type> {};

////////////////////////////////////////
////     Some bool literal zero conversion utilities
////////////////////////////////////////

struct bool_conversion {
  int for_bool;
  int for_arg();
};
typedef int bool_conversion::*explicit_bool_arg;

typedef decltype(nullptr) nullptr_type;

template <bool B> struct is_array_del {};

template <class T> void call_delete(T *p, is_array_del<true>) { delete[] p; }

template <class T> void call_delete(T *p, is_array_del<false>) { delete p; }

template <class T,
          class U,
          bool enable =
              def_del_compatible_cond<U, T>::value && !move_upmu::is_array<T>::value &&
              !move_upmu::is_same<typename move_upmu::remove_cv<T>::type, void>::value &&
              !move_upmu::is_same<typename move_upmu::remove_cv<U>::type,
                                  typename move_upmu::remove_cv<T>::type>::value>
struct missing_virtual_destructor_default_delete {
  static const bool value = !std::has_virtual_destructor<T>::value;
};

template <class T, class U> struct missing_virtual_destructor_default_delete<T, U, false> {
  static const bool value = false;
};

//////////////////////////////////////
//       missing_virtual_destructor
//////////////////////////////////////

template <class Deleter, class U> struct missing_virtual_destructor {
  static const bool value = false;
};

template <class T, class U>
struct missing_virtual_destructor<::boost::movelib::default_delete<T>, U>
    : missing_virtual_destructor_default_delete<T, U> {};

//////////////////////////////////////
//       is_unary_function
//////////////////////////////////////
#if defined(_MSC_VER) || defined(__BORLANDC_)
#  define BOOST_MOVE_TT_DECL __cdecl
#else
#  define BOOST_MOVE_TT_DECL
#endif

#if defined(_MSC_EXTENSIONS) && !defined(__BORLAND__) && !defined(_WIN64) &&                  \
    !defined(_M_ARM) && !defined(_M_ARM64) && !defined(UNDER_CE)
#  define BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS
#endif

template <typename T> struct is_unary_function_impl {
  static const bool value = false;
};

// avoid duplicate definitions of is_unary_function_impl
#ifndef BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R> struct is_unary_function_impl<R (*)()> {
  static const bool value = true;
};

template <typename R> struct is_unary_function_impl<R (*)(...)> {
  static const bool value = true;
};

#else // BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R> struct is_unary_function_impl<R(__stdcall *)()> {
  static const bool value = true;
};

#  ifndef _MANAGED

template <typename R> struct is_unary_function_impl<R(__fastcall *)()> {
  static const bool value = true;
};

#  endif

template <typename R> struct is_unary_function_impl<R(__cdecl *)()> {
  static const bool value = true;
};

template <typename R> struct is_unary_function_impl<R(__cdecl *)(...)> {
  static const bool value = true;
};

#endif

// avoid duplicate definitions of is_unary_function_impl
#ifndef BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0> struct is_unary_function_impl<R (*)(T0)> {
  static const bool value = true;
};

template <typename R, class T0> struct is_unary_function_impl<R (*)(T0...)> {
  static const bool value = true;
};

#else // BOOST_MOVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0> struct is_unary_function_impl<R(__stdcall *)(T0)> {
  static const bool value = true;
};

#  ifndef _MANAGED

template <typename R, class T0> struct is_unary_function_impl<R(__fastcall *)(T0)> {
  static const bool value = true;
};

#  endif

template <typename R, class T0> struct is_unary_function_impl<R(__cdecl *)(T0)> {
  static const bool value = true;
};

template <typename R, class T0> struct is_unary_function_impl<R(__cdecl *)(T0...)> {
  static const bool value = true;
};

#endif

template <typename T> struct is_unary_function_impl<T &> {
  static const bool value = false;
};

template <typename T> struct is_unary_function {
  static const bool value = is_unary_function_impl<T>::value;
};

} //namespace move_upd
// @endcond

namespace movelib {

namespace bmupd = boost::move_upd;
namespace bmupmu = ::boost::move_upmu;

//!The class template <tt>default_delete</tt> serves as the default deleter
//!(destruction policy) for the class template <tt>unique_ptr</tt>.
//!
//! \tparam T The type to be deleted. It may be an incomplete type
template <class T> struct default_delete {
  //! Default constructor.
  //!
  constexpr default_delete() noexcept = default;

  //! Trivial copy constructor
  //!
  default_delete(const default_delete &) noexcept = default;
  //! Trivial assignment
  //!
  default_delete &operator=(const default_delete &) noexcept = default;

  typedef typename bmupmu::remove_extent<T>::type element_type;

  //! <b>Effects</b>: Constructs a default_delete object from another <tt>default_delete<U></tt> object.
  //!
  //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
  //!   - If T is not an array type and U* is implicitly convertible to T*.
  //!   - If T is an array type and U* is a more CV qualified pointer to remove_extent<T>::type.
  template <class U>
  default_delete(const default_delete<U> &,
                 typename bmupd::enable_def_del<U, T>::type * = 0) noexcept {
    //If T is not an array type, U derives from T
    //and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<default_delete, U>::value));
  }

  //! <b>Effects</b>: Constructs a default_delete object from another <tt>default_delete<U></tt> object.
  //!
  //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
  //!   - If T is not an array type and U* is implicitly convertible to T*.
  //!   - If T is an array type and U* is a more CV qualified pointer to remove_extent<T>::type.
  template <class U>
  typename bmupd::enable_def_del<U, T, default_delete &>::type
  operator=(const default_delete<U> &) noexcept {
    //If T is not an array type, U derives from T
    //and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<default_delete, U>::value));
    return *this;
  }

  //! <b>Effects</b>: if T is not an array type, calls <tt>delete</tt> on static_cast<T*>(ptr),
  //!   otherwise calls <tt>delete[]</tt> on static_cast<remove_extent<T>::type*>(ptr).
  //!
  //! <b>Remarks</b>: If U is an incomplete type, the program is ill-formed.
  //!   This operator shall not participate in overload resolution unless:
  //!      - T is not an array type and U* is convertible to T*, OR
  //!      - T is an array type, and remove_cv<U>::type is the same type as
  //!         remove_cv<remove_extent<T>::type>::type and U* is convertible to remove_extent<T>::type*.
  template <class U>
  typename bmupd::enable_defdel_call<U, T, void>::type operator()(U *ptr) const noexcept {
    //U must be a complete type
    static_assert(sizeof(U) > 0);
    //If T is not an array type, U derives from T
    //and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<default_delete, U>::value));
    element_type *const p = static_cast<element_type *>(ptr);
    move_upd::call_delete(p, move_upd::is_array_del<bmupmu::is_array<T>::value>());
  }

  //! <b>Effects</b>: Same as <tt>(*this)(static_cast<element_type*>(nullptr))</tt>.
  //!
  void operator()(bmupd::nullptr_type) const noexcept {
    static_assert(sizeof(element_type) > 0);
  }
};

} // namespace movelib
} // namespace boost

#include <utility>

namespace boost {

using ::std::move;
using ::std::forward;

} //namespace boost

//////////////////////////////////////////////////////////////////////////////
//
//                         move_if_not_lvalue_reference
//
//////////////////////////////////////////////////////////////////////////////

namespace boost {

template <class T>
FORCEINLINE T &&
move_if_not_lvalue_reference(typename std::remove_reference<T>::type &t) noexcept {
  return static_cast<T &&>(t);
}

template <class T>
FORCEINLINE T &&
move_if_not_lvalue_reference(typename std::remove_reference<T>::type &&t) noexcept {
  //"boost::forward<T> error: 'T' is a lvalue reference, can't forward as rvalue.";
  static_assert(!std::is_lvalue_reference<T>::value);
  return static_cast<T &&>(t);
}

} // namespace boost

namespace boost {
namespace move_detail {

template <typename T> typename std::add_rvalue_reference<T>::type declval();

} // namespace move_detail
} // namespace boost

namespace boost_move_adl_swap {

template <class T> FORCEINLINE void swap_proxy(T &x, T &y) {
  using std::swap;
  swap(x, y);
}

} // namespace boost_move_adl_swap

namespace boost_move_adl_swap {

template <class T, std::size_t N> void swap_proxy(T (&x)[N], T (&y)[N]) {
  for (std::size_t i = 0; i < N; ++i) { ::boost_move_adl_swap::swap_proxy(x[i], y[i]); }
}

} // namespace boost_move_adl_swap

namespace boost {

//! Exchanges the values of a and b, using Argument Dependent Lookup (ADL) to select a
//! specialized swap function if available. If no specialized swap function is available,
//! std::swap is used.
//!
//! <b>Exception</b>: If T uses Boost.Move's move emulation and the compiler has
//! no rvalue references then:
//!
//!   -  If T has a <code>T::swap(T&)</code> member, that member is called.
//!   -  Otherwise a move-based swap is called, equivalent to:
//!      <code>T t(::boost::move(x)); x = ::boost::move(y); y = ::boost::move(t);</code>.
template <class T> FORCEINLINE void adl_move_swap(T &x, T &y) {
  ::boost_move_adl_swap::swap_proxy(x, y);
}

//! Exchanges elements between range [first1, last1) and another range starting at first2
//! using boost::adl_move_swap.
//!
//! Parameters:
//!   first1, last1   -   the first range of elements to swap
//!   first2   -   beginning of the second range of elements to swap
//!
//! Type requirements:
//!   - ForwardIt1, ForwardIt2 must meet the requirements of ForwardIterator.
//!   - The types of dereferenced ForwardIt1 and ForwardIt2 must meet the
//!     requirements of Swappable
//!
//! Return value: Iterator to the element past the last element exchanged in the range
//! beginning with first2.
template <class ForwardIt1, class ForwardIt2>
ForwardIt2 adl_move_swap_ranges(ForwardIt1 first1, ForwardIt1 last1, ForwardIt2 first2) {
  while (first1 != last1) {
    ::boost::adl_move_swap(*first1, *first2);
    ++first1;
    ++first2;
  }
  return first2;
}

template <class BidirIt1, class BidirIt2>
BidirIt2 adl_move_swap_ranges_backward(BidirIt1 first1, BidirIt1 last1, BidirIt2 last2) {
  while (first1 != last1) { ::boost::adl_move_swap(*(--last1), *(--last2)); }
  return last2;
}

template <class ForwardIt1, class ForwardIt2>
void adl_move_iter_swap(ForwardIt1 a, ForwardIt2 b) {
  boost::adl_move_swap(*a, *b);
}

} // namespace boost

//!\file
//! Describes the smart pointer unique_ptr, a drop-in replacement for std::unique_ptr,
//! usable also from C++03 compilers.
//!
//! Main differences from std::unique_ptr to avoid heavy dependencies,
//! specially in C++03 compilers:
//!   - <tt>operator < </tt> uses pointer <tt>operator < </tt>instead of <tt>std::less<common_type></tt>.
//!      This avoids dependencies on <tt>std::common_type</tt> and <tt>std::less</tt>
//!      (<tt><type_traits>/<functional></tt> headers). In C++03 this avoid pulling Boost.Typeof and other
//!      cascading dependencies. As in all Boost platforms <tt>operator <</tt> on raw pointers and
//!      other smart pointers provides strict weak ordering in practice this should not be a problem for users.
//!   - assignable from literal 0 for compilers without nullptr
//!   - <tt>unique_ptr<T[]></tt> is constructible and assignable from <tt>unique_ptr<U[]></tt> if
//!      cv-less T and cv-less U are the same type and T is more CV qualified than U.

namespace boost {
// @cond
namespace move_upd {

////////////////////////////////////////////
//          deleter types
////////////////////////////////////////////
template <class D> struct deleter_types {
  typedef typename bmupmu::add_lvalue_reference<D>::type del_ref;
  typedef typename bmupmu::add_const_lvalue_reference<D>::type del_cref;
  typedef typename bmupmu::if_c<bmupmu::is_lvalue_reference<D>::value, D, del_cref>::type
      deleter_arg_type1;
  typedef typename bmupmu::remove_reference<D>::type &&deleter_arg_type2;
};

////////////////////////////////////////////
//          unique_ptr_data
////////////////////////////////////////////
template <class P,
          class D,
          bool = move_upd::is_unary_function<D>::value || bmupmu::is_reference<D>::value>
struct unique_ptr_data {
  typedef typename deleter_types<D>::deleter_arg_type1 deleter_arg_type1;
  typedef typename deleter_types<D>::del_ref del_ref;
  typedef typename deleter_types<D>::del_cref del_cref;

  FORCEINLINE unique_ptr_data() noexcept : m_p(), d() {}

  FORCEINLINE explicit unique_ptr_data(P p) noexcept : m_p(p), d() {}

  FORCEINLINE unique_ptr_data(P p, deleter_arg_type1 d1) noexcept : m_p(p), d(d1) {}

  template <class U>
  FORCEINLINE unique_ptr_data(P p, U &&d1) noexcept : m_p(p), d(::boost::forward<U>(d1)) {}

  FORCEINLINE del_ref deleter() { return d; }
  FORCEINLINE del_cref deleter() const { return d; }

  P m_p;
  D d;

private:
  unique_ptr_data &operator=(const unique_ptr_data &);
  unique_ptr_data(const unique_ptr_data &);
};

template <class P, class D> struct unique_ptr_data<P, D, false> : private D {
  typedef typename deleter_types<D>::deleter_arg_type1 deleter_arg_type1;
  typedef typename deleter_types<D>::del_ref del_ref;
  typedef typename deleter_types<D>::del_cref del_cref;

  FORCEINLINE unique_ptr_data() noexcept : D(), m_p() {}

  FORCEINLINE explicit unique_ptr_data(P p) noexcept : D(), m_p(p) {}

  FORCEINLINE unique_ptr_data(P p, deleter_arg_type1 d1) noexcept : D(d1), m_p(p) {}

  template <class U>
  FORCEINLINE unique_ptr_data(P p, U &&d) noexcept : D(::boost::forward<U>(d)), m_p(p) {}

  FORCEINLINE del_ref deleter() noexcept { return static_cast<del_ref>(*this); }
  FORCEINLINE del_cref deleter() const noexcept { return static_cast<del_cref>(*this); }

  P m_p;

private:
  unique_ptr_data &operator=(const unique_ptr_data &);
  unique_ptr_data(const unique_ptr_data &);
};

////////////////////////////////////////////
//          is_unique_ptr_convertible
////////////////////////////////////////////

//Although non-standard, we avoid using pointer_traits
//to avoid heavy dependencies
template <typename T> struct get_element_type {
  struct DefaultWrap {
    typedef bmupmu::natify<T> element_type;
  };
  template <typename X> static char test(int, typename X::element_type *);
  template <typename X> static int test(...);
  static const bool value = (1 == sizeof(test<T>(0, 0)));
  typedef typename bmupmu::if_c<value, T, DefaultWrap>::type::element_type type;
};

template <class T> struct get_element_type<T *> {
  typedef T type;
};

template <class T>
struct get_cvelement : bmupmu::remove_cv<typename get_element_type<T>::type> {};

template <class P1, class P2> struct is_same_cvelement_and_convertible {
  typedef typename bmupmu::remove_reference<P1>::type arg1;
  typedef typename bmupmu::remove_reference<P2>::type arg2;
  static const bool same_cvless = bmupmu::is_same<typename get_cvelement<arg1>::type,
                                                  typename get_cvelement<arg2>::type>::value;
  static const bool value = same_cvless && bmupmu::is_convertible<arg1, arg2>::value;
};

template <bool IsArray, class FromPointer, class ThisPointer>
struct is_unique_ptr_convertible
    : is_same_cvelement_and_convertible<FromPointer, ThisPointer> {};

template <class FromPointer, class ThisPointer>
struct is_unique_ptr_convertible<false, FromPointer, ThisPointer>
    : bmupmu::is_convertible<FromPointer, ThisPointer> {};

////////////////////////////////////////
////     enable_up_moveconv_assign
////////////////////////////////////////

template <class T, class FromPointer, class ThisPointer, class Type = bmupmu::nat>
struct enable_up_ptr
    : bmupmu::enable_if_c<is_unique_ptr_convertible<bmupmu::is_array<T>::value,
                                                    FromPointer,
                                                    ThisPointer>::value,
                          Type> {};

////////////////////////////////////////
////     enable_up_moveconv_assign
////////////////////////////////////////

template <class T, class D, class U, class E> struct unique_moveconvert_assignable {
  static const bool t_is_array = bmupmu::is_array<T>::value;
  static const bool value =
      t_is_array == bmupmu::is_array<U>::value &&
      bmupmu::extent<T>::value == bmupmu::extent<U>::value &&
      is_unique_ptr_convertible<t_is_array,
                                typename bmupmu::pointer_type<U, E>::type,
                                typename bmupmu::pointer_type<T, D>::type>::value;
};

template <class T, class D, class U, class E, std::size_t N>
struct unique_moveconvert_assignable<T[], D, U[N], E>
    : unique_moveconvert_assignable<T[], D, U[], E> {};

template <class T, class D, class U, class E, class Type = bmupmu::nat>
struct enable_up_moveconv_assign
    : bmupmu::enable_if_c<unique_moveconvert_assignable<T, D, U, E>::value, Type> {};

////////////////////////////////////////
////     enable_up_moveconv_constr
////////////////////////////////////////

template <class D, class E, bool IsReference = bmupmu::is_reference<D>::value>
struct unique_deleter_is_initializable : bmupmu::is_same<D, E> {};

template <class T, class U> class is_rvalue_convertible {
  typedef typename bmupmu::remove_reference<T>::type &&t_from;

  typedef char true_t;
  class false_t {
    char dummy[2];
  };
  static false_t dispatch(...);
  static true_t dispatch(U);
  static t_from trigger();

public:
  static const bool value = sizeof(dispatch(trigger())) == sizeof(true_t);
};

template <class D, class E> struct unique_deleter_is_initializable<D, E, false> {
// Clang has some problems with is_rvalue_convertible with non-copyable types
// so use intrinsic if available
#if defined(__clang__)
#  if __has_feature(is_convertible_to)
  static const bool value = __is_convertible_to(E, D);
#  else
  static const bool value = is_rvalue_convertible<E, D>::value;
#  endif
#else
  static const bool value = is_rvalue_convertible<E, D>::value;
#endif
};

template <class T, class D, class U, class E, class Type = bmupmu::nat>
struct enable_up_moveconv_constr
    : bmupmu::enable_if_c<unique_moveconvert_assignable<T, D, U, E>::value &&
                              unique_deleter_is_initializable<D, E>::value,
                          Type> {};

} // namespace move_upd
// @endcond

namespace movelib {

//! A unique pointer is an object that owns another object and
//! manages that other object through a pointer.
//!
//! More precisely, a unique pointer is an object u that stores a pointer to a second object p and will dispose
//! of p when u is itself destroyed (e.g., when leaving block scope). In this context, u is said to own p.
//!
//! The mechanism by which u disposes of p is known as p's associated deleter, a function object whose correct
//! invocation results in p's appropriate disposition (typically its deletion).
//!
//! Let the notation u.p denote the pointer stored by u, and let u.d denote the associated deleter. Upon request,
//! u can reset (replace) u.p and u.d with another pointer and deleter, but must properly dispose of its owned
//! object via the associated deleter before such replacement is considered completed.
//!
//! Additionally, u can, upon request, transfer ownership to another unique pointer u2. Upon completion of
//! such a transfer, the following post-conditions hold:
//!   - u2.p is equal to the pre-transfer u.p,
//!   - u.p is equal to nullptr, and
//!   - if the pre-transfer u.d maintained state, such state has been transferred to u2.d.
//!
//! As in the case of a reset, u2 must properly dispose of its pre-transfer owned object via the pre-transfer
//! associated deleter before the ownership transfer is considered complete.
//!
//! Each object of a type U instantiated from the unique_ptr template specified in this sub-clause has the strict
//! ownership semantics, specified above, of a unique pointer. In partial satisfaction of these semantics, each
//! such U is MoveConstructible and MoveAssignable, but is not CopyConstructible nor CopyAssignable.
//! The template parameter T of unique_ptr may be an incomplete type.
//!
//! The uses of unique_ptr include providing exception safety for dynamically allocated memory, passing
//! ownership of dynamically allocated memory to a function, and returning dynamically allocated memory from
//! a function.
//!
//! If T is an array type (e.g. unique_ptr<MyType[]>) the interface is slightly altered:
//!   - Pointers to types derived from T are rejected by the constructors, and by reset.
//!   - The observers <tt>operator*</tt> and <tt>operator-></tt> are not provided.
//!   - The indexing observer <tt>operator[]</tt> is provided.
//!
//! \tparam T Provides the type of the stored pointer.
//! \tparam D The deleter type:
//!   -  The default type for the template parameter D is default_delete. A client-supplied template argument
//!      D shall be a function object type, lvalue-reference to function, or lvalue-reference to function object type
//!      for which, given a value d of type D and a value ptr of type unique_ptr<T, D>::pointer, the expression
//!      d(ptr) is valid and has the effect of disposing of the pointer as appropriate for that deleter.
//!   -  If the deleter's type D is not a reference type, D shall satisfy the requirements of Destructible.
//!   -  If the type <tt>remove_reference<D>::type::pointer</tt> exists, it shall satisfy the requirements of NullablePointer.
template <class T, class D = default_delete<T>> class unique_ptr {
public:
  unique_ptr(unique_ptr const &) = delete;
  unique_ptr &operator=(unique_ptr const &) = delete;
  typedef int boost_move_no_copy_constructor_or_assign;
  typedef int boost_move_emulation_t;

private:
  typedef bmupmu::pointer_type<T, D> pointer_type_obtainer;
  typedef bmupd::unique_ptr_data<typename pointer_type_obtainer::type, D> data_type;
  typedef typename bmupd::deleter_types<D>::deleter_arg_type1 deleter_arg_type1;
  typedef typename bmupd::deleter_types<D>::deleter_arg_type2 deleter_arg_type2;
  data_type m_data;

public:
  //! If the type <tt>remove_reference<D>::type::pointer</tt> exists, then it shall be a
  //! synonym for <tt>remove_reference<D>::type::pointer</tt>. Otherwise it shall be a
  //! synonym for T*.
  typedef typename pointer_type_obtainer::type pointer;
  //! If T is an array type, then element_type is equal to T. Otherwise, if T is a type
  //! in the form U[], element_type is equal to U.
  typedef typename bmupmu::remove_extent<T>::type element_type;
  typedef D deleter_type;

  //! <b>Requires</b>: D shall satisfy the requirements of DefaultConstructible, and
  //!   that construction shall not throw an exception.
  //!
  //! <b>Effects</b>: Constructs a unique_ptr object that owns nothing, value-initializing the
  //!   stored pointer and the stored deleter.
  //!
  //! <b>Post-conditions</b>: <tt>get() == nullptr</tt>. <tt>get_deleter()</tt> returns a reference to the stored deleter.
  //!
  //! <b>Remarks</b>: If this constructor is instantiated with a pointer type or reference type
  //!   for the template argument D, the program is ill-formed.
  FORCEINLINE constexpr unique_ptr() noexcept : m_data() {
    //If this constructor is instantiated with a pointer type or reference type
    //for the template argument D, the program is ill-formed.
    static_assert(!bmupmu::is_pointer<D>::value);
    static_assert(!bmupmu::is_reference<D>::value);
  }

  //! <b>Effects</b>: Same as <tt>unique_ptr()</tt> (default constructor).
  //!
  FORCEINLINE constexpr unique_ptr(bmupd::nullptr_type) noexcept : m_data() {
    //If this constructor is instantiated with a pointer type or reference type
    //for the template argument D, the program is ill-formed.
    static_assert(!bmupmu::is_pointer<D>::value);
    static_assert(!bmupmu::is_reference<D>::value);
  }

  //! <b>Requires</b>: D shall satisfy the requirements of DefaultConstructible, and
  //!   that construction shall not throw an exception.
  //!
  //! <b>Effects</b>: Constructs a unique_ptr which owns p, initializing the stored pointer
  //!   with p and value initializing the stored deleter.
  //!
  //! <b>Post-conditions</b>: <tt>get() == p</tt>. <tt>get_deleter()</tt> returns a reference to the stored deleter.
  //!
  //! <b>Remarks</b>: If this constructor is instantiated with a pointer type or reference type
  //!   for the template argument D, the program is ill-formed.
  //!   This constructor shall not participate in overload resolution unless:
  //!      - If T is not an array type and Pointer is implicitly convertible to pointer.
  //!      - If T is an array type and Pointer is a more CV qualified pointer to element_type.
  template <class Pointer>
  FORCEINLINE explicit unique_ptr(
      Pointer p, typename bmupd::enable_up_ptr<T, Pointer, pointer>::type * = 0) noexcept
      : m_data(p) {
    //If T is not an array type, element_type_t<Pointer> derives from T
    //it uses the default deleter and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<
                   D,
                   typename bmupd::get_element_type<Pointer>::type>::value));
    //If this constructor is instantiated with a pointer type or reference type
    //for the template argument D, the program is ill-formed.
    static_assert(!bmupmu::is_pointer<D>::value);
    static_assert(!bmupmu::is_reference<D>::value);
  }

  //!The signature of this constructor depends upon whether D is a reference type.
  //!   - If D is non-reference type A, then the signature is <tt>unique_ptr(pointer p, const A& d)</tt>.
  //!   - If D is an lvalue-reference type A&, then the signature is <tt>unique_ptr(pointer p, A& d)</tt>.
  //!   - If D is an lvalue-reference type const A&, then the signature is <tt>unique_ptr(pointer p, const A& d)</tt>.
  //!
  //!
  //! <b>Requires</b>: Either
  //!   - D is not an lvalue-reference type and d is an lvalue or const rvalue.
  //!         D shall satisfy the requirements of CopyConstructible, and the copy constructor of D
  //!         shall not throw an exception. This unique_ptr will hold a copy of d.
  //!   - D is an lvalue-reference type and d is an lvalue. the type which D references need not be CopyConstructible nor
  //!      MoveConstructible. This unique_ptr will hold a D which refers to the lvalue d.
  //!
  //! <b>Effects</b>: Constructs a unique_ptr object which owns p, initializing the stored pointer with p and
  //!   initializing the deleter as described above.
  //!
  //! <b>Post-conditions</b>: <tt>get() == p</tt>. <tt>get_deleter()</tt> returns a reference to the stored deleter. If D is a
  //!   reference type then <tt>get_deleter()</tt> returns a reference to the lvalue d.
  //!
  //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
  //!      - If T is not an array type and Pointer is implicitly convertible to pointer.
  //!      - If T is an array type and Pointer is a more CV qualified pointer to element_type.
  template <class Pointer>
  FORCEINLINE
  unique_ptr(Pointer p,
             deleter_arg_type1 d1,
             typename bmupd::enable_up_ptr<T, Pointer, pointer>::type * = 0) noexcept
      : m_data(p, d1) {
    //If T is not an array type, element_type_t<Pointer> derives from T
    //it uses the default deleter and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<
                   D,
                   typename bmupd::get_element_type<Pointer>::type>::value));
  }

  //! <b>Effects</b>: Same effects as <tt>template<class Pointer> unique_ptr(Pointer p, deleter_arg_type1 d1)</tt>
  //!   and additionally <tt>get() == nullptr</tt>
  FORCEINLINE unique_ptr(bmupd::nullptr_type, deleter_arg_type1 d1) noexcept
      : m_data(pointer(), d1) {}

  //! The signature of this constructor depends upon whether D is a reference type.
  //!   - If D is non-reference type A, then the signature is <tt>unique_ptr(pointer p, A&& d)</tt>.
  //!   - If D is an lvalue-reference type A&, then the signature is <tt>unique_ptr(pointer p, A&& d)</tt>.
  //!   - If D is an lvalue-reference type const A&, then the signature is <tt>unique_ptr(pointer p, const A&& d)</tt>.
  //!
  //! <b>Requires</b>: Either
  //!   - D is not an lvalue-reference type and d is a non-const rvalue. D
  //!      shall satisfy the requirements of MoveConstructible, and the move constructor
  //!      of D shall not throw an exception. This unique_ptr will hold a value move constructed from d.
  //!   - D is an lvalue-reference type and d is an rvalue, the program is ill-formed.
  //!
  //! <b>Effects</b>: Constructs a unique_ptr object which owns p, initializing the stored pointer with p and
  //!   initializing the deleter as described above.
  //!
  //! <b>Post-conditions</b>: <tt>get() == p</tt>. <tt>get_deleter()</tt> returns a reference to the stored deleter. If D is a
  //!   reference type then <tt>get_deleter()</tt> returns a reference to the lvalue d.
  //!
  //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
  //!      - If T is not an array type and Pointer is implicitly convertible to pointer.
  //!      - If T is an array type and Pointer is a more CV qualified pointer to element_type.
  template <class Pointer>
  FORCEINLINE
  unique_ptr(Pointer p,
             deleter_arg_type2 d2,
             typename bmupd::enable_up_ptr<T, Pointer, pointer>::type * = 0) noexcept
      : m_data(p, ::boost::move(d2)) {
    //If T is not an array type, element_type_t<Pointer> derives from T
    //it uses the default deleter and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<
                   D,
                   typename bmupd::get_element_type<Pointer>::type>::value));
  }

  //! <b>Effects</b>: Same effects as <tt>template<class Pointer> unique_ptr(Pointer p, deleter_arg_type2 d2)</tt>
  //!   and additionally <tt>get() == nullptr</tt>
  FORCEINLINE unique_ptr(bmupd::nullptr_type, deleter_arg_type2 d2) noexcept
      : m_data(pointer(), ::boost::move(d2)) {}

  //! <b>Requires</b>: If D is not a reference type, D shall satisfy the requirements of MoveConstructible.
  //! Construction of the deleter from an rvalue of type D shall not throw an exception.
  //!
  //! <b>Effects</b>: Constructs a unique_ptr by transferring ownership from u to *this. If D is a reference type,
  //! this deleter is copy constructed from u's deleter; otherwise, this deleter is move constructed from u's
  //! deleter.
  //!
  //! <b>Post-conditions</b>: <tt>get()</tt> yields the value u.get() yielded before the construction. <tt>get_deleter()</tt>
  //! returns a reference to the stored deleter that was constructed from u.get_deleter(). If D is a
  //! reference type then <tt>get_deleter()</tt> and <tt>u.get_deleter()</tt> both reference the same lvalue deleter.
  FORCEINLINE unique_ptr(unique_ptr &&u) noexcept
      : m_data(u.release(), ::boost::move_if_not_lvalue_reference<D>(u.get_deleter())) {}

  //! <b>Requires</b>: If E is not a reference type, construction of the deleter from an rvalue of type E shall be
  //!   well formed and shall not throw an exception. Otherwise, E is a reference type and construction of the
  //!   deleter from an lvalue of type E shall be well formed and shall not throw an exception.
  //!
  //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
  //!   - <tt>unique_ptr<U, E>::pointer</tt> is implicitly convertible to pointer,
  //!   - U is not an array type, and
  //!   - either D is a reference type and E is the same type as D, or D is not a reference type and E is
  //!      implicitly convertible to D.
  //!
  //! <b>Effects</b>: Constructs a unique_ptr by transferring ownership from u to *this. If E is a reference type,
  //!   this deleter is copy constructed from u's deleter; otherwise, this deleter is move constructed from u's deleter.
  //!
  //! <b>Post-conditions</b>: <tt>get()</tt> yields the value <tt>u.get()</tt> yielded before the construction. <tt>get_deleter()</tt>
  //!   returns a reference to the stored deleter that was constructed from <tt>u.get_deleter()</tt>.
  template <class U, class E>
  FORCEINLINE
  unique_ptr(unique_ptr<U, E> &&u,
             typename bmupd::enable_up_moveconv_constr<T, D, U, E>::type * = 0) noexcept
      : m_data(u.release(), ::boost::move_if_not_lvalue_reference<E>(u.get_deleter())) {
    //If T is not an array type, U derives from T
    //it uses the default deleter and T has no virtual destructor, then you have a problem
    static_assert(
        (!bmupd::missing_virtual_destructor<D, typename unique_ptr<U, E>::pointer>::value),
        "");
  }

  //! <b>Requires</b>: The expression <tt>get_deleter()(get())</tt> shall be well formed, shall have well-defined behavior,
  //!   and shall not throw exceptions.
  //!
  //! <b>Effects</b>: If <tt>get() == nullpt1r</tt> there are no effects. Otherwise <tt>get_deleter()(get())</tt>.
  //!
  //! <b>Note</b>: The use of default_delete requires T to be a complete type
  ~unique_ptr() {
    if (m_data.m_p) m_data.deleter()(m_data.m_p);
  }

  //! <b>Requires</b>: If D is not a reference type, D shall satisfy the requirements of MoveAssignable
  //!   and assignment of the deleter from an rvalue of type D shall not throw an exception. Otherwise, D
  //!   is a reference type; <tt>remove_reference<D>::type</tt> shall satisfy the CopyAssignable requirements and
  //!   assignment of the deleter from an lvalue of type D shall not throw an exception.
  //!
  //! <b>Effects</b>: Transfers ownership from u to *this as if by calling <tt>reset(u.release())</tt> followed
  //!   by <tt>get_deleter() = std::forward<D>(u.get_deleter())</tt>.
  //!
  //! <b>Returns</b>: *this.
  unique_ptr &operator=(unique_ptr &&u) noexcept {
    this->reset(u.release());
    m_data.deleter() = ::boost::move_if_not_lvalue_reference<D>(u.get_deleter());
    return *this;
  }

  //! <b>Requires</b>: If E is not a reference type, assignment of the deleter from an rvalue of type E shall be
  //!   well-formed and shall not throw an exception. Otherwise, E is a reference type and assignment of the
  //!   deleter from an lvalue of type E shall be well-formed and shall not throw an exception.
  //!
  //! <b>Remarks</b>: This operator shall not participate in overload resolution unless:
  //!   - <tt>unique_ptr<U, E>::pointer</tt> is implicitly convertible to pointer and
  //!   - U is not an array type.
  //!
  //! <b>Effects</b>: Transfers ownership from u to *this as if by calling <tt>reset(u.release())</tt> followed by
  //!   <tt>get_deleter() = std::forward<E>(u.get_deleter())</tt>.
  //!
  //! <b>Returns</b>: *this.
  template <class U, class E>
  typename bmupd::enable_up_moveconv_assign<T, D, U, E, unique_ptr &>::type
  operator=(unique_ptr<U, E> &&u) noexcept {
    this->reset(u.release());
    m_data.deleter() = ::boost::move_if_not_lvalue_reference<E>(u.get_deleter());
    return *this;
  }

  //! <b>Effects</b>: <tt>reset()</tt>.
  //!
  //! <b>Postcondition</b>: <tt>get() == nullptr</tt>
  //!
  //! <b>Returns</b>: *this.
  unique_ptr &operator=(bmupd::nullptr_type) noexcept {
    this->reset();
    return *this;
  }

  //! <b>Requires</b>: <tt>get() != nullptr</tt>.
  //!
  //! <b>Returns</b>: <tt>*get()</tt>.
  //!
  //! <b>Remarks</b: If T is an array type, the program is ill-formed.
  typename bmupmu::add_lvalue_reference<element_type>::type operator*() const noexcept {
    static_assert((!bmupmu::is_array<T>::value));
    return *m_data.m_p;
  }

  //! <b>Requires</b>: i < the number of elements in the array to which the stored pointer points.
  //!
  //! <b>Returns</b>: <tt>get()[i]</tt>.
  //!
  //! <b>Remarks</b: If T is not an array type, the program is ill-formed.
  FORCEINLINE typename bmupmu::add_lvalue_reference<element_type>::type
  operator[](std::size_t i) const noexcept {
    assert(bmupmu::extent<T>::value == 0 || i < bmupmu::extent<T>::value);
    assert(m_data.m_p);
    return m_data.m_p[i];
  }

  //! <b>Requires</b>: <tt>get() != nullptr</tt>.
  //!
  //! <b>Returns</b>: <tt>get()</tt>.
  //!
  //! <b>Note</b>: use typically requires that T be a complete type.
  //!
  //! <b>Remarks</b: If T is an array type, the program is ill-formed.
  FORCEINLINE pointer operator->() const noexcept {
    static_assert((!bmupmu::is_array<T>::value));
    assert(m_data.m_p);
    return m_data.m_p;
  }

  //! <b>Returns</b>: The stored pointer.
  //!
  FORCEINLINE pointer get() const noexcept { return m_data.m_p; }

  //! <b>Returns</b>: A reference to the stored deleter.
  //!
  FORCEINLINE typename bmupmu::add_lvalue_reference<D>::type get_deleter() noexcept {
    return m_data.deleter();
  }

  //! <b>Returns</b>: A reference to the stored deleter.
  //!
  FORCEINLINE typename bmupmu::add_const_lvalue_reference<D>::type
  get_deleter() const noexcept {
    return m_data.deleter();
  }

  //! <b>Returns</b>: Returns: get() != nullptr.
  //!
  FORCEINLINE explicit operator bool() const noexcept {
    return m_data.m_p ? &bmupd::bool_conversion::for_bool : bmupd::explicit_bool_arg(0);
  }
  
  //! <b>Postcondition</b>: <tt>get() == nullptr</tt>.
  //!
  //! <b>Returns</b>: The value <tt>get()</tt> had at the start of the call to release.
  FORCEINLINE pointer release() noexcept {
    const pointer tmp = m_data.m_p;
    m_data.m_p = pointer();
    return tmp;
  }

  //! <b>Requires</b>: The expression <tt>get_deleter()(get())</tt> shall be well formed, shall have well-defined behavior,
  //!   and shall not throw exceptions.
  //!
  //! <b>Effects</b>: assigns p to the stored pointer, and then if the old value of the stored pointer, old_p, was not
  //!   equal to nullptr, calls <tt>get_deleter()(old_p)</tt>. Note: The order of these operations is significant
  //!   because the call to <tt>get_deleter()</tt> may destroy *this.
  //!
  //! <b>Post-conditions</b>: <tt>get() == p</tt>. Note: The postcondition does not hold if the call to <tt>get_deleter()</tt>
  //!   destroys *this since <tt>this->get()</tt> is no longer a valid expression.
  //!
  //! <b>Remarks</b>: This constructor shall not participate in overload resolution unless:
  //!      - If T is not an array type and Pointer is implicitly convertible to pointer.
  //!      - If T is an array type and Pointer is a more CV qualified pointer to element_type.
  template <class Pointer>
  typename bmupd::enable_up_ptr<T, Pointer, pointer, void>::type reset(Pointer p) noexcept {
    //If T is not an array type, element_type_t<Pointer> derives from T
    //it uses the default deleter and T has no virtual destructor, then you have a problem
    static_assert((!bmupd::missing_virtual_destructor<
                   D,
                   typename bmupd::get_element_type<Pointer>::type>::value));
    pointer tmp = m_data.m_p;
    m_data.m_p = p;
    if (tmp) m_data.deleter()(tmp);
  }

  //! <b>Requires</b>: The expression <tt>get_deleter()(get())</tt> shall be well formed, shall have well-defined behavior,
  //!   and shall not throw exceptions.
  //!
  //! <b>Effects</b>: assigns nullptr to the stored pointer, and then if the old value of the stored pointer, old_p, was not
  //!   equal to nullptr, calls <tt>get_deleter()(old_p)</tt>. Note: The order of these operations is significant
  //!   because the call to <tt>get_deleter()</tt> may destroy *this.
  //!
  //! <b>Post-conditions</b>: <tt>get() == p</tt>. Note: The postcondition does not hold if the call to <tt>get_deleter()</tt>
  //!   destroys *this since <tt>this->get()</tt> is no longer a valid expression.
  void reset() noexcept { this->reset(pointer()); }

  //! <b>Effects</b>: Same as <tt>reset()</tt>
  //!
  void reset(bmupd::nullptr_type) noexcept { this->reset(); }

  //! <b>Requires</b>: <tt>get_deleter()</tt> shall be swappable and shall not throw an exception under swap.
  //!
  //! <b>Effects</b>: Invokes swap on the stored pointers and on the stored deleters of *this and u.
  void swap(unique_ptr &u) noexcept {
    ::boost::adl_move_swap(m_data.m_p, u.m_data.m_p);
    ::boost::adl_move_swap(m_data.deleter(), u.m_data.deleter());
  }
};

//! <b>Effects</b>: Calls <tt>x.swap(y)</tt>.
//!
template <class T, class D>
FORCEINLINE void swap(unique_ptr<T, D> &x, unique_ptr<T, D> &y) noexcept {
  x.swap(y);
}

//! <b>Returns</b>: <tt>x.get() == y.get()</tt>.
//!
template <class T1, class D1, class T2, class D2>
FORCEINLINE bool operator==(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y) {
  return x.get() == y.get();
}

//! <b>Returns</b>: <tt>x.get() != y.get()</tt>.
//!
template <class T1, class D1, class T2, class D2>
FORCEINLINE bool operator!=(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y) {
  return x.get() != y.get();
}

//! <b>Returns</b>: x.get() < y.get().
//!
//! <b>Remarks</b>: This comparison shall induce a
//!   strict weak ordering betwen pointers.
template <class T1, class D1, class T2, class D2>
FORCEINLINE bool operator<(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y) {
  return x.get() < y.get();
}

//! <b>Returns</b>: !(y < x).
//!
template <class T1, class D1, class T2, class D2>
FORCEINLINE bool operator<=(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y) {
  return !(y < x);
}

//! <b>Returns</b>: y < x.
//!
template <class T1, class D1, class T2, class D2>
FORCEINLINE bool operator>(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y) {
  return y < x;
}

//! <b>Returns</b>:!(x < y).
//!
template <class T1, class D1, class T2, class D2>
FORCEINLINE bool operator>=(const unique_ptr<T1, D1> &x, const unique_ptr<T2, D2> &y) {
  return !(x < y);
}

//! <b>Returns</b>:!x.
//!
template <class T, class D>
FORCEINLINE bool operator==(const unique_ptr<T, D> &x, bmupd::nullptr_type) noexcept {
  return !x;
}

//! <b>Returns</b>:!x.
//!
template <class T, class D>
FORCEINLINE bool operator==(bmupd::nullptr_type, const unique_ptr<T, D> &x) noexcept {
  return !x;
}

//! <b>Returns</b>: (bool)x.
//!
template <class T, class D>
FORCEINLINE bool operator!=(const unique_ptr<T, D> &x, bmupd::nullptr_type) noexcept {
  return !!x;
}

//! <b>Returns</b>: (bool)x.
//!
template <class T, class D>
FORCEINLINE bool operator!=(bmupd::nullptr_type, const unique_ptr<T, D> &x) noexcept {
  return !!x;
}

//! <b>Requires</b>: <tt>operator </tt> shall induce a strict weak ordering on unique_ptr<T, D>::pointer values.
//!
//! <b>Returns</b>: Returns <tt>x.get() < pointer()</tt>.
template <class T, class D>
FORCEINLINE bool operator<(const unique_ptr<T, D> &x, bmupd::nullptr_type) {
  return x.get() < typename unique_ptr<T, D>::pointer();
}

//! <b>Requires</b>: <tt>operator </tt> shall induce a strict weak ordering on unique_ptr<T, D>::pointer values.
//!
//! <b>Returns</b>: Returns <tt>pointer() < x.get()</tt>.
template <class T, class D>
FORCEINLINE bool operator<(bmupd::nullptr_type, const unique_ptr<T, D> &x) {
  return typename unique_ptr<T, D>::pointer() < x.get();
}

//! <b>Returns</b>: <tt>nullptr < x</tt>.
//!
template <class T, class D>
FORCEINLINE bool operator>(const unique_ptr<T, D> &x, bmupd::nullptr_type) {
  return x.get() > typename unique_ptr<T, D>::pointer();
}

//! <b>Returns</b>: <tt>x < nullptr</tt>.
//!
template <class T, class D>
FORCEINLINE bool operator>(bmupd::nullptr_type, const unique_ptr<T, D> &x) {
  return typename unique_ptr<T, D>::pointer() > x.get();
}

//! <b>Returns</b>: <tt>!(nullptr < x)</tt>.
//!
template <class T, class D>
FORCEINLINE bool operator<=(const unique_ptr<T, D> &x, bmupd::nullptr_type) {
  return !(bmupd::nullptr_type() < x);
}

//! <b>Returns</b>: <tt>!(x < nullptr)</tt>.
//!
template <class T, class D>
FORCEINLINE bool operator<=(bmupd::nullptr_type, const unique_ptr<T, D> &x) {
  return !(x < bmupd::nullptr_type());
}

//! <b>Returns</b>: <tt>!(x < nullptr)</tt>.
//!
template <class T, class D>
FORCEINLINE bool operator>=(const unique_ptr<T, D> &x, bmupd::nullptr_type) {
  return !(x < bmupd::nullptr_type());
}

//! <b>Returns</b>: <tt>!(nullptr < x)</tt>.
//!
template <class T, class D>
FORCEINLINE bool operator>=(bmupd::nullptr_type, const unique_ptr<T, D> &x) {
  return !(bmupd::nullptr_type() < x);
}

} // namespace movelib
} // namespace boost

#if defined _MSC_VER
#  pragma warning(pop)
#endif

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#undef NORETURN
#undef FORCEINLINE

#endif // #ifndef __CANDYBOX_SMART_PTR_HPP__
