

//  sp_convertible_test2.cpp
//
//  Copyright 2012, 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "candybox/smart_ptr.hpp"
#include "lightweight_test.hpp"

//

class X;

class B
{
};

class D : public B
{
};

using boost::is_convertible;

#define TEST_CV_TRUE_(S1, T, S2, U)                                                           \
	BOOST_TEST((is_convertible<S1<T>, S2<U> >::value == true));                               \
	BOOST_TEST((is_convertible<S1<T>, S2<const U> >::value == true));                         \
	BOOST_TEST((is_convertible<S1<T>, S2<volatile U> >::value == true));                      \
	BOOST_TEST((is_convertible<S1<T>, S2<const volatile U> >::value == true));                \
	BOOST_TEST((is_convertible<S1<const T>, S2<U> >::value == false));                        \
	BOOST_TEST((is_convertible<S1<const T>, S2<const U> >::value == true));                   \
	BOOST_TEST((is_convertible<S1<const T>, S2<volatile U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<const T>, S2<const volatile U> >::value == true));          \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<U> >::value == false));                     \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<const U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<volatile U> >::value == true));             \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<const volatile U> >::value == true));       \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<const U> >::value == false));         \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<volatile U> >::value == false));      \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<const volatile U> >::value == true));

#define TEST_CV_FALSE_(S1, T, S2, U)                                                          \
	BOOST_TEST((is_convertible<S1<T>, S2<U> >::value == false));                              \
	BOOST_TEST((is_convertible<S1<T>, S2<const U> >::value == false));                        \
	BOOST_TEST((is_convertible<S1<T>, S2<volatile U> >::value == false));                     \
	BOOST_TEST((is_convertible<S1<T>, S2<const volatile U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<const T>, S2<U> >::value == false));                        \
	BOOST_TEST((is_convertible<S1<const T>, S2<const U> >::value == false));                  \
	BOOST_TEST((is_convertible<S1<const T>, S2<volatile U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<const T>, S2<const volatile U> >::value == false));         \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<U> >::value == false));                     \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<const U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<volatile U> >::value == false));            \
	BOOST_TEST((is_convertible<S1<volatile T>, S2<const volatile U> >::value == false));      \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<U> >::value == false));               \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<const U> >::value == false));         \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<volatile U> >::value == false));      \
	BOOST_TEST((is_convertible<S1<const volatile T>, S2<const volatile U> >::value == false));

using boost::shared_ptr;
using boost::weak_ptr;

#define TEST_CV_TRUE(T, U)                                                                    \
	TEST_CV_TRUE_(shared_ptr, T, shared_ptr, U)                                               \
	TEST_CV_TRUE_(shared_ptr, T, weak_ptr, U)                                                 \
	TEST_CV_TRUE_(weak_ptr, T, weak_ptr, U)

#define TEST_CV_FALSE(T, U)                                                                   \
	TEST_CV_FALSE_(shared_ptr, T, shared_ptr, U)                                              \
	TEST_CV_FALSE_(shared_ptr, T, weak_ptr, U)                                                \
	TEST_CV_FALSE_(weak_ptr, T, weak_ptr, U)

int
main()
{
#if !defined(BOOST_SP_NO_SP_CONVERTIBLE)

	TEST_CV_TRUE(X, X)
	TEST_CV_TRUE(X, void)
	TEST_CV_FALSE(void, X)
	TEST_CV_TRUE(D, B)
	TEST_CV_FALSE(B, D)

	TEST_CV_TRUE(X[], X[])
	TEST_CV_FALSE(D[], B[])

	TEST_CV_TRUE(X[3], X[3])
	TEST_CV_FALSE(X[3], X[4])
	TEST_CV_FALSE(D[3], B[3])

	TEST_CV_TRUE(X[3], X[])
	TEST_CV_FALSE(X[], X[3])

	TEST_CV_TRUE(X[], void)
	TEST_CV_FALSE(void, X[])

	TEST_CV_TRUE(X[3], void)
	TEST_CV_FALSE(void, X[3])

#endif

	return boost::report_errors();
}
