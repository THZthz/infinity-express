#pragma once
#include "utils/Optional.hpp"

void
test_optional_assignment_ref()
{
	tl::optional<int> o1 = 42;
	tl::optional<int> o2 = 12;
	tl::optional<int> o3;

	o1 = o1;
	assert(*o1 == 42);

	o1 = o2;
	assert(*o1 == 12);

	o1 = o3;
	assert(!o1);

	o1 = 42;
	assert(*o1 == 42);

	o1 = tl::nullopt;
	assert(!o1);

	o1 = std::move(o2);
	assert(*o1 == 12);

	tl::optional<short> o4 = 42;

	o1 = o4;
	assert(*o1 == 42);

	o1 = std::move(o4);
	assert(*o1 == 42);
}

void
test_optional_assignment_value()
{
	auto i = 42;
	auto j = 12;

	tl::optional<int &> o1 = i;
	tl::optional<int &> o2 = j;
	tl::optional<int &> o3;

	o1 = o1;
	assert(*o1 == 42);
	assert(&*o1 == &i);

	o1 = o2;
	assert(*o1 == 12);

	o1 = o3;
	assert(!o1);

	auto k = 42;
	o1 = k;
	assert(*o1 == 42);
	assert(*o1 == i);
	assert(*o1 == k);
	assert(&*o1 != &i);
	assert(&*o1 == &k);

	k = 12;
	assert(*o1 == 12);

	o1 = tl::nullopt;
	assert(!o1);

	o1 = std::move(o2);
	assert(*o1 == 12);
}


// Old versions of GCC don't have the correct trait names. Could fix them up if needs be.
#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 && !defined(__clang__))
// nothing for now
void
test_optional_bases_triviality()
{
}
void
test_optional_bases_deletion()
{
}
#else
void
test_optional_bases_triviality()
{
	assert(std::is_trivially_copy_constructible<tl::optional<int>>::value);
	assert(std::is_trivially_copy_assignable<tl::optional<int>>::value);
	assert(std::is_trivially_move_constructible<tl::optional<int>>::value);
	assert(std::is_trivially_move_assignable<tl::optional<int>>::value);
	assert(std::is_trivially_destructible<tl::optional<int>>::value);

	{
		struct T
		{
			T(const T &) = default;
			T(T &&) = default;
			T &operator=(const T &) = default;
			T &operator=(T &&) = default;
			~T() = default;
		};
		assert(std::is_trivially_copy_constructible<tl::optional<T>>::value);
		assert(std::is_trivially_copy_assignable<tl::optional<T>>::value);
		assert(std::is_trivially_move_constructible<tl::optional<T>>::value);
		assert(std::is_trivially_move_assignable<tl::optional<T>>::value);
		assert(std::is_trivially_destructible<tl::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) { }
			T(T &&){};
			T &operator=(const T &) { return *this; }
			T &operator=(T &&) { return *this; };
			~T() { }
		};
		assert(!std::is_trivially_copy_constructible<tl::optional<T>>::value);
		assert(!std::is_trivially_copy_assignable<tl::optional<T>>::value);
		assert(!std::is_trivially_move_constructible<tl::optional<T>>::value);
		assert(!std::is_trivially_move_assignable<tl::optional<T>>::value);
		assert(!std::is_trivially_destructible<tl::optional<T>>::value);
	}
}

void
test_optional_bases_deletion()
{
	assert(std::is_copy_constructible<tl::optional<int>>::value);
	assert(std::is_copy_assignable<tl::optional<int>>::value);
	assert(std::is_move_constructible<tl::optional<int>>::value);
	assert(std::is_move_assignable<tl::optional<int>>::value);
	assert(std::is_destructible<tl::optional<int>>::value);

	{
		struct T
		{
			T(const T &) = default;
			T(T &&) = default;
			T &operator=(const T &) = default;
			T &operator=(T &&) = default;
			~T() = default;
		};
		assert(std::is_copy_constructible<tl::optional<T>>::value);
		assert(std::is_copy_assignable<tl::optional<T>>::value);
		assert(std::is_move_constructible<tl::optional<T>>::value);
		assert(std::is_move_assignable<tl::optional<T>>::value);
		assert(std::is_destructible<tl::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) = delete;
			T(T &&) = delete;
			T &operator=(const T &) = delete;
			T &operator=(T &&) = delete;
		};
		assert(!std::is_copy_constructible<tl::optional<T>>::value);
		assert(!std::is_copy_assignable<tl::optional<T>>::value);
		assert(!std::is_move_constructible<tl::optional<T>>::value);
		assert(!std::is_move_assignable<tl::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) = delete;
			T(T &&) = default;
			T &operator=(const T &) = delete;
			T &operator=(T &&) = default;
		};
		assert(!std::is_copy_constructible<tl::optional<T>>::value);
		assert(!std::is_copy_assignable<tl::optional<T>>::value);
		assert(std::is_move_constructible<tl::optional<T>>::value);
		assert(std::is_move_assignable<tl::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) = default;
			T(T &&) = delete;
			T &operator=(const T &) = default;
			T &operator=(T &&) = delete;
		};
		assert(std::is_copy_constructible<tl::optional<T>>::value);
		assert(std::is_copy_assignable<tl::optional<T>>::value);
	}
}
#endif

