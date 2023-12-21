// Copyright 2011, 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "smart_ptr.hpp"
#include "lightweight_test.hpp"

#include <functional>

int
main()
{
	{
		boost::shared_ptr<int> p1, p2(new int);

		BOOST_TEST_EQ(std::hash<boost::shared_ptr<int> >()(p1), std::hash<int*>()(p1.get()));
		BOOST_TEST_EQ(std::hash<boost::shared_ptr<int> >()(p2), std::hash<int*>()(p2.get()));
	}

	{
		boost::shared_ptr<int[]> p1, p2(new int[1]);

		BOOST_TEST_EQ(std::hash<boost::shared_ptr<int[]> >()(p1), std::hash<int*>()(p1.get()));
		BOOST_TEST_EQ(std::hash<boost::shared_ptr<int[]> >()(p2), std::hash<int*>()(p2.get()));
	}

	{
		boost::shared_ptr<int[1]> p1, p2(new int[1]);

		BOOST_TEST_EQ(
		    std::hash<boost::shared_ptr<int[1]> >()(p1), std::hash<int*>()(p1.get()));
		BOOST_TEST_EQ(
		    std::hash<boost::shared_ptr<int[1]> >()(p2), std::hash<int*>()(p2.get()));
	}

	return boost::report_errors();
}
