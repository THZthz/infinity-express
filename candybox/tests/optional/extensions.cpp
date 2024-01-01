#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"
#include <string>

constexpr int
get_int(int)
{
	return 42;
}
constexpr candybox::optional<int>
get_opt_int(int)
{
	return 42;
}

// What is Clang Format up to?!
TEST_CASE("Monadic operations", "[monadic]")
{
	SECTION("map")
	{ // lhs is empty
		candybox::optional<int> o1;
		auto o1r = o1.map([](int i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o1r), candybox::optional<int>>::value));
		REQUIRE(!o1r);

		// lhs has value
		candybox::optional<int> o2 = 40;
		auto o2r = o2.map([](int i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o2r), candybox::optional<int>>::value));
		REQUIRE(o2r.value() == 42);

		struct rval_call_map
		{
			double operator()(int) && { return 42.0; };
		};

		// ensure that function object is forwarded
		candybox::optional<int> o3 = 42;
		auto o3r = o3.map(rval_call_map{});
		STATIC_REQUIRE((std::is_same<decltype(o3r), candybox::optional<double>>::value));
		REQUIRE(o3r.value() == 42);

		// ensure that lhs is forwarded
		candybox::optional<int> o4 = 40;
		auto o4r = std::move(o4).map([](int &&i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o4r), candybox::optional<int>>::value));
		REQUIRE(o4r.value() == 42);

		// ensure that lhs is const-propagated
		const candybox::optional<int> o5 = 40;
		auto o5r = o5.map([](const int &i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o5r), candybox::optional<int>>::value));
		REQUIRE(o5r.value() == 42);

		// test void return
		candybox::optional<int> o7 = 40;
		auto f7 = [](const int &) { return; };
		auto o7r = o7.map(f7);
		STATIC_REQUIRE((std::is_same<decltype(o7r), candybox::optional<candybox::monostate>>::value));
		REQUIRE(o7r.has_value());

		// test each overload in turn
		candybox::optional<int> o8 = 42;
		auto o8r = o8.map([](int) { return 42; });
		REQUIRE(*o8r == 42);

		candybox::optional<int> o9 = 42;
		auto o9r = o9.map([](int) { return; });
		REQUIRE(o9r);

		candybox::optional<int> o12 = 42;
		auto o12r = std::move(o12).map([](int) { return 42; });
		REQUIRE(*o12r == 42);

		candybox::optional<int> o13 = 42;
		auto o13r = std::move(o13).map([](int) { return; });
		REQUIRE(o13r);

		const candybox::optional<int> o16 = 42;
		auto o16r = o16.map([](int) { return 42; });
		REQUIRE(*o16r == 42);

		const candybox::optional<int> o17 = 42;
		auto o17r = o17.map([](int) { return; });
		REQUIRE(o17r);

		const candybox::optional<int> o20 = 42;
		auto o20r = std::move(o20).map([](int) { return 42; });
		REQUIRE(*o20r == 42);

		const candybox::optional<int> o21 = 42;
		auto o21r = std::move(o21).map([](int) { return; });
		REQUIRE(o21r);

		candybox::optional<int> o24 = candybox::nullopt;
		auto o24r = o24.map([](int) { return 42; });
		REQUIRE(!o24r);

		candybox::optional<int> o25 = candybox::nullopt;
		auto o25r = o25.map([](int) { return; });
		REQUIRE(!o25r);

		candybox::optional<int> o28 = candybox::nullopt;
		auto o28r = std::move(o28).map([](int) { return 42; });
		REQUIRE(!o28r);

		candybox::optional<int> o29 = candybox::nullopt;
		auto o29r = std::move(o29).map([](int) { return; });
		REQUIRE(!o29r);

		const candybox::optional<int> o32 = candybox::nullopt;
		auto o32r = o32.map([](int) { return 42; });
		REQUIRE(!o32r);

		const candybox::optional<int> o33 = candybox::nullopt;
		auto o33r = o33.map([](int) { return; });
		REQUIRE(!o33r);

		const candybox::optional<int> o36 = candybox::nullopt;
		auto o36r = std::move(o36).map([](int) { return 42; });
		REQUIRE(!o36r);

		const candybox::optional<int> o37 = candybox::nullopt;
		auto o37r = std::move(o37).map([](int) { return; });
		REQUIRE(!o37r);

		// callable which returns a reference
		candybox::optional<int> o38 = 42;
		auto o38r = o38.map([](int &i) -> const int & { return i; });
		REQUIRE(o38r);
		REQUIRE(*o38r == 42);

		int i = 42;
		candybox::optional<int &> o39 = i;
		o39.map([](int &x) { x = 12; });
		REQUIRE(i == 12);
	}

	SECTION("map constexpr")
	{
#if !defined(_MSC_VER) && defined(TL_OPTIONAL_CXX14)
		// test each overload in turn
		constexpr candybox::optional<int> o16 = 42;
		constexpr auto o16r = o16.map(get_int);
		STATIC_REQUIRE(*o16r == 42);

		constexpr candybox::optional<int> o20 = 42;
		constexpr auto o20r = std::move(o20).map(get_int);
		STATIC_REQUIRE(*o20r == 42);

		constexpr candybox::optional<int> o32 = candybox::nullopt;
		constexpr auto o32r = o32.map(get_int);
		STATIC_REQUIRE(!o32r);
		constexpr candybox::optional<int> o36 = candybox::nullopt;
		constexpr auto o36r = std::move(o36).map(get_int);
		STATIC_REQUIRE(!o36r);
#endif
	}

	SECTION("transform")
	{ // lhs is empty
		candybox::optional<int> o1;
		auto o1r = o1.transform([](int i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o1r), candybox::optional<int>>::value));
		REQUIRE(!o1r);

		// lhs has value
		candybox::optional<int> o2 = 40;
		auto o2r = o2.transform([](int i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o2r), candybox::optional<int>>::value));
		REQUIRE(o2r.value() == 42);

		struct rval_call_transform
		{
			double operator()(int) && { return 42.0; };
		};

		// ensure that function object is forwarded
		candybox::optional<int> o3 = 42;
		auto o3r = o3.transform(rval_call_transform{});
		STATIC_REQUIRE((std::is_same<decltype(o3r), candybox::optional<double>>::value));
		REQUIRE(o3r.value() == 42);

		// ensure that lhs is forwarded
		candybox::optional<int> o4 = 40;
		auto o4r = std::move(o4).transform([](int &&i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o4r), candybox::optional<int>>::value));
		REQUIRE(o4r.value() == 42);

		// ensure that lhs is const-propagated
		const candybox::optional<int> o5 = 40;
		auto o5r = o5.transform([](const int &i) { return i + 2; });
		STATIC_REQUIRE((std::is_same<decltype(o5r), candybox::optional<int>>::value));
		REQUIRE(o5r.value() == 42);

		// test void return
		candybox::optional<int> o7 = 40;
		auto f7 = [](const int &) { return; };
		auto o7r = o7.transform(f7);
		STATIC_REQUIRE((std::is_same<decltype(o7r), candybox::optional<candybox::monostate>>::value));
		REQUIRE(o7r.has_value());

		// test each overload in turn
		candybox::optional<int> o8 = 42;
		auto o8r = o8.transform([](int) { return 42; });
		REQUIRE(*o8r == 42);

		candybox::optional<int> o9 = 42;
		auto o9r = o9.transform([](int) { return; });
		REQUIRE(o9r);

		candybox::optional<int> o12 = 42;
		auto o12r = std::move(o12).transform([](int) { return 42; });
		REQUIRE(*o12r == 42);

		candybox::optional<int> o13 = 42;
		auto o13r = std::move(o13).transform([](int) { return; });
		REQUIRE(o13r);

		const candybox::optional<int> o16 = 42;
		auto o16r = o16.transform([](int) { return 42; });
		REQUIRE(*o16r == 42);

		const candybox::optional<int> o17 = 42;
		auto o17r = o17.transform([](int) { return; });
		REQUIRE(o17r);

		const candybox::optional<int> o20 = 42;
		auto o20r = std::move(o20).transform([](int) { return 42; });
		REQUIRE(*o20r == 42);

		const candybox::optional<int> o21 = 42;
		auto o21r = std::move(o21).transform([](int) { return; });
		REQUIRE(o21r);

		candybox::optional<int> o24 = candybox::nullopt;
		auto o24r = o24.transform([](int) { return 42; });
		REQUIRE(!o24r);

		candybox::optional<int> o25 = candybox::nullopt;
		auto o25r = o25.transform([](int) { return; });
		REQUIRE(!o25r);

		candybox::optional<int> o28 = candybox::nullopt;
		auto o28r = std::move(o28).transform([](int) { return 42; });
		REQUIRE(!o28r);

		candybox::optional<int> o29 = candybox::nullopt;
		auto o29r = std::move(o29).transform([](int) { return; });
		REQUIRE(!o29r);

		const candybox::optional<int> o32 = candybox::nullopt;
		auto o32r = o32.transform([](int) { return 42; });
		REQUIRE(!o32r);

		const candybox::optional<int> o33 = candybox::nullopt;
		auto o33r = o33.transform([](int) { return; });
		REQUIRE(!o33r);

		const candybox::optional<int> o36 = candybox::nullopt;
		auto o36r = std::move(o36).transform([](int) { return 42; });
		REQUIRE(!o36r);

		const candybox::optional<int> o37 = candybox::nullopt;
		auto o37r = std::move(o37).transform([](int) { return; });
		REQUIRE(!o37r);

		// callable which returns a reference
		candybox::optional<int> o38 = 42;
		auto o38r = o38.transform([](int &i) -> const int & { return i; });
		REQUIRE(o38r);
		REQUIRE(*o38r == 42);

		int i = 42;
		candybox::optional<int &> o39 = i;
		o39.transform([](int &x) { x = 12; });
		REQUIRE(i == 12);
	}

	SECTION("transform constexpr")
	{
#if !defined(_MSC_VER) && defined(TL_OPTIONAL_CXX14)
		// test each overload in turn
		constexpr candybox::optional<int> o16 = 42;
		constexpr auto o16r = o16.transform(get_int);
		STATIC_REQUIRE(*o16r == 42);

		constexpr candybox::optional<int> o20 = 42;
		constexpr auto o20r = std::move(o20).transform(get_int);
		STATIC_REQUIRE(*o20r == 42);

		constexpr candybox::optional<int> o32 = candybox::nullopt;
		constexpr auto o32r = o32.transform(get_int);
		STATIC_REQUIRE(!o32r);
		constexpr candybox::optional<int> o36 = candybox::nullopt;
		constexpr auto o36r = std::move(o36).transform(get_int);
		STATIC_REQUIRE(!o36r);
#endif
	}

	SECTION("and_then")
	{
		// lhs is empty
		candybox::optional<int> o1;
		auto o1r = o1.and_then([](int) { return candybox::optional<float>{42}; });
		STATIC_REQUIRE((std::is_same<decltype(o1r), candybox::optional<float>>::value));
		REQUIRE(!o1r);

		// lhs has value
		candybox::optional<int> o2 = 12;
		auto o2r = o2.and_then([](int) { return candybox::optional<float>{42}; });
		STATIC_REQUIRE((std::is_same<decltype(o2r), candybox::optional<float>>::value));
		REQUIRE(o2r.value() == 42.f);

		// lhs is empty, rhs returns empty
		candybox::optional<int> o3;
		auto o3r = o3.and_then([](int) { return candybox::optional<float>{}; });
		STATIC_REQUIRE((std::is_same<decltype(o3r), candybox::optional<float>>::value));
		REQUIRE(!o3r);

		// rhs returns empty
		candybox::optional<int> o4 = 12;
		auto o4r = o4.and_then([](int) { return candybox::optional<float>{}; });
		STATIC_REQUIRE((std::is_same<decltype(o4r), candybox::optional<float>>::value));
		REQUIRE(!o4r);

		struct rval_call_and_then
		{
			candybox::optional<double> operator()(int) && { return candybox::optional<double>(42.0); };
		};

		// ensure that function object is forwarded
		candybox::optional<int> o5 = 42;
		auto o5r = o5.and_then(rval_call_and_then{});
		STATIC_REQUIRE((std::is_same<decltype(o5r), candybox::optional<double>>::value));
		REQUIRE(o5r.value() == 42);

		// ensure that lhs is forwarded
		candybox::optional<int> o6 = 42;
		auto o6r = std::move(o6).and_then([](int &&i) { return candybox::optional<double>(i); });
		STATIC_REQUIRE((std::is_same<decltype(o6r), candybox::optional<double>>::value));
		REQUIRE(o6r.value() == 42);

		// ensure that function object is const-propagated
		const candybox::optional<int> o7 = 42;
		auto o7r = o7.and_then([](const int &i) { return candybox::optional<double>(i); });
		STATIC_REQUIRE((std::is_same<decltype(o7r), candybox::optional<double>>::value));
		REQUIRE(o7r.value() == 42);

		// test each overload in turn
		candybox::optional<int> o8 = 42;
		auto o8r = o8.and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(*o8r == 42);

		candybox::optional<int> o9 = 42;
		auto o9r = std::move(o9).and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(*o9r == 42);

		const candybox::optional<int> o10 = 42;
		auto o10r = o10.and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(*o10r == 42);

		const candybox::optional<int> o11 = 42;
		auto o11r = std::move(o11).and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(*o11r == 42);

		candybox::optional<int> o16 = candybox::nullopt;
		auto o16r = o16.and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(!o16r);

		candybox::optional<int> o17 = candybox::nullopt;
		auto o17r = std::move(o17).and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(!o17r);

		const candybox::optional<int> o18 = candybox::nullopt;
		auto o18r = o18.and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(!o18r);

		const candybox::optional<int> o19 = candybox::nullopt;
		auto o19r = std::move(o19).and_then([](int) { return candybox::make_optional(42); });
		REQUIRE(!o19r);

		int i = 3;
		candybox::optional<int &> o20{i};
		std::move(o20).and_then([](int &r) { return candybox::optional<int &>{++r}; });
		REQUIRE(o20);
		REQUIRE(i == 4);
	}

	SECTION("constexpr and_then")
	{
#if !defined(_MSC_VER) && defined(TL_OPTIONAL_CXX14)

		constexpr candybox::optional<int> o10 = 42;
		constexpr auto o10r = o10.and_then(get_opt_int);
		REQUIRE(*o10r == 42);

		constexpr candybox::optional<int> o11 = 42;
		constexpr auto o11r = std::move(o11).and_then(get_opt_int);
		REQUIRE(*o11r == 42);

		constexpr candybox::optional<int> o18 = candybox::nullopt;
		constexpr auto o18r = o18.and_then(get_opt_int);
		REQUIRE(!o18r);

		constexpr candybox::optional<int> o19 = candybox::nullopt;
		constexpr auto o19r = std::move(o19).and_then(get_opt_int);
		REQUIRE(!o19r);
#endif
	}

	SECTION("or else")
	{
		candybox::optional<int> o1 = 42;
		REQUIRE(*(o1.or_else([] { return candybox::make_optional(13); })) == 42);

		candybox::optional<int> o2;
		REQUIRE(*(o2.or_else([] { return candybox::make_optional(13); })) == 13);
	}

	SECTION("disjunction")
	{
		candybox::optional<int> o1 = 42;
		candybox::optional<int> o2 = 12;
		candybox::optional<int> o3;

		REQUIRE(*o1.disjunction(o2) == 42);
		REQUIRE(*o1.disjunction(o3) == 42);
		REQUIRE(*o2.disjunction(o1) == 12);
		REQUIRE(*o2.disjunction(o3) == 12);
		REQUIRE(*o3.disjunction(o1) == 42);
		REQUIRE(*o3.disjunction(o2) == 12);
	}

	SECTION("conjunction")
	{
		candybox::optional<int> o1 = 42;
		REQUIRE(*o1.conjunction(42.0) == 42.0);
		REQUIRE(*o1.conjunction(std::string{"hello"}) == std::string{"hello"});

		candybox::optional<int> o2;
		REQUIRE(!o2.conjunction(42.0));
		REQUIRE(!o2.conjunction(std::string{"hello"}));
	}

	SECTION("map_or")
	{
		candybox::optional<int> o1 = 21;
		REQUIRE((o1.map_or([](int x) { return x * 2; }, 13)) == 42);

		candybox::optional<int> o2;
		REQUIRE((o2.map_or([](int x) { return x * 2; }, 13)) == 13);
	}

	SECTION("map_or_else")
	{
		candybox::optional<int> o1 = 21;
		REQUIRE((o1.map_or_else([](int x) { return x * 2; }, [] { return 13; })) == 42);

		candybox::optional<int> o2;
		REQUIRE((o2.map_or_else([](int x) { return x * 2; }, [] { return 13; })) == 13);
	}

	SECTION("take")
	{
		candybox::optional<int> o1 = 42;
		REQUIRE(*o1.take() == 42);
		REQUIRE(!o1);

		candybox::optional<int> o2;
		REQUIRE(!o2.take());
		REQUIRE(!o2);
	}

	struct foo
	{
		void non_const() { }
	};

#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&                              \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
	SECTION("Issue #1")
	{
		candybox::optional<foo> f = foo{};
		auto l = [](auto &&x) { x.non_const(); };
		f.map(l);
	}
#endif

	struct overloaded
	{
		candybox::optional<int> operator()(foo &) { return 0; }
		candybox::optional<std::string> operator()(const foo &) { return ""; }
	};

	SECTION("Issue #2")
	{
		candybox::optional<foo> f = foo{};
		auto x = f.and_then(overloaded{});
	}
};