void
test_optional_constexpr()
{
#if !defined(TL_OPTIONAL_MSVC2015) && defined(TL_OPTIONAL_CXX14)
	// empty construct
	constexpr tl::optional<int> o2{};
	constexpr tl::optional<int> o3 = {};
	constexpr tl::optional<int> o4 = tl::nullopt;
	constexpr tl::optional<int> o5 = {tl::nullopt};
	constexpr tl::optional<int> o6(tl::nullopt);

	static_assert(!o2, "");
	static_assert(!o3, "");
	static_assert(!o4, "");
	static_assert(!o5, "");
	static_assert(!o6, "");

	// value construct
	constexpr tl::optional<int> o1 = 42;
	constexpr tl::optional<int> o2{42};
	constexpr tl::optional<int> o3(42);
	constexpr tl::optional<int> o4 = {42};
	constexpr int i = 42;
	constexpr tl::optional<int> o5 = std::move(i);
	constexpr tl::optional<int> o6{std::move(i)};
	constexpr tl::optional<int> o7(std::move(i));
	constexpr tl::optional<int> o8 = {std::move(i)};

	static_assert(*o1 == 42, "");
	static_assert(*o2 == 42, "");
	static_assert(*o3 == 42, "");
	static_assert(*o4 == 42, "");
	static_assert(*o5 == 42, "");
	static_assert(*o6 == 42, "");
	static_assert(*o7 == 42, "");
	static_assert(*o8 == 42, "");
#endif
}

void
test_optional_constructors()
{
	struct foo
	{
		foo() = default;
		foo(foo &) = delete;
		foo(foo &&){};
	};

	tl::optional<int> o1;
	assert(!o1);

	tl::optional<int> o2 = tl::nullopt;
	assert(!o2);

	tl::optional<int> o3 = 42;
	assert(*o3 == 42);

	tl::optional<int> o4 = o3;
	assert(*o4 == 42);

	tl::optional<int> o5 = o1;
	assert(!o5);

	tl::optional<int> o6 = std::move(o3);
	assert(*o6 == 42);

	tl::optional<short> o7 = 42;
	assert(*o7 == 42);

	tl::optional<int> o8 = o7;
	assert(*o8 == 42);

	tl::optional<int> o9 = std::move(o7);
	assert(*o9 == 42);

	{
		tl::optional<int &> o;
		assert(!o);

		tl::optional<int &> oo = o;
		assert(!oo);
	}

	{
		auto i = 42;
		tl::optional<int &> o = i;
		assert(o);
		assert(*o == 42);

		tl::optional<int &> oo = o;
		assert(oo);
		assert(*oo == 42);
	}

	std::vector<foo> v;
	v.emplace_back();
	tl::optional<std::vector<foo>> ov = std::move(v);
	assert(ov->size() == 1);
}

void
test_optional_emplace()
{
	tl::optional<std::pair<std::pair<int, int>, std::pair<double, double>>> i;
	i.emplace(std::piecewise_construct, std::make_tuple(0, 2), std::make_tuple(3, 4));
	assert(i->first.first == 0);
	assert(i->first.second == 2);
	assert(i->second.first == 3);
	assert(i->second.second == 4);

	struct A
	{
		A() { throw std::exception(); }
	};

	// Emplace with exception thrown
	tl::optional<A> a;
	try
	{
		a.emplace();
	}
	catch (const std::exception &)
	{
		assert(1);
		return;
	}
	assert(0); // should not be here
}

constexpr int
get_int(int)
{
	return 42;
}
TL_OPTIONAL_11_CONSTEXPR tl::optional<int>
get_opt_int(int)
{
	return 42;
}

