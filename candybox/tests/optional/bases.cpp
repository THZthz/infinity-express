#include <catch2/catch_all.hpp>
#include "candybox/optional.hpp"

// Old versions of GCC don't have the correct trait names. Could fix them up if needs be.
#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 && !defined(__clang__))
// nothing for now
#else
TEST_CASE("Triviality", "[bases.triviality]")
{
	REQUIRE(std::is_trivially_copy_constructible<candybox::optional<int> >::value);
	REQUIRE(std::is_trivially_copy_assignable<candybox::optional<int> >::value);
	REQUIRE(std::is_trivially_move_constructible<candybox::optional<int> >::value);
	REQUIRE(std::is_trivially_move_assignable<candybox::optional<int> >::value);
	REQUIRE(std::is_trivially_destructible<candybox::optional<int> >::value);

	{
		struct T
		{
			T(const T&) = default;
			T(T&&) = default;
			T& operator=(const T&) = default;
			T& operator=(T&&) = default;
			~T() = default;
		};
		REQUIRE(std::is_trivially_copy_constructible<candybox::optional<T> >::value);
		REQUIRE(std::is_trivially_copy_assignable<candybox::optional<T> >::value);
		REQUIRE(std::is_trivially_move_constructible<candybox::optional<T> >::value);
		REQUIRE(std::is_trivially_move_assignable<candybox::optional<T> >::value);
		REQUIRE(std::is_trivially_destructible<candybox::optional<T> >::value);
	}

	{
		struct T
		{
			T(const T&) { }
			T(T&&){};
			T& operator=(const T&) { return *this; }
			T& operator=(T&&) { return *this; };
			~T() { }
		};
		REQUIRE(!std::is_trivially_copy_constructible<candybox::optional<T> >::value);
		REQUIRE(!std::is_trivially_copy_assignable<candybox::optional<T> >::value);
		REQUIRE(!std::is_trivially_move_constructible<candybox::optional<T> >::value);
		REQUIRE(!std::is_trivially_move_assignable<candybox::optional<T> >::value);
		REQUIRE(!std::is_trivially_destructible<candybox::optional<T> >::value);
	}
}

TEST_CASE("Deletion", "[bases.deletion]")
{
	REQUIRE(std::is_copy_constructible<candybox::optional<int> >::value);
	REQUIRE(std::is_copy_assignable<candybox::optional<int> >::value);
	REQUIRE(std::is_move_constructible<candybox::optional<int> >::value);
	REQUIRE(std::is_move_assignable<candybox::optional<int> >::value);
	REQUIRE(std::is_destructible<candybox::optional<int> >::value);

	{
		struct T
		{
			T(const T&) = default;
			T(T&&) = default;
			T& operator=(const T&) = default;
			T& operator=(T&&) = default;
			~T() = default;
		};
		REQUIRE(std::is_copy_constructible<candybox::optional<T> >::value);
		REQUIRE(std::is_copy_assignable<candybox::optional<T> >::value);
		REQUIRE(std::is_move_constructible<candybox::optional<T> >::value);
		REQUIRE(std::is_move_assignable<candybox::optional<T> >::value);
		REQUIRE(std::is_destructible<candybox::optional<T> >::value);
	}

	{
		struct T
		{
			T(const T&) = delete;
			T(T&&) = delete;
			T& operator=(const T&) = delete;
			T& operator=(T&&) = delete;
		};
		REQUIRE(!std::is_copy_constructible<candybox::optional<T> >::value);
		REQUIRE(!std::is_copy_assignable<candybox::optional<T> >::value);
		REQUIRE(!std::is_move_constructible<candybox::optional<T> >::value);
		REQUIRE(!std::is_move_assignable<candybox::optional<T> >::value);
	}

	{
		struct T
		{
			T(const T&) = delete;
			T(T&&) = default;
			T& operator=(const T&) = delete;
			T& operator=(T&&) = default;
		};
		REQUIRE(!std::is_copy_constructible<candybox::optional<T> >::value);
		REQUIRE(!std::is_copy_assignable<candybox::optional<T> >::value);
		REQUIRE(std::is_move_constructible<candybox::optional<T> >::value);
		REQUIRE(std::is_move_assignable<candybox::optional<T> >::value);
	}

	{
		struct T
		{
			T(const T&) = default;
			T(T&&) = delete;
			T& operator=(const T&) = default;
			T& operator=(T&&) = delete;
		};
		REQUIRE(std::is_copy_constructible<candybox::optional<T> >::value);
		REQUIRE(std::is_copy_assignable<candybox::optional<T> >::value);
	}
}
#endif
