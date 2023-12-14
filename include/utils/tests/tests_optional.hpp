#pragma once

#include <vector>
#include <tuple>
#include <string>
#include <cassert>
#include "utils/optional.hpp"

void
test_optional_assignment_ref()
{
	ie::optional<int> o1 = 42;
	ie::optional<int> o2 = 12;
	ie::optional<int> o3;

	o1 = o1;
	assert(*o1 == 42);

	o1 = o2;
	assert(*o1 == 12);

	o1 = o3;
	assert(!o1);

	o1 = 42;
	assert(*o1 == 42);

	o1 = ie::nullopt;
	assert(!o1);

	o1 = std::move(o2);
	assert(*o1 == 12);

	ie::optional<short> o4 = 42;

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

	ie::optional<int &> o1 = i;
	ie::optional<int &> o2 = j;
	ie::optional<int &> o3;

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

	o1 = ie::nullopt;
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
	assert(std::is_trivially_copy_constructible<ie::optional<int>>::value);
	assert(std::is_trivially_copy_assignable<ie::optional<int>>::value);
	assert(std::is_trivially_move_constructible<ie::optional<int>>::value);
	assert(std::is_trivially_move_assignable<ie::optional<int>>::value);
	assert(std::is_trivially_destructible<ie::optional<int>>::value);

	{
		struct T
		{
			T(const T &) = default;
			T(T &&) = default;
			T &operator=(const T &) = default;
			T &operator=(T &&) = default;
			~T() = default;
		};
		assert(std::is_trivially_copy_constructible<ie::optional<T>>::value);
		assert(std::is_trivially_copy_assignable<ie::optional<T>>::value);
		assert(std::is_trivially_move_constructible<ie::optional<T>>::value);
		assert(std::is_trivially_move_assignable<ie::optional<T>>::value);
		assert(std::is_trivially_destructible<ie::optional<T>>::value);
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
		assert(!std::is_trivially_copy_constructible<ie::optional<T>>::value);
		assert(!std::is_trivially_copy_assignable<ie::optional<T>>::value);
		assert(!std::is_trivially_move_constructible<ie::optional<T>>::value);
		assert(!std::is_trivially_move_assignable<ie::optional<T>>::value);
		assert(!std::is_trivially_destructible<ie::optional<T>>::value);
	}
}

void
test_optional_bases_deletion()
{
	assert(std::is_copy_constructible<ie::optional<int>>::value);
	assert(std::is_copy_assignable<ie::optional<int>>::value);
	assert(std::is_move_constructible<ie::optional<int>>::value);
	assert(std::is_move_assignable<ie::optional<int>>::value);
	assert(std::is_destructible<ie::optional<int>>::value);

	{
		struct T
		{
			T(const T &) = default;
			T(T &&) = default;
			T &operator=(const T &) = default;
			T &operator=(T &&) = default;
			~T() = default;
		};
		assert(std::is_copy_constructible<ie::optional<T>>::value);
		assert(std::is_copy_assignable<ie::optional<T>>::value);
		assert(std::is_move_constructible<ie::optional<T>>::value);
		assert(std::is_move_assignable<ie::optional<T>>::value);
		assert(std::is_destructible<ie::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) = delete;
			T(T &&) = delete;
			T &operator=(const T &) = delete;
			T &operator=(T &&) = delete;
		};
		assert(!std::is_copy_constructible<ie::optional<T>>::value);
		assert(!std::is_copy_assignable<ie::optional<T>>::value);
		assert(!std::is_move_constructible<ie::optional<T>>::value);
		assert(!std::is_move_assignable<ie::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) = delete;
			T(T &&) = default;
			T &operator=(const T &) = delete;
			T &operator=(T &&) = default;
		};
		assert(!std::is_copy_constructible<ie::optional<T>>::value);
		assert(!std::is_copy_assignable<ie::optional<T>>::value);
		assert(std::is_move_constructible<ie::optional<T>>::value);
		assert(std::is_move_assignable<ie::optional<T>>::value);
	}

	{
		struct T
		{
			T(const T &) = default;
			T(T &&) = delete;
			T &operator=(const T &) = default;
			T &operator=(T &&) = delete;
		};
		assert(std::is_copy_constructible<ie::optional<T>>::value);
		assert(std::is_copy_assignable<ie::optional<T>>::value);
	}
}
#endif