// What is Clang Format up to?!
void
test_optional_monadic()
{
	/*SECTION("map") */ {
		// lhs is empty
		tl::optional<int> o1;
		auto o1r = o1.map([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o1r), tl::optional<int>>::value), "");
		assert(!o1r);

		// lhs has value
		tl::optional<int> o2 = 40;
		auto o2r = o2.map([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o2r), tl::optional<int>>::value), "");
		assert(o2r.value() == 42);

		struct rval_call_map
		{
			double operator()(int) && { return 42.0; };
		};

		// ensure that function object is forwarded
		tl::optional<int> o3 = 42;
		auto o3r = o3.map(rval_call_map{});
		static_assert((std::is_same<decltype(o3r), tl::optional<double>>::value), "");
		assert(o3r.value() == 42);

		// ensure that lhs is forwarded
		tl::optional<int> o4 = 40;
		auto o4r = std::move(o4).map([](int &&i) { return i + 2; });
		static_assert((std::is_same<decltype(o4r), tl::optional<int>>::value), "");
		assert(o4r.value() == 42);

		// ensure that lhs is const-propagated
		const tl::optional<int> o5 = 40;
		auto o5r = o5.map([](const int &i) { return i + 2; });
		static_assert((std::is_same<decltype(o5r), tl::optional<int>>::value), "");
		assert(o5r.value() == 42);

		// test void return
		tl::optional<int> o7 = 40;
		auto f7 = [](const int &) { return; };
		auto o7r = o7.map(f7);
		static_assert((std::is_same<decltype(o7r), tl::optional<tl::monostate>>::value), "");
		assert(o7r.has_value());

		// test each overload in turn
		tl::optional<int> o8 = 42;
		auto o8r = o8.map([](int) { return 42; });
		assert(*o8r == 42);

		tl::optional<int> o9 = 42;
		auto o9r = o9.map([](int) { return; });
		assert(o9r);

		tl::optional<int> o12 = 42;
		auto o12r = std::move(o12).map([](int) { return 42; });
		assert(*o12r == 42);

		tl::optional<int> o13 = 42;
		auto o13r = std::move(o13).map([](int) { return; });
		assert(o13r);

		const tl::optional<int> o16 = 42;
		auto o16r = o16.map([](int) { return 42; });
		assert(*o16r == 42);

		const tl::optional<int> o17 = 42;
		auto o17r = o17.map([](int) { return; });
		assert(o17r);

		const tl::optional<int> o20 = 42;
		auto o20r = std::move(o20).map([](int) { return 42; });
		assert(*o20r == 42);

		const tl::optional<int> o21 = 42;
		auto o21r = std::move(o21).map([](int) { return; });
		assert(o21r);

		tl::optional<int> o24 = tl::nullopt;
		auto o24r = o24.map([](int) { return 42; });
		assert(!o24r);

		tl::optional<int> o25 = tl::nullopt;
		auto o25r = o25.map([](int) { return; });
		assert(!o25r);

		tl::optional<int> o28 = tl::nullopt;
		auto o28r = std::move(o28).map([](int) { return 42; });
		assert(!o28r);

		tl::optional<int> o29 = tl::nullopt;
		auto o29r = std::move(o29).map([](int) { return; });
		assert(!o29r);

		const tl::optional<int> o32 = tl::nullopt;
		auto o32r = o32.map([](int) { return 42; });
		assert(!o32r);

		const tl::optional<int> o33 = tl::nullopt;
		auto o33r = o33.map([](int) { return; });
		assert(!o33r);

		const tl::optional<int> o36 = tl::nullopt;
		auto o36r = std::move(o36).map([](int) { return 42; });
		assert(!o36r);

		const tl::optional<int> o37 = tl::nullopt;
		auto o37r = std::move(o37).map([](int) { return; });
		assert(!o37r);

		// callable which returns a reference
		tl::optional<int> o38 = 42;
		auto o38r = o38.map([](int &i) -> const int & { return i; });
		assert(o38r);
		assert(*o38r == 42);

		int i = 42;
		tl::optional<int &> o39 = i;
		o39.map([](int &x) { x = 12; });
		assert(i == 12);
	}

	/*SECTION("map constexpr") */ {
#if !defined(_MSC_VER) && defined(TL_OPTIONAL_CXX14)
		// test each overload in turn
		constexpr tl::optional<int> o16 = 42;
		constexpr auto o16r = o16.map(get_int);
		static_assert(*o16r == 42, "");

		constexpr tl::optional<int> o20 = 42;
		constexpr auto o20r = std::move(o20).map(get_int);
		static_assert(*o20r == 42, "");

		constexpr tl::optional<int> o32 = tl::nullopt;
		constexpr auto o32r = o32.map(get_int);
		static_assert(!o32r, "");
		constexpr tl::optional<int> o36 = tl::nullopt;
		constexpr auto o36r = std::move(o36).map(get_int);
		static_assert(!o36r, "");
#endif
	}

	/*SECTION("transform") */ { // lhs is empty
		tl::optional<int> o1;
		auto o1r = o1.transform([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o1r), tl::optional<int>>::value), "");
		assert(!o1r);

		// lhs has value
		tl::optional<int> o2 = 40;
		auto o2r = o2.transform([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o2r), tl::optional<int>>::value), "");
		assert(o2r.value() == 42);

		struct rval_call_transform
		{
			double operator()(int) && { return 42.0; };
		};

		// ensure that function object is forwarded
		tl::optional<int> o3 = 42;
		auto o3r = o3.transform(rval_call_transform{});
		static_assert((std::is_same<decltype(o3r), tl::optional<double>>::value), "");
		assert(o3r.value() == 42);

		// ensure that lhs is forwarded
		tl::optional<int> o4 = 40;
		auto o4r = std::move(o4).transform([](int &&i) { return i + 2; });
		static_assert((std::is_same<decltype(o4r), tl::optional<int>>::value), "");
		assert(o4r.value() == 42);

		// ensure that lhs is const-propagated
		const tl::optional<int> o5 = 40;
		auto o5r = o5.transform([](const int &i) { return i + 2; });
		static_assert((std::is_same<decltype(o5r), tl::optional<int>>::value), "");
		assert(o5r.value() == 42);

		// test void return
		tl::optional<int> o7 = 40;
		auto f7 = [](const int &) { return; };
		auto o7r = o7.transform(f7);
		static_assert((std::is_same<decltype(o7r), tl::optional<tl::monostate>>::value), "");
		assert(o7r.has_value());

		// test each overload in turn
		tl::optional<int> o8 = 42;
		auto o8r = o8.transform([](int) { return 42; });
		assert(*o8r == 42);

		tl::optional<int> o9 = 42;
		auto o9r = o9.transform([](int) { return; });
		assert(o9r);

		tl::optional<int> o12 = 42;
		auto o12r = std::move(o12).transform([](int) { return 42; });
		assert(*o12r == 42);

		tl::optional<int> o13 = 42;
		auto o13r = std::move(o13).transform([](int) { return; });
		assert(o13r);

		const tl::optional<int> o16 = 42;
		auto o16r = o16.transform([](int) { return 42; });
		assert(*o16r == 42);

		const tl::optional<int> o17 = 42;
		auto o17r = o17.transform([](int) { return; });
		assert(o17r);

		const tl::optional<int> o20 = 42;
		auto o20r = std::move(o20).transform([](int) { return 42; });
		assert(*o20r == 42);

		const tl::optional<int> o21 = 42;
		auto o21r = std::move(o21).transform([](int) { return; });
		assert(o21r);

		tl::optional<int> o24 = tl::nullopt;
		auto o24r = o24.transform([](int) { return 42; });
		assert(!o24r);

		tl::optional<int> o25 = tl::nullopt;
		auto o25r = o25.transform([](int) { return; });
		assert(!o25r);

		tl::optional<int> o28 = tl::nullopt;
		auto o28r = std::move(o28).transform([](int) { return 42; });
		assert(!o28r);

		tl::optional<int> o29 = tl::nullopt;
		auto o29r = std::move(o29).transform([](int) { return; });
		assert(!o29r);

		const tl::optional<int> o32 = tl::nullopt;
		auto o32r = o32.transform([](int) { return 42; });
		assert(!o32r);

		const tl::optional<int> o33 = tl::nullopt;
		auto o33r = o33.transform([](int) { return; });
		assert(!o33r);

		const tl::optional<int> o36 = tl::nullopt;
		auto o36r = std::move(o36).transform([](int) { return 42; });
		assert(!o36r);

		const tl::optional<int> o37 = tl::nullopt;
		auto o37r = std::move(o37).transform([](int) { return; });
		assert(!o37r);

		// callable which returns a reference
		tl::optional<int> o38 = 42;
		auto o38r = o38.transform([](int &i) -> const int & { return i; });
		assert(o38r);
		assert(*o38r == 42);

		int i = 42;
		tl::optional<int &> o39 = i;
		o39.transform([](int &x) { x = 12; });
		assert(i == 12);
	}

	/*SECTION("transform constexpr") */ {
#if !defined(_MSC_VER) && defined(TL_OPTIONAL_CXX14)
		// test each overload in turn
		constexpr tl::optional<int> o16 = 42;
		constexpr auto o16r = o16.transform(get_int);
		static_assert(*o16r == 42, "");

		constexpr tl::optional<int> o20 = 42;
		constexpr auto o20r = std::move(o20).transform(get_int);
		static_assert(*o20r == 42, "");

		constexpr tl::optional<int> o32 = tl::nullopt;
		constexpr auto o32r = o32.transform(get_int);
		static_assert(!o32r, "");
		constexpr tl::optional<int> o36 = tl::nullopt;
		constexpr auto o36r = std::move(o36).transform(get_int);
		static_assert(!o36r, "");
#endif
	}

	/*SECTION("and_then") */ {
		// lhs is empty
		tl::optional<int> o1;
		auto o1r = o1.and_then([](int) { return tl::optional<float>{42}; });
		static_assert((std::is_same<decltype(o1r), tl::optional<float>>::value), "");
		assert(!o1r);

		// lhs has value
		tl::optional<int> o2 = 12;
		auto o2r = o2.and_then([](int) { return tl::optional<float>{42}; });
		static_assert((std::is_same<decltype(o2r), tl::optional<float>>::value), "");
		assert(o2r.value() == 42.f);

		// lhs is empty, rhs returns empty
		tl::optional<int> o3;
		auto o3r = o3.and_then([](int) { return tl::optional<float>{}; });
		static_assert((std::is_same<decltype(o3r), tl::optional<float>>::value), "");
		;
		assert(!o3r);

		// rhs returns empty
		tl::optional<int> o4 = 12;
		auto o4r = o4.and_then([](int) { return tl::optional<float>{}; });
		static_assert((std::is_same<decltype(o4r), tl::optional<float>>::value), "");
		assert(!o4r);

		struct rval_call_and_then
		{
			tl::optional<double> operator()(int) && { return tl::optional<double>(42.0); };
		};

		// ensure that function object is forwarded
		tl::optional<int> o5 = 42;
		auto o5r = o5.and_then(rval_call_and_then{});
		static_assert((std::is_same<decltype(o5r), tl::optional<double>>::value), "");
		assert(o5r.value() == 42);

		// ensure that lhs is forwarded
		tl::optional<int> o6 = 42;
		auto o6r = std::move(o6).and_then([](int &&i) { return tl::optional<double>(i); });
		static_assert((std::is_same<decltype(o6r), tl::optional<double>>::value), "");
		assert(o6r.value() == 42);

		// ensure that function object is const-propagated
		const tl::optional<int> o7 = 42;
		auto o7r = o7.and_then([](const int &i) { return tl::optional<double>(i); });
		static_assert((std::is_same<decltype(o7r), tl::optional<double>>::value), "");
		assert(o7r.value() == 42);

		// test each overload in turn
		tl::optional<int> o8 = 42;
		auto o8r = o8.and_then([](int) { return tl::make_optional(42); });
		assert(*o8r == 42);

		tl::optional<int> o9 = 42;
		auto o9r = std::move(o9).and_then([](int) { return tl::make_optional(42); });
		assert(*o9r == 42);

		const tl::optional<int> o10 = 42;
		auto o10r = o10.and_then([](int) { return tl::make_optional(42); });
		assert(*o10r == 42);

		const tl::optional<int> o11 = 42;
		auto o11r = std::move(o11).and_then([](int) { return tl::make_optional(42); });
		assert(*o11r == 42);

		tl::optional<int> o16 = tl::nullopt;
		auto o16r = o16.and_then([](int) { return tl::make_optional(42); });
		assert(!o16r);

		tl::optional<int> o17 = tl::nullopt;
		auto o17r = std::move(o17).and_then([](int) { return tl::make_optional(42); });
		assert(!o17r);

		const tl::optional<int> o18 = tl::nullopt;
		auto o18r = o18.and_then([](int) { return tl::make_optional(42); });
		assert(!o18r);

		const tl::optional<int> o19 = tl::nullopt;
		auto o19r = std::move(o19).and_then([](int) { return tl::make_optional(42); });
		assert(!o19r);

		int i = 3;
		tl::optional<int &> o20{i};
		std::move(o20).and_then([](int &r) { return tl::optional<int &>{++r}; });
		assert(o20);
		assert(i == 4);
	}

	/*SECTION("constexpr and_then") */ {
#if !defined(_MSC_VER) && defined(TL_OPTIONAL_CXX14)

		constexpr tl::optional<int> o10 = 42;
		constexpr auto o10r = o10.and_then(get_opt_int);
		assert(*o10r == 42);

		constexpr tl::optional<int> o11 = 42;
		constexpr auto o11r = std::move(o11).and_then(get_opt_int);
		assert(*o11r == 42);

		constexpr tl::optional<int> o18 = tl::nullopt;
		constexpr auto o18r = o18.and_then(get_opt_int);
		assert(!o18r);

		constexpr tl::optional<int> o19 = tl::nullopt;
		constexpr auto o19r = std::move(o19).and_then(get_opt_int);
		assert(!o19r);
#endif
	}

	/*SECTION("or else") */ {
		tl::optional<int> o1 = 42;
		assert(*(o1.or_else([] { return tl::make_optional(13); })) == 42);

		tl::optional<int> o2;
		assert(*(o2.or_else([] { return tl::make_optional(13); })) == 13);
	}

	/*SECTION("disjunction") */ {
		tl::optional<int> o1 = 42;
		tl::optional<int> o2 = 12;
		tl::optional<int> o3;

		assert(*o1.disjunction(o2) == 42);
		assert(*o1.disjunction(o3) == 42);
		assert(*o2.disjunction(o1) == 12);
		assert(*o2.disjunction(o3) == 12);
		assert(*o3.disjunction(o1) == 42);
		assert(*o3.disjunction(o2) == 12);
	}

	/*SECTION("conjunction") */ {
		tl::optional<int> o1 = 42;
		assert(*o1.conjunction(42.0) == 42.0);
		assert(*o1.conjunction(std::string{"hello"}) == std::string{"hello"});

		tl::optional<int> o2;
		assert(!o2.conjunction(42.0));
		assert(!o2.conjunction(std::string{"hello"}));
	}

	/*SECTION("map_or") */ {
		tl::optional<int> o1 = 21;
		assert((o1.map_or([](int x) { return x * 2; }, 13)) == 42);

		tl::optional<int> o2;
		assert((o2.map_or([](int x) { return x * 2; }, 13)) == 13);
	}

	/*SECTION("map_or_else") */ {
		tl::optional<int> o1 = 21;
		assert((o1.map_or_else([](int x) { return x * 2; }, [] { return 13; })) == 42);

		tl::optional<int> o2;
		assert((o2.map_or_else([](int x) { return x * 2; }, [] { return 13; })) == 13);
	}

	/*SECTION("take") */ {
		tl::optional<int> o1 = 42;
		assert(*o1.take() == 42);
		assert(!o1);

		tl::optional<int> o2;
		assert(!o2.take());
		assert(!o2);
	}

	struct foo
	{
		void non_const() { }
	};

