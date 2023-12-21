#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"

TEST_CASE("Swap value", "[swap.value]")
{
	ie::optional<int> o1 = 42;
	ie::optional<int> o2 = 12;
	o1.swap(o2);
	CHECK(o1.value() == 12);
	CHECK(o2.value() == 42);
}

TEST_CASE("Swap value with null intialized", "[swap.value_nullopt]")
{
	ie::optional<int> o1 = 42;
	ie::optional<int> o2 = ie::nullopt;
	o1.swap(o2);
	CHECK(!o1.has_value());
	CHECK(o2.value() == 42);
}

TEST_CASE("Swap null intialized with value", "[swap.nullopt_value]")
{
	ie::optional<int> o1 = ie::nullopt;
	ie::optional<int> o2 = 42;
	o1.swap(o2);
	CHECK(o1.value() == 42);
	CHECK(!o2.has_value());
}
