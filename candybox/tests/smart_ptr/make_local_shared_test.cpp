// make_local_shared_test.cpp
//
// Copyright 2007-2009, 2017 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt



#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

int
main()
{
}

#else

#	include "lightweight_test.hpp"
#include "candybox/smart_ptr.hpp"
#include "candybox/smart_ptr.hpp"
#include "candybox/smart_ptr.hpp"
#	include <cstddef>

class X
{
private:
	X(X const &);
	X &operator=(X const &);

	void *operator new(std::size_t n)
	{
		// lack of this definition causes link errors on Comeau C++
		BOOST_ERROR("private X::new called");
		return ::operator new(n);
	}

	void operator delete(void *p)
	{
		// lack of this definition causes link errors on MSVC
		BOOST_ERROR("private X::delete called");
		::operator delete(p);
	}

public:
	static int instances;

	int v;

	explicit X(
	    int a1 = 0,
	    int a2 = 0,
	    int a3 = 0,
	    int a4 = 0,
	    int a5 = 0,
	    int a6 = 0,
	    int a7 = 0,
	    int a8 = 0,
	    int a9 = 0)
	    : v(a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9)
	{
		++instances;
	}

	~X() { --instances; }
};

int X::instances = 0;

template <class T>
static long
use_count(boost::local_shared_ptr<T> const &p)
{
	return boost::shared_ptr<T>(p).use_count() - 1;
}

int
main()
{
	{
		boost::local_shared_ptr<int> pi = boost::make_local_shared<int>();

		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);

		BOOST_TEST(*pi == 0);
	}

	{
		boost::local_shared_ptr<int> pi = boost::make_local_shared_noinit<int>();

		BOOST_TEST(pi.get() != 0);
	}

	{
		boost::local_shared_ptr<int> pi = boost::make_local_shared<int>(5);

		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);

		BOOST_TEST(*pi == 5);
	}

	BOOST_TEST(X::instances == 0);

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>();
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 0);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared_noinit<X>();
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 0);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3, 4);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3 + 4);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3, 4, 5);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3 + 4 + 5);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3, 4, 5, 6);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3 + 4 + 5 + 6);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3, 4, 5, 6, 7);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3 + 4 + 5 + 6 + 7);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3, 4, 5, 6, 7, 8);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	{
		boost::local_shared_ptr<X> pi = boost::make_local_shared<X>(1, 2, 3, 4, 5, 6, 7, 8, 9);
		boost::weak_ptr<X> wp(pi);

		BOOST_TEST(X::instances == 1);
		BOOST_TEST(pi.get() != 0);
		BOOST_TEST(pi.local_use_count() == 1);
		BOOST_TEST(use_count(pi) == 1);
		BOOST_TEST(pi->v == 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9);

		pi.reset();

		BOOST_TEST(X::instances == 0);
	}

	return boost::report_errors();
}

#endif