#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&                              \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
	/*SECTION("Issue #1") */ {
		tl::optional<foo> f = foo{};
		auto l = [](auto &&x) { x.non_const(); };
		f.map(l);
	}
#endif

	struct overloaded
	{
		tl::optional<int> operator()(foo &) { return 0; }
		tl::optional<std::string> operator()(const foo &) { return ""; }
	};

	/*SECTION("Issue #2") */ {
		tl::optional<foo> f = foo{};
		auto x = f.and_then(overloaded{});
	}
}

struct takes_init_and_variadic
{
	std::vector<int> v;
	std::tuple<int, int> t;
	template <class... Args>
	takes_init_and_variadic(std::initializer_list<int> l, Args &&...args)
	    : v(l), t(std::forward<Args>(args)...)
	{
	}
};

void
test_optional_in_place()
{
	tl::optional<int> o1{tl::in_place};
	tl::optional<int> o2(tl::in_place);
	assert(o1);
	assert(o1 == 0);
	assert(o2);
	assert(o2 == 0);

	tl::optional<int> o3(tl::in_place, 42);
	assert(o3 == 42);

	tl::optional<std::tuple<int, int>> o4(tl::in_place, 0, 1);
	assert(o4);
	assert(std::get<0>(*o4) == 0);
	assert(std::get<1>(*o4) == 1);

	tl::optional<std::vector<int>> o5(tl::in_place, {0, 1});
	assert(o5);
	assert((*o5)[0] == 0);
	assert((*o5)[1] == 1);

	tl::optional<takes_init_and_variadic> o6(tl::in_place, {0, 1}, 2, 3);
	assert(o6->v[0] == 0);
	assert(o6->v[1] == 1);
	assert(std::get<0>(o6->t) == 2);
	assert(std::get<1>(o6->t) == 3);
}

