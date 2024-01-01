//
// sp_constexpr_test.cpp
//
// Copyright 2017 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//



#include "candybox/smart_ptr.hpp"
#include "lightweight_test.hpp"

struct X : public boost::enable_shared_from_this<X>
{
};

struct Z
{
	Z();
};

static Z z;

static boost::shared_ptr<X> p1;
static boost::weak_ptr<X> p2;

static boost::shared_ptr<X> p3(nullptr);

Z::Z()
{
	p1.reset(new X);
	p2 = p1;
#if !defined(BOOST_NO_CXX11_NULLPTR)
	p3.reset(new X);
#endif
}

int
main()
{
	BOOST_TEST(p1.get() != 0);
	BOOST_TEST_EQ(p1.use_count(), 1);

	BOOST_TEST_EQ(p2.use_count(), 1);
	BOOST_TEST_EQ(p2.lock(), p1);

	BOOST_TEST(p3.get() != 0);
	BOOST_TEST_EQ(p3.use_count(), 1);

	return boost::report_errors();
}
