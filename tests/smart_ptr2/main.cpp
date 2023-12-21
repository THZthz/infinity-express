#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "utils/smart_ptr.hpp"

struct allow
{
};

template <class T = void> struct creator
{
	typedef T value_type;

	template <class U> struct rebind
	{
		typedef creator<U> other;
	};

	creator() { }

	template <class U> creator(const creator<U> &) { }

	T *allocate(std::size_t size)
	{
		return static_cast<T *>(::operator new(sizeof(T) * size));
	}

	void deallocate(T *ptr, std::size_t) { ::operator delete(ptr); }

	template <class U> void construct(U *ptr) { ::new (static_cast<void *>(ptr)) U(allow()); }

	template <class U> void destroy(U *ptr) { ptr->~U(); }
};

template <class T, class U>
inline bool
operator==(const creator<T> &, const creator<U> &)
{
	return true;
}

template <class T, class U>
inline bool
operator!=(const creator<T> &, const creator<U> &)
{
	return false;
}

class type
{
public:
	static unsigned instances;

	explicit type(allow) { ++instances; }

	~type() { --instances; }

private:
	type(const type &);

	type &operator=(const type &);
};

unsigned type::instances = 0;

TEST_CASE("construct local shared array", "[local_shared_array.construct]")
{
	//
	{
		boost::local_shared_ptr<type[]> result =
		    boost::allocate_local_shared<type[]>(creator<type>(), 3);
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 3);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<type[3]> result =
		    boost::allocate_local_shared<type[3]>(creator<type>());
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 3);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<type[][2]> result =
		    boost::allocate_local_shared<type[][2]>(creator<>(), 2);
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 4);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<type[2][2]> result =
		    boost::allocate_local_shared<type[2][2]>(creator<>());
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 4);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<const type[]> result =
		    boost::allocate_local_shared<const type[]>(creator<>(), 3);
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 3);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<const type[3]> result =
		    boost::allocate_local_shared<const type[3]>(creator<>());
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 3);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<const type[][2]> result =
		    boost::allocate_local_shared<const type[][2]>(creator<>(), 2);
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 4);
		result.reset();
		REQUIRE(type::instances == 0);
	}
	{
		boost::local_shared_ptr<const type[2][2]> result =
		    boost::allocate_local_shared<const type[2][2]>(creator<>());
		REQUIRE(result.get() != 0);
		REQUIRE(result.local_use_count() == 1);
		REQUIRE(type::instances == 4);
		result.reset();
		REQUIRE(type::instances == 0);
	}
}

int
main(int argc, char *argv[])
{
	int result = Catch::Session().run(argc, argv);

	return result;
}
