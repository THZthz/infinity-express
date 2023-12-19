// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#if defined(__cpp_deduction_guides)

#	include "smart_ptr.hpp"

int
main()
{
	boost::shared_ptr<int> p1(new int);
	boost::weak_ptr p2(p1);
}

#else

int
main()
{
	return 0;
}

#endif
