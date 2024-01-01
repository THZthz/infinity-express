// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#if defined(__cpp_deduction_guides)

#include "candybox/smart_ptr.hpp"
#	include <memory>

int
main()
{
	boost::shared_ptr p2(std::unique_ptr<int>(new int));
}

#else

int
main()
{
}

#endif
