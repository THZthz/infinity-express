//
// sp_nothrow_test.cpp
//
// Copyright 2016 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//



#include "smart_ptr.hpp"
#include <type_traits>
#include "lightweight_test.hpp"

template <class T>
void
test_copy()
{
	assert(std::is_nothrow_copy_constructible<T>());
	assert(std::is_nothrow_copy_assignable<T>());
}

template <class T>
void
test_move()
{
	assert(std::is_nothrow_move_constructible<T>());
	assert(std::is_nothrow_move_assignable<T>());
}

template <class T>
void
test_default()
{
	assert((std::is_nothrow_default_constructible<T>()));
}

template <class T>
void
test_destroy()
{
	assert((std::is_nothrow_destructible<T>()));
}

template <class T>
void
test_cmd()
{
	test_copy<T>();
	test_move<T>();
	test_default<T>();
}

struct X
{
};

struct Y : public boost::enable_shared_from_this<Y>
{
};

int
main()
{
	test_cmd<boost::shared_ptr<X> >();
	test_cmd<boost::shared_array<X> >();
	test_cmd<boost::weak_ptr<X> >();

	test_copy<Y>();
	test_default<Y>();
	test_destroy<Y>();

	// test_move< Y >();
	assert((std::is_nothrow_move_constructible<Y>()));

#if !(defined(BOOST_MSVC) && BOOST_MSVC == 1700)

	assert((std::is_nothrow_move_assignable<Y>()));

#endif

	test_default<boost::scoped_ptr<X> >();
	test_default<boost::scoped_array<X> >();

	test_move<boost::intrusive_ptr<X> >();
	test_default<boost::intrusive_ptr<X> >();

	return boost::report_errors();
}
