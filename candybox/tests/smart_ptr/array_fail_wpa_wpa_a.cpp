//
// Copyright (c) 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "candybox/smart_ptr.hpp"

struct X
{
};

struct Y : public X
{
};

int
main()
{
	boost::weak_ptr<Y[]> px;
	boost::weak_ptr<X[]> px2;
	px2 = px;
}