int &
test_optional_x(int &i)
{
	i = 42;
	return i;
}

void
test_optional_issue_14()
{
	struct foo
	{
		int &v() { return i; }
		int i = 0;
	};

	tl::optional<foo> f = foo{};
	auto v = f.map(&foo::v).map(test_optional_x);
	static_assert(
	    std::is_same<decltype(v), tl::optional<int &>>::value,
	    "Must return a "
	    "reference");
	assert(f->i == 42);
	assert(*v == 42);
	assert((&f->i) == (&*v));
}

struct fail_on_copy_self
{
	int value;
	fail_on_copy_self(int v) : value(v) { }
	fail_on_copy_self(const fail_on_copy_self &other) : value(other.value)
	{
		assert(&other != this);
	}
};

void
test_optional_issue_15()
{
	tl::optional<fail_on_copy_self> o = fail_on_copy_self(42);

	o = o;
	assert(o->value == 42);
}

void
test_optional_issue_33()
{
	int i = 0;
	int j = 0;
	tl::optional<int &> a = i;
	a.emplace(j);
	*a = 42;
	assert(j == 42);
	assert(*a == 42);
	assert(a.has_value());
}

void
test_optional_make_optional()
{
	auto o1 = tl::make_optional(42);
	auto o2 = tl::optional<int>(42);

	constexpr bool is_same = std::is_same<decltype(o1), tl::optional<int>>::value;
	assert(is_same);
	assert(o1 == o2);

	auto o3 = tl::make_optional<std::tuple<int, int, int, int>>(0, 1, 2, 3);
	assert(std::get<0>(*o3) == 0);
	assert(std::get<1>(*o3) == 1);
	assert(std::get<2>(*o3) == 2);
	assert(std::get<3>(*o3) == 3);

	auto o4 = tl::make_optional<std::vector<int>>({0, 1, 2, 3});
	assert(o4.value()[0] == 0);
	assert(o4.value()[1] == 1);
	assert(o4.value()[2] == 2);
	assert(o4.value()[3] == 3);

	auto o5 = tl::make_optional<takes_init_and_variadic>({0, 1}, 2, 3);
	assert(o5->v[0] == 0);
	assert(o5->v[1] == 1);
	assert(std::get<0>(o5->t) == 2);
	assert(std::get<1>(o5->t) == 3);

	auto i = 42;
	auto o6 = tl::make_optional<int &>(i);
	assert((std::is_same<decltype(o6), tl::optional<int &>>::value));
	assert(o6);
	assert(*o6 == 42);
}

