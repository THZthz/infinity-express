#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"

TEST_CASE("Constexpr", "[constexpr]")
{
#if !defined(TL_OPTIONAL_MSVC2015) && defined(TL_OPTIONAL_CXX14)
	SECTION("empty construct")
	{
		constexpr ie::optional<int> o2{};
		constexpr ie::optional<int> o3 = {};
		constexpr ie::optional<int> o4 = ie::nullopt;
		constexpr ie::optional<int> o5 = {ie::nullopt};
		constexpr ie::optional<int> o6(ie::nullopt);

		STATIC_REQUIRE(!o2);
		STATIC_REQUIRE(!o3);
		STATIC_REQUIRE(!o4);
		STATIC_REQUIRE(!o5);
		STATIC_REQUIRE(!o6);
	}

	SECTION("value construct")
	{
		constexpr ie::optional<int> o1 = 42;
		constexpr ie::optional<int> o2{42};
		constexpr ie::optional<int> o3(42);
		constexpr ie::optional<int> o4 = {42};
		constexpr int i = 42;
		constexpr ie::optional<int> o5 = std::move(i);
		constexpr ie::optional<int> o6{std::move(i)};
		constexpr ie::optional<int> o7(std::move(i));
		constexpr ie::optional<int> o8 = {std::move(i)};

		STATIC_REQUIRE(*o1 == 42);
		STATIC_REQUIRE(*o2 == 42);
		STATIC_REQUIRE(*o3 == 42);
		STATIC_REQUIRE(*o4 == 42);
		STATIC_REQUIRE(*o5 == 42);
		STATIC_REQUIRE(*o6 == 42);
		STATIC_REQUIRE(*o7 == 42);
		STATIC_REQUIRE(*o8 == 42);
	}
#endif
}
