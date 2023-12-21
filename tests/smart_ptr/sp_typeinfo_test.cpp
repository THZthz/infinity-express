//
// sp_typeinfo_test.cpp
//
// Copyright (c) 2009 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "smart_ptr.hpp"
#include "lightweight_test.hpp"
#include <iostream>

int
main()
{
	BOOST_TEST(typeid(int) == typeid(int));
	BOOST_TEST(typeid(int) != typeid(long));
	BOOST_TEST(typeid(int) != typeid(void));

	std::type_info const &ti = typeid(int);

	std::type_info const *pti = &typeid(int);
	BOOST_TEST(*pti == ti);

	BOOST_TEST(ti == ti);
	BOOST_TEST(!(ti != ti));
	BOOST_TEST(!ti.before(ti));

	char const *nti = ti.name();
	std::cout << nti << std::endl;

	std::type_info const &tv = typeid(void);

	std::type_info const *ptv = &typeid(void);
	BOOST_TEST(*ptv == tv);

	BOOST_TEST(tv == tv);
	BOOST_TEST(!(tv != tv));
	BOOST_TEST(!tv.before(tv));

	char const *ntv = tv.name();
	std::cout << ntv << std::endl;

	BOOST_TEST(ti != tv);
	BOOST_TEST(!(ti == tv));

	BOOST_TEST(ti.before(tv) != tv.before(ti));

	return boost::report_errors();
}
