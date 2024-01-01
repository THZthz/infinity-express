#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"

TEST_CASE("Assignment value", "[assignment.value]")
{
	candybox::optional<int> o1 = 42;
	candybox::optional<int> o2 = 12;
	candybox::optional<int> o3;

	o1 = o1;
	REQUIRE(*o1 == 42);

	o1 = o2;
	REQUIRE(*o1 == 12);

	o1 = o3;
	REQUIRE(!o1);

	o1 = 42;
	REQUIRE(*o1 == 42);

	o1 = candybox::nullopt;
	REQUIRE(!o1);

	o1 = std::move(o2);
	REQUIRE(*o1 == 12);

	candybox::optional<short> o4 = 42;

	o1 = o4;
	REQUIRE(*o1 == 42);

	o1 = std::move(o4);
	REQUIRE(*o1 == 42);
}


TEST_CASE("Assignment reference", "[assignment.ref]")
{
	auto i = 42;
	auto j = 12;

	candybox::optional<int &> o1 = i;
	candybox::optional<int &> o2 = j;
	candybox::optional<int &> o3;

	o1 = o1;
	REQUIRE(*o1 == 42);
	REQUIRE(&*o1 == &i);

	o1 = o2;
	REQUIRE(*o1 == 12);

	o1 = o3;
	REQUIRE(!o1);

	auto k = 42;
	o1 = k;
	REQUIRE(*o1 == 42);
	REQUIRE(*o1 == i);
	REQUIRE(*o1 == k);
	REQUIRE(&*o1 != &i);
	REQUIRE(&*o1 == &k);

	k = 12;
	REQUIRE(*o1 == 12);

	o1 = candybox::nullopt;
	REQUIRE(!o1);

	o1 = std::move(o2);
	REQUIRE(*o1 == 12);
}
