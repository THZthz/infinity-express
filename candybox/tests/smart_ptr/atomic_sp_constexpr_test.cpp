//
// atomic_sp_constexpr_test.cpp
//
// Copyright 2017, 2018 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//


#include "smart_ptr.hpp"
#include "lightweight_test.hpp"

struct X
{
};

struct Z
{
	Z();
};

static Z z;

static boost::atomic_shared_ptr<X> p1;

Z::Z() { p1 = boost::shared_ptr<X>(new X); }

int
main()
{
	boost::shared_ptr<X> p2 = p1;

	BOOST_TEST(p2.get() != 0);
	BOOST_TEST_EQ(p2.use_count(), 2);

	return boost::report_errors();
}
