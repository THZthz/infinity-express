//
//  make_shared_fp_test.cpp
//
//  Copyright 2010 Georg Fritzsche
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include "candybox/smart_ptr.hpp"
#include "candybox/smart_ptr.hpp"

int
main()
{
	typedef boost::shared_ptr<int> (*FP)();
	FP fp = boost::make_shared<int>;
}
