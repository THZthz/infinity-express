#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"
#include <vector>

struct foo
{
	foo() = default;
	foo(foo &) = delete;
	foo(foo &&){};
};

TEST_CASE("Constructors", "[constructors]")
{
	ie::optional<int> o1;
	REQUIRE(!o1);

	ie::optional<int> o2 = ie::nullopt;
	REQUIRE(!o2);

	ie::optional<int> o3 = 42;
	REQUIRE(*o3 == 42);

	ie::optional<int> o4 = o3;
	REQUIRE(*o4 == 42);

	ie::optional<int> o5 = o1;
	REQUIRE(!o5);

	ie::optional<int> o6 = std::move(o3);
	REQUIRE(*o6 == 42);

	ie::optional<short> o7 = 42;
	REQUIRE(*o7 == 42);

	ie::optional<int> o8 = o7;
	REQUIRE(*o8 == 42);

	ie::optional<int> o9 = std::move(o7);
	REQUIRE(*o9 == 42);

	{
		ie::optional<int &> o;
		REQUIRE(!o);

		ie::optional<int &> oo = o;
		REQUIRE(!oo);
	}

	{
		auto i = 42;
		ie::optional<int &> o = i;
		REQUIRE(o);
		REQUIRE(*o == 42);

		ie::optional<int &> oo = o;
		REQUIRE(oo);
		REQUIRE(*oo == 42);
	}

	std::vector<foo> v;
	v.emplace_back();
	ie::optional<std::vector<foo>> ov = std::move(v);
	REQUIRE(ov->size() == 1);
}