void
test_optional_constexpr()
{
#if !defined(IE_OPTIONAL_MSVC2015) && defined(IE_OPTIONAL_CXX14)
	// empty construct
	constexpr ie::optional<int> o2{};
	constexpr ie::optional<int> o3 = {};
	constexpr ie::optional<int> o4 = ie::nullopt;
	constexpr ie::optional<int> o5 = {ie::nullopt};
	constexpr ie::optional<int> o6(ie::nullopt);

	static_assert(!o2, "");
	static_assert(!o3, "");
	static_assert(!o4, "");
	static_assert(!o5, "");
	static_assert(!o6, "");

	// value construct
	constexpr ie::optional<int> o1 = 42;
	constexpr ie::optional<int> o2{42};
	constexpr ie::optional<int> o3(42);
	constexpr ie::optional<int> o4 = {42};
	constexpr int i = 42;
	constexpr ie::optional<int> o5 = std::move(i);
	constexpr ie::optional<int> o6{std::move(i)};
	constexpr ie::optional<int> o7(std::move(i));
	constexpr ie::optional<int> o8 = {std::move(i)};

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

	ie::optional<int> o1;
	assert(!o1);

	ie::optional<int> o2 = ie::nullopt;
	assert(!o2);

	ie::optional<int> o3 = 42;
	assert(*o3 == 42);

	ie::optional<int> o4 = o3;
	assert(*o4 == 42);

	ie::optional<int> o5 = o1;
	assert(!o5);

	ie::optional<int> o6 = std::move(o3);
	assert(*o6 == 42);

	ie::optional<short> o7 = 42;
	assert(*o7 == 42);

	ie::optional<int> o8 = o7;
	assert(*o8 == 42);

	ie::optional<int> o9 = std::move(o7);
	assert(*o9 == 42);

	{
		ie::optional<int &> o;
		assert(!o);

		ie::optional<int &> oo = o;
		assert(!oo);
	}

	{
		auto i = 42;
		ie::optional<int &> o = i;
		assert(o);
		assert(*o == 42);

		ie::optional<int &> oo = o;
		assert(oo);
		assert(*oo == 42);
	}

	std::vector<foo> v;
	v.emplace_back();
	ie::optional<std::vector<foo>> ov = std::move(v);
	assert(ov->size() == 1);
}

