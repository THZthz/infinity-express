//  make_shared_array_tmp_test.cpp
//
//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "smart_ptr.hpp"
#include "lightweight_test.hpp"

struct X
{
	static int destroyed;

	~X() { ++destroyed; }
};

int X::destroyed = 0;

int
main()
{
	{
		X::destroyed = 0;

		boost::make_shared<X[3]>();

		BOOST_TEST_EQ(X::destroyed, 3);
	}

	{
		X::destroyed = 0;

		boost::make_shared<X[]>(3);

		BOOST_TEST_EQ(X::destroyed, 3);
	}

	return boost::report_errors();
}