void
test_optional_noexcept()
{
	tl::optional<int> o1{4};
	tl::optional<int> o2{42};

	/*SECTION("comparison with nullopt") */ {
		assert(noexcept(o1 == tl::nullopt));
		assert(noexcept(tl::nullopt == o1));
		assert(noexcept(o1 != tl::nullopt));
		assert(noexcept(tl::nullopt != o1));
		assert(noexcept(o1 < tl::nullopt));
		assert(noexcept(tl::nullopt < o1));
		assert(noexcept(o1 <= tl::nullopt));
		assert(noexcept(tl::nullopt <= o1));
		assert(noexcept(o1 > tl::nullopt));
		assert(noexcept(tl::nullopt > o1));
		assert(noexcept(o1 >= tl::nullopt));
		assert(noexcept(tl::nullopt >= o1));
	}

	/*SECTION("swap") */ {
		//TODO see why this fails
#if !defined(_MSC_VER) || _MSC_VER > 1900
		assert(noexcept(swap(o1, o2)) == noexcept(o1.swap(o2)));

		struct nothrow_swappable
		{
			nothrow_swappable &swap(const nothrow_swappable &) noexcept { return *this; }
		};

		struct throw_swappable
		{
			throw_swappable() = default;
			throw_swappable(const throw_swappable &) { }
			throw_swappable(throw_swappable &&) { }
			throw_swappable &swap(const throw_swappable &) { return *this; }
		};

		tl::optional<nothrow_swappable> ont;
		tl::optional<throw_swappable> ot;

		assert(noexcept(ont.swap(ont)));
		assert(!noexcept(ot.swap(ot)));
#endif
	}

	/*SECTION("constructors") */ {
		//TODO see why this fails
#if !defined(_MSC_VER) || _MSC_VER > 1900
		assert(noexcept(tl::optional<int>{}));
		assert(noexcept(tl::optional<int>{tl::nullopt}));

		struct nothrow_move
		{
			nothrow_move(nothrow_move &&) noexcept = default;
		};

		struct throw_move
		{
			throw_move(throw_move &&){};
		};

		using nothrow_opt = tl::optional<nothrow_move>;
		using throw_opt = tl::optional<throw_move>;

		assert(std::is_nothrow_move_constructible<nothrow_opt>::value);
		assert(!std::is_nothrow_move_constructible<throw_opt>::value);
#endif
	}

	/*SECTION("assignment") */ {
		assert(noexcept(o1 = tl::nullopt));

		struct nothrow_move_assign
		{
			nothrow_move_assign() = default;
			nothrow_move_assign(nothrow_move_assign &&) noexcept = default;
			nothrow_move_assign &operator=(const nothrow_move_assign &) = default;
		};

		struct throw_move_assign
		{
			throw_move_assign() = default;
			throw_move_assign(throw_move_assign &&){};
			throw_move_assign &operator=(const throw_move_assign &) { return *this; }
		};

		using nothrow_opt = tl::optional<nothrow_move_assign>;
		using throw_opt = tl::optional<throw_move_assign>;

		assert(noexcept(std::declval<nothrow_opt>() = std::declval<nothrow_opt>()));
		assert(!noexcept(std::declval<throw_opt>() = std::declval<throw_opt>()));
	}

	/*SECTION("observers") */ {
		assert(noexcept(static_cast<bool>(o1)));
		assert(noexcept(o1.has_value()));
	}

	/*SECTION("modifiers") */ {
		assert(noexcept(o1.reset()));
	}
}