void
test_optional_emplace()
{
	ie::optional<std::pair<std::pair<int, int>, std::pair<double, double>>> i;
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
	ie::optional<A> a;
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
IE_OPTIONAL_11_CONSTEXPR ie::optional<int>
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
		ie::optional<int> o1;
		auto o1r = o1.map([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o1r), ie::optional<int>>::value), "");
		assert(!o1r);

		// lhs has value
		ie::optional<int> o2 = 40;
		auto o2r = o2.map([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o2r), ie::optional<int>>::value), "");
		assert(o2r.value() == 42);

		struct rval_call_map
		{
			double operator()(int) && { return 42.0; };
		};

		// ensure that function object is forwarded
		ie::optional<int> o3 = 42;
		auto o3r = o3.map(rval_call_map{});
		static_assert((std::is_same<decltype(o3r), ie::optional<double>>::value), "");
		assert(o3r.value() == 42);

		// ensure that lhs is forwarded
		ie::optional<int> o4 = 40;
		auto o4r = std::move(o4).map([](int &&i) { return i + 2; });
		static_assert((std::is_same<decltype(o4r), ie::optional<int>>::value), "");
		assert(o4r.value() == 42);

		// ensure that lhs is const-propagated
		const ie::optional<int> o5 = 40;
		auto o5r = o5.map([](const int &i) { return i + 2; });
		static_assert((std::is_same<decltype(o5r), ie::optional<int>>::value), "");
		assert(o5r.value() == 42);

		// test void return
		ie::optional<int> o7 = 40;
		auto f7 = [](const int &) { return; };
		auto o7r = o7.map(f7);
		static_assert((std::is_same<decltype(o7r), ie::optional<ie::monostate>>::value), "");
		assert(o7r.has_value());

		// test each overload in turn
		ie::optional<int> o8 = 42;
		auto o8r = o8.map([](int) { return 42; });
		assert(*o8r == 42);

		ie::optional<int> o9 = 42;
		auto o9r = o9.map([](int) { return; });
		assert(o9r);

		ie::optional<int> o12 = 42;
		auto o12r = std::move(o12).map([](int) { return 42; });
		assert(*o12r == 42);

		ie::optional<int> o13 = 42;
		auto o13r = std::move(o13).map([](int) { return; });
		assert(o13r);

		const ie::optional<int> o16 = 42;
		auto o16r = o16.map([](int) { return 42; });
		assert(*o16r == 42);

		const ie::optional<int> o17 = 42;
		auto o17r = o17.map([](int) { return; });
		assert(o17r);

		const ie::optional<int> o20 = 42;
		auto o20r = std::move(o20).map([](int) { return 42; });
		assert(*o20r == 42);

		const ie::optional<int> o21 = 42;
		auto o21r = std::move(o21).map([](int) { return; });
		assert(o21r);

		ie::optional<int> o24 = ie::nullopt;
		auto o24r = o24.map([](int) { return 42; });
		assert(!o24r);

		ie::optional<int> o25 = ie::nullopt;
		auto o25r = o25.map([](int) { return; });
		assert(!o25r);

		ie::optional<int> o28 = ie::nullopt;
		auto o28r = std::move(o28).map([](int) { return 42; });
		assert(!o28r);

		ie::optional<int> o29 = ie::nullopt;
		auto o29r = std::move(o29).map([](int) { return; });
		assert(!o29r);

		const ie::optional<int> o32 = ie::nullopt;
		auto o32r = o32.map([](int) { return 42; });
		assert(!o32r);

		const ie::optional<int> o33 = ie::nullopt;
		auto o33r = o33.map([](int) { return; });
		assert(!o33r);

		const ie::optional<int> o36 = ie::nullopt;
		auto o36r = std::move(o36).map([](int) { return 42; });
		assert(!o36r);

		const ie::optional<int> o37 = ie::nullopt;
		auto o37r = std::move(o37).map([](int) { return; });
		assert(!o37r);

		// callable which returns a reference
		ie::optional<int> o38 = 42;
		auto o38r = o38.map([](int &i) -> const int & { return i; });
		assert(o38r);
		assert(*o38r == 42);

		int i = 42;
		ie::optional<int &> o39 = i;
		o39.map([](int &x) { x = 12; });
		assert(i == 12);
	}

	/*SECTION("map constexpr") */ {
#if !defined(_MSC_VER) && defined(IE_OPTIONAL_CXX14)
		// test each overload in turn
		constexpr ie::optional<int> o16 = 42;
		constexpr auto o16r = o16.map(get_int);
		static_assert(*o16r == 42, "");

		constexpr ie::optional<int> o20 = 42;
		constexpr auto o20r = std::move(o20).map(get_int);
		static_assert(*o20r == 42, "");

		constexpr ie::optional<int> o32 = ie::nullopt;
		constexpr auto o32r = o32.map(get_int);
		static_assert(!o32r, "");
		constexpr ie::optional<int> o36 = ie::nullopt;
		constexpr auto o36r = std::move(o36).map(get_int);
		static_assert(!o36r, "");
#endif
	}

	/*SECTION("transform") */ { // lhs is empty
		ie::optional<int> o1;
		auto o1r = o1.transform([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o1r), ie::optional<int>>::value), "");
		assert(!o1r);

		// lhs has value
		ie::optional<int> o2 = 40;
		auto o2r = o2.transform([](int i) { return i + 2; });
		static_assert((std::is_same<decltype(o2r), ie::optional<int>>::value), "");
		assert(o2r.value() == 42);

		struct rval_call_transform
		{
			double operator()(int) && { return 42.0; };
		};

		// ensure that function object is forwarded
		ie::optional<int> o3 = 42;
		auto o3r = o3.transform(rval_call_transform{});
		static_assert((std::is_same<decltype(o3r), ie::optional<double>>::value), "");
		assert(o3r.value() == 42);

		// ensure that lhs is forwarded
		ie::optional<int> o4 = 40;
		auto o4r = std::move(o4).transform([](int &&i) { return i + 2; });
		static_assert((std::is_same<decltype(o4r), ie::optional<int>>::value), "");
		assert(o4r.value() == 42);

		// ensure that lhs is const-propagated
		const ie::optional<int> o5 = 40;
		auto o5r = o5.transform([](const int &i) { return i + 2; });
		static_assert((std::is_same<decltype(o5r), ie::optional<int>>::value), "");
		assert(o5r.value() == 42);

		// test void return
		ie::optional<int> o7 = 40;
		auto f7 = [](const int &) { return; };
		auto o7r = o7.transform(f7);
		static_assert((std::is_same<decltype(o7r), ie::optional<ie::monostate>>::value), "");
		assert(o7r.has_value());

		// test each overload in turn
		ie::optional<int> o8 = 42;
		auto o8r = o8.transform([](int) { return 42; });
		assert(*o8r == 42);

		ie::optional<int> o9 = 42;
		auto o9r = o9.transform([](int) { return; });
		assert(o9r);

		ie::optional<int> o12 = 42;
		auto o12r = std::move(o12).transform([](int) { return 42; });
		assert(*o12r == 42);

		ie::optional<int> o13 = 42;
		auto o13r = std::move(o13).transform([](int) { return; });
		assert(o13r);

		const ie::optional<int> o16 = 42;
		auto o16r = o16.transform([](int) { return 42; });
		assert(*o16r == 42);

		const ie::optional<int> o17 = 42;
		auto o17r = o17.transform([](int) { return; });
		assert(o17r);

		const ie::optional<int> o20 = 42;
		auto o20r = std::move(o20).transform([](int) { return 42; });
		assert(*o20r == 42);

		const ie::optional<int> o21 = 42;
		auto o21r = std::move(o21).transform([](int) { return; });
		assert(o21r);

		ie::optional<int> o24 = ie::nullopt;
		auto o24r = o24.transform([](int) { return 42; });
		assert(!o24r);

		ie::optional<int> o25 = ie::nullopt;
		auto o25r = o25.transform([](int) { return; });
		assert(!o25r);

		ie::optional<int> o28 = ie::nullopt;
		auto o28r = std::move(o28).transform([](int) { return 42; });
		assert(!o28r);

		ie::optional<int> o29 = ie::nullopt;
		auto o29r = std::move(o29).transform([](int) { return; });
		assert(!o29r);

		const ie::optional<int> o32 = ie::nullopt;
		auto o32r = o32.transform([](int) { return 42; });
		assert(!o32r);

		const ie::optional<int> o33 = ie::nullopt;
		auto o33r = o33.transform([](int) { return; });
		assert(!o33r);

		const ie::optional<int> o36 = ie::nullopt;
		auto o36r = std::move(o36).transform([](int) { return 42; });
		assert(!o36r);

		const ie::optional<int> o37 = ie::nullopt;
		auto o37r = std::move(o37).transform([](int) { return; });
		assert(!o37r);

		// callable which returns a reference
		ie::optional<int> o38 = 42;
		auto o38r = o38.transform([](int &i) -> const int & { return i; });
		assert(o38r);
		assert(*o38r == 42);

		int i = 42;
		ie::optional<int &> o39 = i;
		o39.transform([](int &x) { x = 12; });
		assert(i == 12);
	}

	/*SECTION("transform constexpr") */ {
#if !defined(_MSC_VER) && defined(IE_OPTIONAL_CXX14)
		// test each overload in turn
		constexpr ie::optional<int> o16 = 42;
		constexpr auto o16r = o16.transform(get_int);
		static_assert(*o16r == 42, "");

		constexpr ie::optional<int> o20 = 42;
		constexpr auto o20r = std::move(o20).transform(get_int);
		static_assert(*o20r == 42, "");

		constexpr ie::optional<int> o32 = ie::nullopt;
		constexpr auto o32r = o32.transform(get_int);
		static_assert(!o32r, "");
		constexpr ie::optional<int> o36 = ie::nullopt;
		constexpr auto o36r = std::move(o36).transform(get_int);
		static_assert(!o36r, "");
#endif
	}

	/*SECTION("and_then") */ {
		// lhs is empty
		ie::optional<int> o1;
		auto o1r = o1.and_then([](int) { return ie::optional<float>{42}; });
		static_assert((std::is_same<decltype(o1r), ie::optional<float>>::value), "");
		assert(!o1r);

		// lhs has value
		ie::optional<int> o2 = 12;
		auto o2r = o2.and_then([](int) { return ie::optional<float>{42}; });
		static_assert((std::is_same<decltype(o2r), ie::optional<float>>::value), "");
		assert(o2r.value() == 42.f);

		// lhs is empty, rhs returns empty
		ie::optional<int> o3;
		auto o3r = o3.and_then([](int) { return ie::optional<float>{}; });
		static_assert((std::is_same<decltype(o3r), ie::optional<float>>::value), "");
		;
		assert(!o3r);

		// rhs returns empty
		ie::optional<int> o4 = 12;
		auto o4r = o4.and_then([](int) { return ie::optional<float>{}; });
		static_assert((std::is_same<decltype(o4r), ie::optional<float>>::value), "");
		assert(!o4r);

		struct rval_call_and_then
		{
			ie::optional<double> operator()(int) && { return ie::optional<double>(42.0); };
		};

		// ensure that function object is forwarded
		ie::optional<int> o5 = 42;
		auto o5r = o5.and_then(rval_call_and_then{});
		static_assert((std::is_same<decltype(o5r), ie::optional<double>>::value), "");
		assert(o5r.value() == 42);

		// ensure that lhs is forwarded
		ie::optional<int> o6 = 42;
		auto o6r = std::move(o6).and_then([](int &&i) { return ie::optional<double>(i); });
		static_assert((std::is_same<decltype(o6r), ie::optional<double>>::value), "");
		assert(o6r.value() == 42);

		// ensure that function object is const-propagated
		const ie::optional<int> o7 = 42;
		auto o7r = o7.and_then([](const int &i) { return ie::optional<double>(i); });
		static_assert((std::is_same<decltype(o7r), ie::optional<double>>::value), "");
		assert(o7r.value() == 42);

		// test each overload in turn
		ie::optional<int> o8 = 42;
		auto o8r = o8.and_then([](int) { return ie::make_optional(42); });
		assert(*o8r == 42);

		ie::optional<int> o9 = 42;
		auto o9r = std::move(o9).and_then([](int) { return ie::make_optional(42); });
		assert(*o9r == 42);

		const ie::optional<int> o10 = 42;
		auto o10r = o10.and_then([](int) { return ie::make_optional(42); });
		assert(*o10r == 42);

		const ie::optional<int> o11 = 42;
		auto o11r = std::move(o11).and_then([](int) { return ie::make_optional(42); });
		assert(*o11r == 42);

		ie::optional<int> o16 = ie::nullopt;
		auto o16r = o16.and_then([](int) { return ie::make_optional(42); });
		assert(!o16r);

		ie::optional<int> o17 = ie::nullopt;
		auto o17r = std::move(o17).and_then([](int) { return ie::make_optional(42); });
		assert(!o17r);

		const ie::optional<int> o18 = ie::nullopt;
		auto o18r = o18.and_then([](int) { return ie::make_optional(42); });
		assert(!o18r);

		const ie::optional<int> o19 = ie::nullopt;
		auto o19r = std::move(o19).and_then([](int) { return ie::make_optional(42); });
		assert(!o19r);

		int i = 3;
		ie::optional<int &> o20{i};
		std::move(o20).and_then([](int &r) { return ie::optional<int &>{++r}; });
		assert(o20);
		assert(i == 4);
	}

	/*SECTION("constexpr and_then") */ {
#if !defined(_MSC_VER) && defined(IE_OPTIONAL_CXX14)

		constexpr ie::optional<int> o10 = 42;
		constexpr auto o10r = o10.and_then(get_opt_int);
		assert(*o10r == 42);

		constexpr ie::optional<int> o11 = 42;
		constexpr auto o11r = std::move(o11).and_then(get_opt_int);
		assert(*o11r == 42);

		constexpr ie::optional<int> o18 = ie::nullopt;
		constexpr auto o18r = o18.and_then(get_opt_int);
		assert(!o18r);

		constexpr ie::optional<int> o19 = ie::nullopt;
		constexpr auto o19r = std::move(o19).and_then(get_opt_int);
		assert(!o19r);
#endif
	}

	/*SECTION("or else") */ {
		ie::optional<int> o1 = 42;
		assert(*(o1.or_else([] { return ie::make_optional(13); })) == 42);

		ie::optional<int> o2;
		assert(*(o2.or_else([] { return ie::make_optional(13); })) == 13);
	}

	/*SECTION("disjunction") */ {
		ie::optional<int> o1 = 42;
		ie::optional<int> o2 = 12;
		ie::optional<int> o3;

		assert(*o1.disjunction(o2) == 42);
		assert(*o1.disjunction(o3) == 42);
		assert(*o2.disjunction(o1) == 12);
		assert(*o2.disjunction(o3) == 12);
		assert(*o3.disjunction(o1) == 42);
		assert(*o3.disjunction(o2) == 12);
	}

	/*SECTION("conjunction") */ {
		ie::optional<int> o1 = 42;
		assert(*o1.conjunction(42.0) == 42.0);
		assert(*o1.conjunction(std::string{"hello"}) == std::string{"hello"});

		ie::optional<int> o2;
		assert(!o2.conjunction(42.0));
		assert(!o2.conjunction(std::string{"hello"}));
	}

	/*SECTION("map_or") */ {
		ie::optional<int> o1 = 21;
		assert((o1.map_or([](int x) { return x * 2; }, 13)) == 42);

		ie::optional<int> o2;
		assert((o2.map_or([](int x) { return x * 2; }, 13)) == 13);
	}

	/*SECTION("map_or_else") */ {
		ie::optional<int> o1 = 21;
		assert((o1.map_or_else([](int x) { return x * 2; }, [] { return 13; })) == 42);

		ie::optional<int> o2;
		assert((o2.map_or_else([](int x) { return x * 2; }, [] { return 13; })) == 13);
	}

	/*SECTION("take") */ {
		ie::optional<int> o1 = 42;
		assert(*o1.take() == 42);
		assert(!o1);

		ie::optional<int> o2;
		assert(!o2.take());
		assert(!o2);
	}

	struct foo
	{
		void non_const() { }
	};

