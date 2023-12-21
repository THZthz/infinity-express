#include <catch2/catch_all.hpp>
#include "optional.hpp"
#include <type_traits>

struct foo
{
	int& v() { return i; }
	int i = 0;
};

int&
x(int& i)
{
	i = 42;
	return i;
}

TEST_CASE("issue 14")
{
	ie::optional<foo> f = foo{};
	auto v = f.map(&foo::v).map(x);
	static_assert(std::is_same<decltype(v), ie::optional<int&>>::value, "Must return a "
	                                                                    "reference");
	REQUIRE(f->i == 42);
	REQUIRE(*v == 42);
	REQUIRE((&f->i) == (&*v));
}

struct fail_on_copy_self
{
	int value;
	fail_on_copy_self(int v) : value(v) { }
	fail_on_copy_self(const fail_on_copy_self& other) : value(other.value)
	{
		REQUIRE(&other != this);
	}
};

TEST_CASE("issue 15")
{
	ie::optional<fail_on_copy_self> o = fail_on_copy_self(42);

	o = o;
	REQUIRE(o->value == 42);
}

TEST_CASE("issue 33")
{
	int i = 0;
	int j = 0;
	ie::optional<int&> a = i;
	a.emplace(j);
	*a = 42;
	REQUIRE(j == 42);
	REQUIRE(*a == 42);
	REQUIRE(a.has_value());
}
