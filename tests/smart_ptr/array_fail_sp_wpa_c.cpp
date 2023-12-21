//
// Copyright (c) 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "smart_ptr.hpp"
#include "smart_ptr.hpp"

struct X
{
};

int
main()
{
	boost::shared_ptr<X> px;
	boost::weak_ptr<X[]> px2(px);
}
