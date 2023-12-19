#include <catch2/catch_all.hpp>
#include "optional.hpp"

TEST_CASE("Relational ops", "[relops]") {
  ie::optional<int> o1{4};
  ie::optional<int> o2{42};
  ie::optional<int> o3{};

  SECTION("self simple") {
    REQUIRE(!(o1 == o2));
    REQUIRE(o1 == o1);
    REQUIRE(o1 != o2);
    REQUIRE(!(o1 != o1));
    REQUIRE(o1 < o2);
    REQUIRE(!(o1 < o1));
    REQUIRE(!(o1 > o2));
    REQUIRE(!(o1 > o1));
    REQUIRE(o1 <= o2);
    REQUIRE(o1 <= o1);
    REQUIRE(!(o1 >= o2));
    REQUIRE(o1 >= o1);
  }

  SECTION("nullopt simple") {
    REQUIRE(!(o1 == ie::nullopt));
    REQUIRE(!(ie::nullopt == o1));
    REQUIRE(o1 != ie::nullopt);
    REQUIRE(ie::nullopt != o1);
    REQUIRE(!(o1 < ie::nullopt));
    REQUIRE(ie::nullopt < o1);
    REQUIRE(o1 > ie::nullopt);
    REQUIRE(!(ie::nullopt > o1));
    REQUIRE(!(o1 <= ie::nullopt));
    REQUIRE(ie::nullopt <= o1);
    REQUIRE(o1 >= ie::nullopt);
    REQUIRE(!(ie::nullopt >= o1));

    REQUIRE(o3 == ie::nullopt);
    REQUIRE(ie::nullopt == o3);
    REQUIRE(!(o3 != ie::nullopt));
    REQUIRE(!(ie::nullopt != o3));
    REQUIRE(!(o3 < ie::nullopt));
    REQUIRE(!(ie::nullopt < o3));
    REQUIRE(!(o3 > ie::nullopt));
    REQUIRE(!(ie::nullopt > o3));
    REQUIRE(o3 <= ie::nullopt);
    REQUIRE(ie::nullopt <= o3);
    REQUIRE(o3 >= ie::nullopt);
    REQUIRE(ie::nullopt >= o3);
  }

  SECTION("with T simple") {
    REQUIRE(!(o1 == 1));
    REQUIRE(!(1 == o1));
    REQUIRE(o1 != 1);
    REQUIRE(1 != o1);
    REQUIRE(!(o1 < 1));
    REQUIRE(1 < o1);
    REQUIRE(o1 > 1);
    REQUIRE(!(1 > o1));
    REQUIRE(!(o1 <= 1));
    REQUIRE(1 <= o1);
    REQUIRE(o1 >= 1);
    REQUIRE(!(1 >= o1));

    REQUIRE(o1 == 4);
    REQUIRE(4 == o1);
    REQUIRE(!(o1 != 4));
    REQUIRE(!(4 != o1));
    REQUIRE(!(o1 < 4));
    REQUIRE(!(4 < o1));
    REQUIRE(!(o1 > 4));
    REQUIRE(!(4 > o1));
    REQUIRE(o1 <= 4);
    REQUIRE(4 <= o1);
    REQUIRE(o1 >= 4);
    REQUIRE(4 >= o1);
  }

  ie::optional<std::string> o4{"hello"};
  ie::optional<std::string> o5{"xyz"};

  SECTION("self complex") {
    REQUIRE(!(o4 == o5));
    REQUIRE(o4 == o4);
    REQUIRE(o4 != o5);
    REQUIRE(!(o4 != o4));
    REQUIRE(o4 < o5);
    REQUIRE(!(o4 < o4));
    REQUIRE(!(o4 > o5));
    REQUIRE(!(o4 > o4));
    REQUIRE(o4 <= o5);
    REQUIRE(o4 <= o4);
    REQUIRE(!(o4 >= o5));
    REQUIRE(o4 >= o4);
  }

  SECTION("nullopt complex") {
    REQUIRE(!(o4 == ie::nullopt));
    REQUIRE(!(ie::nullopt == o4));
    REQUIRE(o4 != ie::nullopt);
    REQUIRE(ie::nullopt != o4);
    REQUIRE(!(o4 < ie::nullopt));
    REQUIRE(ie::nullopt < o4);
    REQUIRE(o4 > ie::nullopt);
    REQUIRE(!(ie::nullopt > o4));
    REQUIRE(!(o4 <= ie::nullopt));
    REQUIRE(ie::nullopt <= o4);
    REQUIRE(o4 >= ie::nullopt);
    REQUIRE(!(ie::nullopt >= o4));

    REQUIRE(o3 == ie::nullopt);
    REQUIRE(ie::nullopt == o3);
    REQUIRE(!(o3 != ie::nullopt));
    REQUIRE(!(ie::nullopt != o3));
    REQUIRE(!(o3 < ie::nullopt));
    REQUIRE(!(ie::nullopt < o3));
    REQUIRE(!(o3 > ie::nullopt));
    REQUIRE(!(ie::nullopt > o3));
    REQUIRE(o3 <= ie::nullopt);
    REQUIRE(ie::nullopt <= o3);
    REQUIRE(o3 >= ie::nullopt);
    REQUIRE(ie::nullopt >= o3);
  }

  SECTION("with T complex") {
    REQUIRE(!(o4 == "a"));
    REQUIRE(!("a" == o4));
    REQUIRE(o4 != "a");
    REQUIRE("a" != o4);
    REQUIRE(!(o4 < "a"));
    REQUIRE("a" < o4);
    REQUIRE(o4 > "a");
    REQUIRE(!("a" > o4));
    REQUIRE(!(o4 <= "a"));
    REQUIRE("a" <= o4);
    REQUIRE(o4 >= "a");
    REQUIRE(!("a" >= o4));

    REQUIRE(o4 == "hello");
    REQUIRE("hello" == o4);
    REQUIRE(!(o4 != "hello"));
    REQUIRE(!("hello" != o4));
    REQUIRE(!(o4 < "hello"));
    REQUIRE(!("hello" < o4));
    REQUIRE(!(o4 > "hello"));
    REQUIRE(!("hello" > o4));
    REQUIRE(o4 <= "hello");
    REQUIRE("hello" <= o4);
    REQUIRE(o4 >= "hello");
    REQUIRE("hello" >= o4);
  }
}