void
test_optional_nullopt()
{
	tl::optional<int> o1 = tl::nullopt;
	tl::optional<int> o2{tl::nullopt};
	tl::optional<int> o3(tl::nullopt);
	tl::optional<int> o4 = {tl::nullopt};

	assert(!o1);
	assert(!o2);
	assert(!o3);
	assert(!o4);

	assert(!std::is_default_constructible<tl::nullopt_t>::value);
}


struct move_detector
{
	move_detector() = default;
	move_detector(move_detector &&rhs) { rhs.been_moved = true; }
	bool been_moved = false;
};

void
test_optional_observers()
{
	tl::optional<int> o1 = 42;
	tl::optional<int> o2;
	const tl::optional<int> o3 = 42;

	assert(*o1 == 42);
	assert(*o1 == o1.value());
	assert(o2.value_or(42) == 42);
	assert(o3.value() == 42);
	auto success = std::is_same<decltype(o1.value()), int &>::value;
	assert(success);
	success = std::is_same<decltype(o3.value()), const int &>::value;
	assert(success);
	success = std::is_same<decltype(std::move(o1).value()), int &&>::value;
	assert(success);

#ifndef TL_OPTIONAL_NO_CONSTRR
	success = std::is_same<decltype(std::move(o3).value()), const int &&>::value;
	assert(success);
#endif

	tl::optional<move_detector> o4{tl::in_place};
	move_detector o5 = std::move(o4).value();
	assert(o4->been_moved);
	assert(!o5.been_moved);
}

