#include <catch2/catch_all.hpp>
#include "optional.hpp"

TEST_CASE("Nullopt", "[nullopt]") {
  ie::optional<int> o1 = ie::nullopt;
  ie::optional<int> o2{ie::nullopt};
  ie::optional<int> o3(ie::nullopt);
  ie::optional<int> o4 = {ie::nullopt};

  REQUIRE(!o1);
  REQUIRE(!o2);
  REQUIRE(!o3);
  REQUIRE(!o4);

  REQUIRE(!std::is_default_constructible<tl::nullopt_t>::value);
}
