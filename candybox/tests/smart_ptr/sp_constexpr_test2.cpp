//
// sp_constexpr_test2.cpp
//
// Copyright 2017 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//


#include "smart_ptr.hpp"
#include "lightweight_test.hpp"

struct X : public boost::enable_shared_from_this<X>
{
	int v_;

	constexpr X() noexcept : v_(1) { }
};

struct Z
{
	Z();
};

static Z z;
static X x;

Z::Z() { BOOST_TEST_EQ(x.v_, 1); }

int
main()
{
	return boost::report_errors();
}
