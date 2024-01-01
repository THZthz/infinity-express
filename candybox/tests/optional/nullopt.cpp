#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"

TEST_CASE("Nullopt", "[nullopt]")
{
	candybox::optional<int> o1 = candybox::nullopt;
	candybox::optional<int> o2{candybox::nullopt};
	candybox::optional<int> o3(candybox::nullopt);
	candybox::optional<int> o4 = {candybox::nullopt};

	REQUIRE(!o1);
	REQUIRE(!o2);
	REQUIRE(!o3);
	REQUIRE(!o4);

	REQUIRE(!std::is_default_constructible<tl::nullopt_t>::value);
}
