// Copyright 2018, 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt)

#include "candybox/smart_ptr.hpp"
#include "lightweight_test.hpp"

static int count = 0;

void
f(int n)
{
	for (int i = 0; i < n; ++i)
	{
		boost::detail::spinlock_pool<0>::scoped_lock lock(&count);
		++count;
	}
}

int
main()
{
	int const N = 100000; // iterations
	int const M = 8; // threads

	boost::detail::lw_thread_t th[M] = {};

	for (int i = 0; i < M; ++i)
	{
		boost::detail::lw_thread_create(th[i], [N] { return f(N); });
	}

	for (int i = 0; i < M; ++i) { boost::detail::lw_thread_join(th[i]); }

	BOOST_TEST_EQ(count, N * M);

	return boost::report_errors();
}