// Relational ops
void
test_optional_relops()
{
	tl::optional<int> o1{4};
	tl::optional<int> o2{42};
	tl::optional<int> o3{};

	/*SECTION("self simple") */ {
		assert(!(o1 == o2));
		assert(o1 == o1);
		assert(o1 != o2);
		assert(!(o1 != o1));
		assert(o1 < o2);
		assert(!(o1 < o1));
		assert(!(o1 > o2));
		assert(!(o1 > o1));
		assert(o1 <= o2);
		assert(o1 <= o1);
		assert(!(o1 >= o2));
		assert(o1 >= o1);
	}

	/*SECTION("nullopt simple") */ {
		assert(!(o1 == tl::nullopt));
		assert(!(tl::nullopt == o1));
		assert(o1 != tl::nullopt);
		assert(tl::nullopt != o1);
		assert(!(o1 < tl::nullopt));
		assert(tl::nullopt < o1);
		assert(o1 > tl::nullopt);
		assert(!(tl::nullopt > o1));
		assert(!(o1 <= tl::nullopt));
		assert(tl::nullopt <= o1);
		assert(o1 >= tl::nullopt);
		assert(!(tl::nullopt >= o1));

		assert(o3 == tl::nullopt);
		assert(tl::nullopt == o3);
		assert(!(o3 != tl::nullopt));
		assert(!(tl::nullopt != o3));
		assert(!(o3 < tl::nullopt));
		assert(!(tl::nullopt < o3));
		assert(!(o3 > tl::nullopt));
		assert(!(tl::nullopt > o3));
		assert(o3 <= tl::nullopt);
		assert(tl::nullopt <= o3);
		assert(o3 >= tl::nullopt);
		assert(tl::nullopt >= o3);
	}

	/*SECTION("with T simple") */ {
		assert(!(o1 == 1));
		assert(!(1 == o1));
		assert(o1 != 1);
		assert(1 != o1);
		assert(!(o1 < 1));
		assert(1 < o1);
		assert(o1 > 1);
		assert(!(1 > o1));
		assert(!(o1 <= 1));
		assert(1 <= o1);
		assert(o1 >= 1);
		assert(!(1 >= o1));

		assert(o1 == 4);
		assert(4 == o1);
		assert(!(o1 != 4));
		assert(!(4 != o1));
		assert(!(o1 < 4));
		assert(!(4 < o1));
		assert(!(o1 > 4));
		assert(!(4 > o1));
		assert(o1 <= 4);
		assert(4 <= o1);
		assert(o1 >= 4);
		assert(4 >= o1);
	}

	tl::optional<std::string> o4{"hello"};
	tl::optional<std::string> o5{"xyz"};

	/*SECTION("self complex") */ {
		assert(!(o4 == o5));
		assert(o4 == o4);
		assert(o4 != o5);
		assert(!(o4 != o4));
		assert(o4 < o5);
		assert(!(o4 < o4));
		assert(!(o4 > o5));
		assert(!(o4 > o4));
		assert(o4 <= o5);
		assert(o4 <= o4);
		assert(!(o4 >= o5));
		assert(o4 >= o4);
	}

	/*SECTION("nullopt complex") */ {
		assert(!(o4 == tl::nullopt));
		assert(!(tl::nullopt == o4));
		assert(o4 != tl::nullopt);
		assert(tl::nullopt != o4);
		assert(!(o4 < tl::nullopt));
		assert(tl::nullopt < o4);
		assert(o4 > tl::nullopt);
		assert(!(tl::nullopt > o4));
		assert(!(o4 <= tl::nullopt));
		assert(tl::nullopt <= o4);
		assert(o4 >= tl::nullopt);
		assert(!(tl::nullopt >= o4));

		assert(o3 == tl::nullopt);
		assert(tl::nullopt == o3);
		assert(!(o3 != tl::nullopt));
		assert(!(tl::nullopt != o3));
		assert(!(o3 < tl::nullopt));
		assert(!(tl::nullopt < o3));
		assert(!(o3 > tl::nullopt));
		assert(!(tl::nullopt > o3));
		assert(o3 <= tl::nullopt);
		assert(tl::nullopt <= o3);
		assert(o3 >= tl::nullopt);
		assert(tl::nullopt >= o3);
	}

	/*SECTION("with T complex") */ {
		assert(!(o4 == "a"));
		assert(!("a" == o4));
		assert(o4 != "a");
		assert("a" != o4);
		assert(!(o4 < "a"));
		assert("a" < o4);
		assert(o4 > "a");
		assert(!("a" > o4));
		assert(!(o4 <= "a"));
		assert("a" <= o4);
		assert(o4 >= "a");
		assert(!("a" >= o4));

		assert(o4 == "hello");
		assert("hello" == o4);
		assert(!(o4 != "hello"));
		assert(!("hello" != o4));
		assert(!(o4 < "hello"));
		assert(!("hello" < o4));
		assert(!(o4 > "hello"));
		assert(!("hello" > o4));
		assert(o4 <= "hello");
		assert("hello" <= o4);
		assert(o4 >= "hello");
		assert("hello" >= o4);
	}
}

void
test_optional_swap_value()
{
	tl::optional<int> o1 = 42;
	tl::optional<int> o2 = 12;
	o1.swap(o2);
	assert(o1.value() == 12);
	assert(o2.value() == 42);
}

void
test_optional_swap_value_nullopt()
{
	tl::optional<int> o1 = 42;
	tl::optional<int> o2 = tl::nullopt;
	o1.swap(o2);
	assert(!o1.has_value());
	assert(o2.value() == 42);
}

void
test_optional_swap_nullopt_value()
{
	tl::optional<int> o1 = tl::nullopt;
	tl::optional<int> o2 = 42;
	o1.swap(o2);
	assert(o1.value() == 42);
	assert(!o2.has_value());
}

void
test_optional()
{
	test_optional_assignment_ref();
	test_optional_assignment_value();

	test_optional_bases_triviality();
	test_optional_bases_deletion();

	test_optional_constexpr();

	test_optional_constructors();

	test_optional_emplace();

	test_optional_monadic();

	test_optional_in_place();

	test_optional_issue_14();
	test_optional_issue_15();
	test_optional_issue_33();

	test_optional_make_optional();

	test_optional_noexcept();

	test_optional_nullopt();

	test_optional_observers();

	test_optional_relops();

	test_optional_swap_value();
	test_optional_swap_value_nullopt();
	test_optional_swap_nullopt_value();
}