#if defined(IE_OPTIONAL_CXX14) && !defined(IE_OPTIONAL_GCC49) &&                              \
    !defined(IE_OPTIONAL_GCC54) && !defined(IE_OPTIONAL_GCC55)
	/*SECTION("Issue #1") */ {
		ie::optional<foo> f = foo{};
		auto l = [](auto &&x) { x.non_const(); };
		f.map(l);
	}
#endif

	struct overloaded
	{
		ie::optional<int> operator()(foo &) { return 0; }
		ie::optional<std::string> operator()(const foo &) { return ""; }
	};

	/*SECTION("Issue #2") */ {
		ie::optional<foo> f = foo{};
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
	ie::optional<int> o1{ie::in_place};
	ie::optional<int> o2(ie::in_place);
	assert(o1);
	assert(o1 == 0);
	assert(o2);
	assert(o2 == 0);

	ie::optional<int> o3(ie::in_place, 42);
	assert(o3 == 42);

	ie::optional<std::tuple<int, int>> o4(ie::in_place, 0, 1);
	assert(o4);
	assert(std::get<0>(*o4) == 0);
	assert(std::get<1>(*o4) == 1);

	ie::optional<std::vector<int>> o5(ie::in_place, {0, 1});
	assert(o5);
	assert((*o5)[0] == 0);
	assert((*o5)[1] == 1);

	ie::optional<takes_init_and_variadic> o6(ie::in_place, {0, 1}, 2, 3);
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

	ie::optional<foo> f = foo{};
	auto v = f.map(&foo::v).map(test_optional_x);
	static_assert(
	    std::is_same<decltype(v), ie::optional<int &>>::value,
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
	ie::optional<fail_on_copy_self> o = fail_on_copy_self(42);

	o = o;
	assert(o->value == 42);
}

void
test_optional_issue_33()
{
	int i = 0;
	int j = 0;
	ie::optional<int &> a = i;
	a.emplace(j);
	*a = 42;
	assert(j == 42);
	assert(*a == 42);
	assert(a.has_value());
}

void
test_optional_make_optional()
{
	auto o1 = ie::make_optional(42);
	auto o2 = ie::optional<int>(42);

	constexpr bool is_same = std::is_same<decltype(o1), ie::optional<int>>::value;
	assert(is_same);
	assert(o1 == o2);

	auto o3 = ie::make_optional<std::tuple<int, int, int, int>>(0, 1, 2, 3);
	assert(std::get<0>(*o3) == 0);
	assert(std::get<1>(*o3) == 1);
	assert(std::get<2>(*o3) == 2);
	assert(std::get<3>(*o3) == 3);

	auto o4 = ie::make_optional<std::vector<int>>({0, 1, 2, 3});
	assert(o4.value()[0] == 0);
	assert(o4.value()[1] == 1);
	assert(o4.value()[2] == 2);
	assert(o4.value()[3] == 3);

	auto o5 = ie::make_optional<takes_init_and_variadic>({0, 1}, 2, 3);
	assert(o5->v[0] == 0);
	assert(o5->v[1] == 1);
	assert(std::get<0>(o5->t) == 2);
	assert(std::get<1>(o5->t) == 3);

	auto i = 42;
	auto o6 = ie::make_optional<int &>(i);
	assert((std::is_same<decltype(o6), ie::optional<int &>>::value));
	assert(o6);
	assert(*o6 == 42);
}

void
test_optional_noexcept()
{
	ie::optional<int> o1{4};
	ie::optional<int> o2{42};

	/*SECTION("comparison with nullopt") */ {
		assert(noexcept(o1 == ie::nullopt));
		assert(noexcept(ie::nullopt == o1));
		assert(noexcept(o1 != ie::nullopt));
		assert(noexcept(ie::nullopt != o1));
		assert(noexcept(o1 < ie::nullopt));
		assert(noexcept(ie::nullopt < o1));
		assert(noexcept(o1 <= ie::nullopt));
		assert(noexcept(ie::nullopt <= o1));
		assert(noexcept(o1 > ie::nullopt));
		assert(noexcept(ie::nullopt > o1));
		assert(noexcept(o1 >= ie::nullopt));
		assert(noexcept(ie::nullopt >= o1));
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

		ie::optional<nothrow_swappable> ont;
		ie::optional<throw_swappable> ot;

		assert(noexcept(ont.swap(ont)));
		assert(!noexcept(ot.swap(ot)));
#endif
	}

	/*SECTION("constructors") */ {
		//TODO see why this fails
#if !defined(_MSC_VER) || _MSC_VER > 1900
		assert(noexcept(ie::optional<int>{}));
		assert(noexcept(ie::optional<int>{ie::nullopt}));

		struct nothrow_move
		{
			nothrow_move(nothrow_move &&) noexcept = default;
		};

		struct throw_move
		{
			throw_move(throw_move &&){};
		};

		using nothrow_opt = ie::optional<nothrow_move>;
		using throw_opt = ie::optional<throw_move>;

		assert(std::is_nothrow_move_constructible<nothrow_opt>::value);
		assert(!std::is_nothrow_move_constructible<throw_opt>::value);
#endif
	}

	/*SECTION("assignment") */ {
		assert(noexcept(o1 = ie::nullopt));

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

		using nothrow_opt = ie::optional<nothrow_move_assign>;
		using throw_opt = ie::optional<throw_move_assign>;

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
	ie::optional<int> o1 = ie::nullopt;
	ie::optional<int> o2{ie::nullopt};
	ie::optional<int> o3(ie::nullopt);
	ie::optional<int> o4 = {ie::nullopt};

	assert(!o1);
	assert(!o2);
	assert(!o3);
	assert(!o4);

	assert(!std::is_default_constructible<ie::nullopt_t>::value);
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
	ie::optional<int> o1 = 42;
	ie::optional<int> o2;
	const ie::optional<int> o3 = 42;

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

#ifndef IE_OPTIONAL_NO_CONSTRR
	success = std::is_same<decltype(std::move(o3).value()), const int &&>::value;
	assert(success);
#endif

	ie::optional<move_detector> o4{ie::in_place};
	move_detector o5 = std::move(o4).value();
	assert(o4->been_moved);
	assert(!o5.been_moved);
}

// Relational ops
void
test_optional_relops()
{
	ie::optional<int> o1{4};
	ie::optional<int> o2{42};
	ie::optional<int> o3{};

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
		assert(!(o1 == ie::nullopt));
		assert(!(ie::nullopt == o1));
		assert(o1 != ie::nullopt);
		assert(ie::nullopt != o1);
		assert(!(o1 < ie::nullopt));
		assert(ie::nullopt < o1);
		assert(o1 > ie::nullopt);
		assert(!(ie::nullopt > o1));
		assert(!(o1 <= ie::nullopt));
		assert(ie::nullopt <= o1);
		assert(o1 >= ie::nullopt);
		assert(!(ie::nullopt >= o1));

		assert(o3 == ie::nullopt);
		assert(ie::nullopt == o3);
		assert(!(o3 != ie::nullopt));
		assert(!(ie::nullopt != o3));
		assert(!(o3 < ie::nullopt));
		assert(!(ie::nullopt < o3));
		assert(!(o3 > ie::nullopt));
		assert(!(ie::nullopt > o3));
		assert(o3 <= ie::nullopt);
		assert(ie::nullopt <= o3);
		assert(o3 >= ie::nullopt);
		assert(ie::nullopt >= o3);
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

	ie::optional<std::string> o4{"hello"};
	ie::optional<std::string> o5{"xyz"};

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
		assert(!(o4 == ie::nullopt));
		assert(!(ie::nullopt == o4));
		assert(o4 != ie::nullopt);
		assert(ie::nullopt != o4);
		assert(!(o4 < ie::nullopt));
		assert(ie::nullopt < o4);
		assert(o4 > ie::nullopt);
		assert(!(ie::nullopt > o4));
		assert(!(o4 <= ie::nullopt));
		assert(ie::nullopt <= o4);
		assert(o4 >= ie::nullopt);
		assert(!(ie::nullopt >= o4));

		assert(o3 == ie::nullopt);
		assert(ie::nullopt == o3);
		assert(!(o3 != ie::nullopt));
		assert(!(ie::nullopt != o3));
		assert(!(o3 < ie::nullopt));
		assert(!(ie::nullopt < o3));
		assert(!(o3 > ie::nullopt));
		assert(!(ie::nullopt > o3));
		assert(o3 <= ie::nullopt);
		assert(ie::nullopt <= o3);
		assert(o3 >= ie::nullopt);
		assert(ie::nullopt >= o3);
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
	ie::optional<int> o1 = 42;
	ie::optional<int> o2 = 12;
	o1.swap(o2);
	assert(o1.value() == 12);
	assert(o2.value() == 42);
}

void
test_optional_swap_value_nullopt()
{
	ie::optional<int> o1 = 42;
	ie::optional<int> o2 = ie::nullopt;
	o1.swap(o2);
	assert(!o1.has_value());
	assert(o2.value() == 42);
}

void
test_optional_swap_nullopt_value()
{
	ie::optional<int> o1 = ie::nullopt;
	ie::optional<int> o2 = 42;
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
