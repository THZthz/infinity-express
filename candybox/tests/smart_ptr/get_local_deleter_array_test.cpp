//
// get_local_deleter_array_test2.cpp
//
// Copyright 2002, 2011, 2017 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "candybox/smart_ptr.hpp"
#include "lightweight_test.hpp"

struct deleter
{
	int data;

	deleter() : data(0) { }

	void operator()(void *) { BOOST_TEST(data == 17041); }
};

struct deleter2
{
};

struct X
{
};

int
main()
{
	{
		boost::local_shared_ptr<X[]> p;

		BOOST_TEST(boost::get_deleter<void>(p) == 0);
		BOOST_TEST(boost::get_deleter<void const>(p) == 0);
		BOOST_TEST(boost::get_deleter<int>(p) == 0);
		BOOST_TEST(boost::get_deleter<int const>(p) == 0);
		BOOST_TEST(boost::get_deleter<X>(p) == 0);
		BOOST_TEST(boost::get_deleter<X const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2 const>(p) == 0);
	}

	{
		boost::local_shared_ptr<X[1]> p;

		BOOST_TEST(boost::get_deleter<void>(p) == 0);
		BOOST_TEST(boost::get_deleter<void const>(p) == 0);
		BOOST_TEST(boost::get_deleter<int>(p) == 0);
		BOOST_TEST(boost::get_deleter<int const>(p) == 0);
		BOOST_TEST(boost::get_deleter<X>(p) == 0);
		BOOST_TEST(boost::get_deleter<X const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2 const>(p) == 0);
	}

	{
		boost::local_shared_ptr<X[]> p(new X[1]);

		BOOST_TEST(boost::get_deleter<void>(p) == 0);
		BOOST_TEST(boost::get_deleter<void const>(p) == 0);
		BOOST_TEST(boost::get_deleter<int>(p) == 0);
		BOOST_TEST(boost::get_deleter<int const>(p) == 0);
		BOOST_TEST(boost::get_deleter<X>(p) == 0);
		BOOST_TEST(boost::get_deleter<X const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2 const>(p) == 0);
	}

	{
		boost::local_shared_ptr<X[1]> p(new X[1]);

		BOOST_TEST(boost::get_deleter<void>(p) == 0);
		BOOST_TEST(boost::get_deleter<void const>(p) == 0);
		BOOST_TEST(boost::get_deleter<int>(p) == 0);
		BOOST_TEST(boost::get_deleter<int const>(p) == 0);
		BOOST_TEST(boost::get_deleter<X>(p) == 0);
		BOOST_TEST(boost::get_deleter<X const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2 const>(p) == 0);
	}

	{
		X x[1];
		boost::local_shared_ptr<X[]> p(x, deleter());

		BOOST_TEST(boost::get_deleter<void>(p) == 0);
		BOOST_TEST(boost::get_deleter<void const>(p) == 0);
		BOOST_TEST(boost::get_deleter<int>(p) == 0);
		BOOST_TEST(boost::get_deleter<int const>(p) == 0);
		BOOST_TEST(boost::get_deleter<X>(p) == 0);
		BOOST_TEST(boost::get_deleter<X const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2 const>(p) == 0);

		deleter *q = boost::get_deleter<deleter>(p);

		BOOST_TEST(q != 0);
		BOOST_TEST(q->data == 0);

		q->data = 17041;

		deleter const *r = boost::get_deleter<deleter const>(p);

		BOOST_TEST(r == q);
		BOOST_TEST(r->data == 17041);
	}

	{
		X x[1];
		boost::local_shared_ptr<X[1]> p(x, deleter());

		BOOST_TEST(boost::get_deleter<void>(p) == 0);
		BOOST_TEST(boost::get_deleter<void const>(p) == 0);
		BOOST_TEST(boost::get_deleter<int>(p) == 0);
		BOOST_TEST(boost::get_deleter<int const>(p) == 0);
		BOOST_TEST(boost::get_deleter<X>(p) == 0);
		BOOST_TEST(boost::get_deleter<X const>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2>(p) == 0);
		BOOST_TEST(boost::get_deleter<deleter2 const>(p) == 0);

		deleter *q = boost::get_deleter<deleter>(p);

		BOOST_TEST(q != 0);
		BOOST_TEST(q->data == 0);

		q->data = 17041;

		deleter const *r = boost::get_deleter<deleter const>(p);

		BOOST_TEST(r == q);
		BOOST_TEST(r->data == 17041);
	}

	return boost::report_errors();
}
