

#if defined(BOOST_MSVC)
#	pragma warning(disable : 4786) // identifier truncated in debug info
#	pragma warning(disable : 4710) // function not inlined
#	pragma warning(disable : 4711) // function selected for automatic inline expansion
#	pragma warning(disable : 4514) // unreferenced inline removed
#endif

//
//  shared_ptr_pv_fail.cpp - a negative test for converting a shared_ptr to void*
//
//  Copyright 2007 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "candybox/smart_ptr.hpp"

void
f(void *)
{
}

int
main()
{
	boost::shared_ptr<int> p;
	f(p); // must fail
	return 0;
}
