#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>
#include "utils/vector.hpp"

using vector_int = ie::vector<int>;
using vector_int_ea = ie::vector<int, ie::malloc_allocator<int>>;

class TA_A
{
public:
	TA_A() = default;
	explicit TA_A(int i) { operator=(i); }

	bool equals(int i) const { return atoi(s.c_str()) == i; }

	bool operator==(int i) const { return equals(i); }
	bool operator==(TA_A const& o) const { return s == o.s; }

	TA_A& operator=(const int& rhs)
	{
		std::stringstream ss;
		ss << rhs;
		s = ss.str();
		return *this;
	}

private:
	std::string s;
};

using vector_a = ie::vector<TA_A>;
using vector_a_ea = ie::vector<TA_A, ie::malloc_allocator<TA_A>>;

template <class V>
static bool
cmp(V const& a, V const& b)
{
	return a == b;
}

/*-------------------------------------------------------------------------------------------*/

int
test_vector_append()
{
	int ret = 0;

	vector_int v;
	for (int i = 0; i < 20; i++) { v.push_back(i); }

	if (v.size() != 20)
	{
		std::cerr << "invalid size!" << std::endl;
		ret = 1;
	}

	size_t cur_capacity = v.capacity();

	v.reserve(10);
	if (v.capacity() != cur_capacity)
	{
		std::cerr << "resize 10 failure" << std::endl;
		ret = 1;
	}
	v.reserve(40);
	if (v.capacity() != 40)
	{
		std::cerr << "reserve 40 failure" << std::endl;
		ret = 1;
	}

	for (int i = 20; i < 40; i++) { v.emplace_back(i); }

	if (v.size() != 40)
	{
		std::cerr << "invalid size!" << std::endl;
		ret = 1;
	}

	for (int i = 0; i < (int)v.size(); i++)
	{
		if (v.at(i) != i)
		{
			std::cerr << "error at " << i << std::endl;
			ret = 1;
		}
	}

	vector_a va;
	for (int i = 0; i < 20; i++) { va.push_back(TA_A(i)); }

	if (va.size() != 20)
	{
		std::cerr << "invalid size!" << std::endl;
		ret = 1;
	}

	cur_capacity = va.capacity();

	va.reserve(10);
	if (va.capacity() != cur_capacity)
	{
		std::cerr << "resize 10 failure" << std::endl;
		ret = 1;
	}
	va.reserve(40);
	if (va.capacity() != 40)
	{
		std::cerr << "reserve 40 failure" << std::endl;
		ret = 1;
	}

	for (int i = 20; i < 40; i++) { va.emplace_back(i); }

	if (va.size() != 40)
	{
		std::cerr << "invalid size!" << std::endl;
		ret = 1;
	}

	for (int i = 0; i < (int)v.size(); i++)
	{
		if (!va.at(i).equals(i))
		{
			std::cerr << "error at " << i << std::endl;
			ret = 1;
		}
	}

	vector_a vas;
	vas.resize(4, TA_A(1));
	for (int i = 0; i < 4; i++)
	{
		if (!vas.at(i).equals(1))
		{
			std::cerr << "error on resize with an object at " << i << std::endl;
			ret = 1;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		vas.pop_back();
		if (vas.size() != (size_t)(3 - i))
		{
			std::cerr << "size error!" << std::endl;
			ret = 1;
		}
	}

	vas.reserve(40);
	vas.push_back(TA_A(0));
	vas.push_back(TA_A(1));
	vas.shrink_to_fit();
	if (vas.size() != 2 || vas.capacity() != 2)
	{
		std::cerr << "error with shrink_to_fit" << std::endl;
		ret = 1;
	}

	return ret;
}

/*-------------------------------------------------------------------------------------------*/

int
test_vector_assign()
{
	int ret = 0;

	vector_a va;
	for (int i = 0; i < 20; i++) { va.emplace_back(i); }

	va.assign(4, TA_A(5));
	if (va.size() != 4)
	{
		std::cerr << "wrong size!" << std::endl;
		ret = 1;
	}

	std::vector<TA_A> sva;
	for (int i = 0; i < 10; i++) { sva.emplace_back(i); }

	va.assign(std::begin(sva), std::end(sva));
	if (va.size() != 10)
	{
		std::cerr << "wrong size!" << std::endl;
		ret = 1;
	}
	for (int i = 0; i < (int)va.size(); i++)
	{
		if (!va.at(i).equals(i))
		{
			std::cerr << "wrong value at " << i << std::endl;
			ret = 1;
		}
	}

	vector_int v;
	for (int i = 0; i < 20; i++) { v.emplace_back(i); }

	v.assign((size_t)4, 5);
	if (v.size() != 4)
	{
		std::cerr << "wrong size!" << std::endl;
		ret = 1;
	}

	std::vector<int> sv;
	for (int i = 0; i < 10; i++) { sv.emplace_back(i); }

	v.assign(std::begin(sv), std::end(sv));
	if (v.size() != 10)
	{
		std::cerr << "wrong size!" << std::endl;
		ret = 1;
	}
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (v.at(i) != i)
		{
			std::cerr << "wrong value at " << i << std::endl;
			ret = 1;
		}
	}

	v.assign({0, 1, 2, 3});
	if (v.size() != 4)
	{
		std::cerr << "wrong size!" << std::endl;
		ret = 1;
	}
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (v.at(i) != i)
		{
			std::cerr << "wrong value at " << i << std::endl;
			ret = 1;
		}
	}

	vector_int vil({0, 1, 2, 3, 4, 5, 6});
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (vil.at(i) != i)
		{
			std::cerr << "wrong value at " << i << std::endl;
			ret = 1;
		}
	}

	return ret;
}

/*-------------------------------------------------------------------------------------------*/

int
check_v(vector_int const& v)
{
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (i != v.at(i)) { return 1; }
	}
	return 0;
}

int
check_va(vector_a const& v)
{
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (!v.at(i).equals(i)) { return 1; }
	}
	return 0;
}

int
test_vector_copy_move()
{
	int ret = 0;

	{
		vector_int v;
		v.resize(20);
		for (int i = 0; i < 20; i++) { v[i] = i; }

		v = v;
		ret |= check_v(v);

		vector_int v2;
		v2 = v;
		if (&v2.front() == &v.front())
		{
			std::cerr << "error copy!" << std::endl;
			ret = 1;
		}

		ret |= check_v(v2);

		int* buf = &v.front();
		vector_int vm(std::move(v));
		if ((&vm.front() != buf) || (v.size() != 0) || (v.capacity() != 0))
		{
			std::cerr << "error move constructor!" << std::endl;
			ret = 1;
		}
		ret |= check_v(vm);

		vector_int vm2;
		vm2.reserve(4);
		vm2 = std::move(vm);
		if ((&vm2.front() != buf) || (vm.size() != 0) || (vm.capacity() != 0))
		{
			std::cerr << "error move constructor!" << std::endl;
			ret = 1;
		}
		ret |= check_v(vm2);

		vector_a va;
		va.reserve(20);
		for (int i = 0; i < 20; i++) { va.emplace_back(i); }

		va = va;
		ret |= check_va(va);

		TA_A* bufa = &va.front();
		vector_a vma(std::move(va));
		if ((&vma.front() != bufa) || (va.size() != 0) || (va.capacity() != 0))
		{
			std::cerr << "error move constructor!" << std::endl;
			ret = 1;
		}
		ret |= check_va(vma);

		vector_a vma2;
		vma2.reserve(4);
		vma2 = std::move(vma);
		if ((&vma2.front() != bufa) || (vma.size() != 0) || (vma.capacity() != 0))
		{
			std::cerr << "error move constructor!" << std::endl;
			ret = 1;
		}
		ret |= check_va(vma2);

		vector_a vac;
		vac.resize(5);
		vac = vma2;
		ret |= check_va(vac);

		vector_a vac2(vac);
		if (vac2.size() != vac.size())
		{
			std::cerr << "Error on vector copy: wrong size!" << std::endl;
			ret = 1;
		}
		ret |= check_va(vac2);
	}

	{
		vector_a v1;
		vector_a v2;

		v1.resize(4);
		v2.resize(5);

		TA_A* buf_1 = &v1.front();
		TA_A* buf_2 = &v2.front();

		v1.swap(v2);
		if (buf_1 != &v2.front())
		{
			std::cerr << "buf_1 != v2" << std::endl;
			ret = 1;
		}

		if (buf_2 != &v1.front())
		{
			std::cerr << "buf_2 != v1" << std::endl;
			ret = 1;
		}

		if (v1.size() != 5 || v2.size() != 4)
		{
			std::cerr << "invalid sizes!" << std::endl;
			ret = 1;
		}
	}

	return ret;
}

/*-------------------------------------------------------------------------------------------*/

int
test_vector_enhanced_allocators()
{
	ie::vector<TA_A, ie::malloc_allocator<TA_A, true, true>> v({TA_A(1), TA_A(2)});
	//	std::cout << sizeof(v) << std::endl;

	v.emplace_back(0);
	v.emplace_back(1);
	v.emplace_back(2);
	v.emplace_back(3);
	v.emplace_back(4);
	//	std::cout << v.capacity() << std::endl;
	v.resize(14);
	//	std::cout << v.capacity() << std::endl;

	ie::vector<TA_A, ie::malloc_allocator<TA_A, true, true>> v2 = std::move(v);
	//	std::cout << v2.capacity() << std::endl;

	return 0;
}

/*-------------------------------------------------------------------------------------------*/

template <class V>
int
test_vector_erase_test1(V& v)
{
	typedef typename V::value_type TA_A;
	v.resize(8);
	for (int i = 0; i < 8; i++) { v[i] = i; }
	v.erase(v.cend() - 1);

	V ref0({TA_A(0), TA_A(1), TA_A(2), TA_A(3), TA_A(4), TA_A(5), TA_A(6)});
	if (!cmp(v, ref0))
	{
		std::cerr << "erase last failed!" << std::endl;
		return 1;
	}

	v.erase(v.cbegin() + 3);

	V ref({TA_A(0), TA_A(1), TA_A(2), TA_A(4), TA_A(5), TA_A(6)});
	if (!cmp(v, ref))
	{
		std::cerr << "erase begin+3 failed!" << std::endl;
		return 1;
	}

	v.erase(v.cbegin() + 1, v.cbegin() + 4);
	V ref1({TA_A(0), TA_A(5), TA_A(6)});
	if (!cmp(v, ref1))
	{
		std::cerr << "erase range failed" << std::endl;
		return 1;
	}

	v.erase(v.cbegin() + 1);
	V ref5({TA_A(0), TA_A(6)});
	if (!cmp(v, ref5))
	{
		std::cerr << "erase middle failed" << std::endl;
		return 1;
	}

	v.erase(v.cbegin());
	V ref2({TA_A(6)});
	if (!cmp(v, ref2))
	{
		std::cerr << "erase begin failed" << std::endl;
		return 1;
	}

	v.erase(v.cend() - 1);
	V ref4({});
	if (!cmp(v, ref4))
	{
		std::cerr << "erase last bis failed" << std::endl;
		return 1;
	}
	return 0;
}

template <class V>
int
test_vector_erase_test2(V& v)
{
	v.resize(5);
	for (int i = 0; i < 5; i++) { v[i] = i; }
	v.erase(v.cbegin(), v.cend());
	if (v.size() != 0)
	{
		std::cerr << "error erase all!" << std::endl;
		return 1;
	}
	return 0;
}

template <class V>
int
test_vector_erase_test_all()
{
	int ret = 0;
	{
		V v;
		ret |= test_vector_erase_test1(v);
	}
	{
		V v;
		ret |= test_vector_erase_test2(v);
	}
	return ret;
}

int
test_vector_erase()
{
	int ret = 0;
	ret |= test_vector_erase_test_all<vector_int>();
	ret |= test_vector_erase_test_all<vector_int_ea>();
	ret |= test_vector_erase_test_all<vector_a>();
	ret |= test_vector_erase_test_all<vector_a_ea>();

	return ret;
}

/*-------------------------------------------------------------------------------------------*/


struct B;

namespace std {

template <> struct is_pod<B> : public std::false_type
{
};

} // namespace std

int
test_vector_forward_decl()
{
	typedef ie::vector<B> vector_b;
	//	std::cout << sizeof(vector_b) << std::endl;
	//	std::cout << std::is_pod<B>::value << std::endl;
	return 0;
}

/*-------------------------------------------------------------------------------------------*/

template <class V>
int
test_vector_insert_test1(V& v)
{
	typedef typename V::value_type TA_A;
	v.resize(5);
	for (int i = 0; i < 5; i++) { v[i] = i; }
	std::vector<TA_A> ar({TA_A(10), TA_A(11)});
	v.insert(v.cbegin() + 1, ar.begin(), ar.end());

	V ref({TA_A(0), TA_A(10), TA_A(11), TA_A(1), TA_A(2), TA_A(3), TA_A(4)});
	if (!cmp(v, ref))
	{
		std::cerr << "Insert begin+1 failed!" << std::endl;
		return 1;
	}

	v.insert(v.cbegin(), ar.begin(), ar.end());
	V ref1(
	    {TA_A(10), TA_A(11), TA_A(0), TA_A(10), TA_A(11), TA_A(1), TA_A(2), TA_A(3), TA_A(4)});
	if (!cmp(v, ref1))
	{
		std::cerr << "Insert begin failed!" << std::endl;
		return 1;
	}

	v.insert(v.cend(), ar.begin(), ar.end());
	V ref2({TA_A(10), TA_A(11), TA_A(0), TA_A(10), TA_A(11), TA_A(1), TA_A(2), TA_A(3),
	        TA_A(4), TA_A(10), TA_A(11)});
	if (!cmp(v, ref2))
	{
		std::cerr << "Insert end failed!" << std::endl;
		return 1;
	}
	return 0;
}

template <class V>
int
test_vector_insert_test2(V& v)
{
	typedef typename V::value_type TA_A;
	v.resize(2);
	for (int i = 0; i < 2; i++) { v[i] = i; }
	std::vector<TA_A> ar({TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14)});
	v.insert(v.cbegin() + 1, ar.begin(), ar.end());

	V ref({TA_A(0), TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(1)});
	if (!cmp(v, ref))
	{
		std::cerr << "Insert begin+1 failed!" << std::endl;
		return 1;
	}

	v.insert(v.cbegin(), ar.begin(), ar.end());
	V ref1({TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(0), TA_A(10), TA_A(11),
	        TA_A(12), TA_A(13), TA_A(14), TA_A(1)});
	if (!cmp(v, ref1))
	{
		std::cerr << "Insert begin failed!" << std::endl;
		return 1;
	}

	v.insert(v.cend(), ar.begin(), ar.end());
	V ref2(
	    {TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(0), TA_A(10), TA_A(11),
	     TA_A(12), TA_A(13), TA_A(14), TA_A(1), TA_A(10), TA_A(11), TA_A(12), TA_A(13),
	     TA_A(14)});
	if (!cmp(v, ref2))
	{
		std::cerr << "Insert end failed!" << std::endl;
		return 1;
	}

	v.insert(v.cend(), 2, TA_A(20));
	V ref4(
	    {TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(0), TA_A(10), TA_A(11),
	     TA_A(12), TA_A(13), TA_A(14), TA_A(1), TA_A(10), TA_A(11), TA_A(12), TA_A(13),
	     TA_A(14), TA_A(20), TA_A(20)});
	if (!cmp(v, ref4))
	{
		std::cerr << "Insert (count,value) to the end failed!" << std::endl;
		return 1;
	}

	v.emplace(v.cbegin() + 5, 21);
	V ref5(
	    {TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(21), TA_A(0),
	     TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(1),  TA_A(10),
	     TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(20), TA_A(20)});
	if (!cmp(v, ref5))
	{
		std::cerr << "emplace to begin+5 failed!" << std::endl;
		return 1;
	}

	v.insert(v.cbegin(), 2, TA_A(40));
	V ref6(
	    {TA_A(40), TA_A(40), TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(21),
	     TA_A(0),  TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(1),  TA_A(10),
	     TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(20), TA_A(20)});
	if (!cmp(v, ref6))
	{
		std::cerr << "emplace to begin+5 failed!" << std::endl;
		return 1;
	}

	v.insert(v.cbegin() + 1, 2, TA_A(41));
	V ref7(
	    {TA_A(40), TA_A(41), TA_A(41), TA_A(40), TA_A(10), TA_A(11), TA_A(12), TA_A(13),
	     TA_A(14), TA_A(21), TA_A(0),  TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14),
	     TA_A(1),  TA_A(10), TA_A(11), TA_A(12), TA_A(13), TA_A(14), TA_A(20), TA_A(20)});
	if (!cmp(v, ref7))
	{
		std::cerr << "emplace to begin+5 failed!" << std::endl;
		return 1;
	}

	v.insert(v.cbegin(), std::move(TA_A(100)));
	V ref8(
	    {TA_A(100), TA_A(40), TA_A(41), TA_A(41), TA_A(40), TA_A(10), TA_A(11),
	     TA_A(12),  TA_A(13), TA_A(14), TA_A(21), TA_A(0),  TA_A(10), TA_A(11),
	     TA_A(12),  TA_A(13), TA_A(14), TA_A(1),  TA_A(10), TA_A(11), TA_A(12),
	     TA_A(13),  TA_A(14), TA_A(20), TA_A(20)});
	if (!cmp(v, ref8))
	{
		std::cerr << "insert to begin + std::move failed!" << std::endl;
		return 1;
	}

	TA_A a_(101);
	v.insert(v.cbegin() + 4, a_);
	V ref9(
	    {TA_A(100), TA_A(40), TA_A(41), TA_A(41), TA_A(101), TA_A(40), TA_A(10),
	     TA_A(11),  TA_A(12), TA_A(13), TA_A(14), TA_A(21),  TA_A(0),  TA_A(10),
	     TA_A(11),  TA_A(12), TA_A(13), TA_A(14), TA_A(1),   TA_A(10), TA_A(11),
	     TA_A(12),  TA_A(13), TA_A(14), TA_A(20), TA_A(20)});
	if (!cmp(v, ref9))
	{
		std::cerr << "insert to begin + copy failed!" << std::endl;
		return 1;
	}

	v.insert(v.cbegin() + 7, {TA_A(60), TA_A(61)});
	V ref10(
	    {TA_A(100), TA_A(40), TA_A(41), TA_A(41), TA_A(101), TA_A(40), TA_A(10),
	     TA_A(60),  TA_A(61), TA_A(11), TA_A(12), TA_A(13),  TA_A(14), TA_A(21),
	     TA_A(0),   TA_A(10), TA_A(11), TA_A(12), TA_A(13),  TA_A(14), TA_A(1),
	     TA_A(10),  TA_A(11), TA_A(12), TA_A(13), TA_A(14),  TA_A(20), TA_A(20)});
	if (!cmp(v, ref10))
	{
		std::cerr << "insert to begin + copy failed!" << std::endl;
		return 1;
	}
	return 0;
}

template <class V>
int
test_vector_insert_test_no_reserve()
{
	int ret = 0;
	{
		V v;
		ret |= test_vector_insert_test1(v);
	}
	{
		V v;
		ret |= test_vector_insert_test2(v);
	}
	return ret;
}

template <class V>
int
test_vector_insert_test_reserve()
{
	int ret = 0;
	{
		V v;
		v.reserve(100);
		ret |= test_vector_insert_test1(v);
	}
	{
		V v;
		v.reserve(100);
		ret |= test_vector_insert_test2(v);
	}
	return ret;
}

int
test_vector_insert()
{
	int ret = 0;
	ret |= test_vector_insert_test_no_reserve<vector_int>();
	ret |= test_vector_insert_test_reserve<vector_int>();
	ret |= test_vector_insert_test_no_reserve<vector_a>();
	ret |= test_vector_insert_test_reserve<vector_a>();

	ret |= test_vector_insert_test_no_reserve<vector_int_ea>();
	ret |= test_vector_insert_test_reserve<vector_int_ea>();
	ret |= test_vector_insert_test_no_reserve<vector_a_ea>();
	ret |= test_vector_insert_test_reserve<vector_a_ea>();

	return ret;
}

/*-------------------------------------------------------------------------------------------*/

int
test_vector_instance()
{
	// Non-pod structure
	struct AA
	{
		AA() : i(4) { }

		int i;
	};

	static_assert(std::is_pod<AA>::value == false, "TA_A shouldn't be a POD");

	ie::vector<int> v;
	ie::vector<AA> v2;
	ie::vector<AA, std::allocator<int>> v4;

	return 0;
}

/*-------------------------------------------------------------------------------------------*/


template <bool is_const, class T>
int
test_vector_iterators_test(T& v)
{
	typedef typename std::conditional<
	    is_const, typename T::const_iterator, typename T::iterator>::type iterator_type;

	int ret = 0;
	int c = 0;
	for (int i : v)
	{
		if (i != c)
		{
			std::cerr << "error at " << c << std::endl;
			ret = 1;
		}
		c++;
	}

	c = 19;
	for (auto it = v.rbegin(); it != v.rend(); it++)
	{
		if (*it != c)
		{
			std::cerr << "error with reverse at " << c << std::endl;
			ret = 1;
		}
		c--;
	}

	{
		iterator_type it;
		it = v.end();
		--it;
		if (*it != 19)
		{
			std::cerr << "error with --iterator" << std::endl;
			ret = 1;
		}
		iterator_type prev_it = it--;
		if (*it != 18 || *prev_it != 19)
		{
			std::cerr << "error with --iterator" << std::endl;
			ret = 1;
		}

		it -= 10;
		if (*it != 8)
		{
			std::cerr << "error with iterator-=10" << std::endl;
			ret = 1;
		}
	}

	{
		iterator_type it;
		it = v.begin();
		++it;
		if (*it != 1)
		{
			std::cerr << "error with ++iterator" << std::endl;
			ret = 1;
		}
		iterator_type prev_it = it++;
		if (*it != 2 || *prev_it != 1)
		{
			std::cerr << "error with --iterator" << std::endl;
			ret = 1;
		}
		it += 15;
		if (*it != 17)
		{
			std::cerr << "error with iterator+=15" << std::endl;
			ret = 1;
		}
	}

	{
		iterator_type it1 = v.begin();
		iterator_type it2 = v.begin();
		it2 += 5;
		if ((it2 - it1) != 5)
		{
			std::cerr << "error on iterator-iterator" << std::endl;
			ret = 1;
		}
		if ((it1 - it2) != -5)
		{
			std::cerr << "error on iterator-iterator" << std::endl;
			ret = 1;
		}
	}
	return ret;
}

int
test_vector_iterators()
{
	int ret = 0;
	vector_int v;
	for (int i = 0; i < 20; i++) { v.push_back(i); }

	ret |= test_vector_iterators_test<false>(v);
	ret |= test_vector_iterators_test<true>(const_cast<vector_int const&>(v));

	v.clear();
	int i = 0;
	for (auto it = v.rbegin(); it != v.rend(); it++) { i++; }
	for (int i_ : v)
	{
		(void)i_;
		i++;
	}
	vector_int v2;
	for (auto it = v.rbegin(); it != v.rend(); it++) { i++; }
	for (int i_ : v2)
	{
		(void)i_;
		i++;
	}
	if (i != 0)
	{
		std::cerr << "empty container failure" << std::endl;
		ret = 1;
	}

	return ret;
}

/*-------------------------------------------------------------------------------------------*/

int
test_vector_min_construct()
{
	ie::vector<
	    int, ie::malloc_allocator<int, true, false>, size_t, ie::recommended_size_dummy, false>
	    v;
	v.emplace_back(10);
	v.emplace_back(10);

	return !(v.front() == 10);
}

/*-------------------------------------------------------------------------------------------*/

int
test_vector_overflow_multiply_by()
{
	ie::vector<TA_A, std::allocator<TA_A>, uint8_t, ie::recommended_size_multiply_by<3, 2>> v;
	// 169 is the limit before we will be close to max_size
	v.resize(169);
	v.emplace_back(1);
	if (v.capacity() != 254)
	{
		std::cerr << "overflow occurred in the capacity computation!" << std::endl;
		return 1;
	}
	v.resize_fit(180);
	v.emplace_back(1);
	if (v.capacity() != 255)
	{
		std::cerr << "overflow occurred in the capacity computation!" << std::endl;
		return 1;
	}
	v.resize(254);
	v.emplace_back(1);
	try
	{
		v.emplace_back(2);
	}
	catch (std::length_error const&)
	{
		//		std::cerr << "got overflow exception" << std::endl;
		return 0;
	}

	std::cerr << "should have caught an overflow exception!" << std::endl;
	return 1;
}

int
test_vector_overflow_add_by()
{
	ie::vector<TA_A, std::allocator<TA_A>, uint8_t, ie::recommended_size_add_by<50>> v;
	// 169 is the limit before we will be close to max_size
	v.resize(205);
	v.emplace_back(1);
	if (v.capacity() != 255)
	{
		std::cerr << "overflow occurred in the capacity computation!" << std::endl;
		return 1;
	}
	return 0;
}

int
test_vector_overflow()
{
	int ad = test_vector_overflow_add_by();
	int mb = test_vector_overflow_multiply_by();
	return ad | mb;
}

/*-------------------------------------------------------------------------------------------*/


int
test_vector_recommended_sizes()
{
	ie::vector<int, std::allocator<int>, size_t, ie::recommended_size_add_by<5>, false> v;
	v.emplace_back(0);
	v.emplace_back(1);
	if (v.capacity() != 5)
	{
		std::cerr << "capacity should be of 5" << std::endl;
		return 1;
	}
	for (int i = 0; i < 7; i++) { v.emplace_back(i + 2); }

	if (v.capacity() != 10)
	{
		std::cerr << "capacity should be of 10" << std::endl;
		return 1;
	}

	return 0;
}

/*-------------------------------------------------------------------------------------------*/

template <class V>
int
test_vector_resize_test_no_ea()
{
	typedef typename V::value_type TA_A;
	V v;
	v.resize(10);
	TA_A* pfirst = &v[0];
	for (int i = 0; i < 10; i++) { v[i] = i; }
	v.shrink_to_fit();
	if (&v[0] != pfirst)
	{
		std::cerr << "shrink_to_fit should'nt have done nothing!" << std::endl;
		return 1;
	}

	v.resize_fit(4);
	if (&v[0] == pfirst)
	{
		std::cerr << "pointer to data stored should'nt be the same!" << std::endl;
		return 0;
	}
	V vref({TA_A(0), TA_A(1), TA_A(2), TA_A(3)});
	if (!cmp(v, vref))
	{
		std::cerr << "invalid values after resize_fit" << std::endl;
		return 1;
	}

	return 0;
}

template <class V>
int
test_vector_resize_test_ea()
{
	typedef typename V::value_type TA_A;
	V v;
	v.resize(10);
	for (int i = 0; i < 10; i++) { v[i] = i; }
	v.shrink_to_fit();
	v.resize_fit(4);
	V vref({TA_A(0), TA_A(1), TA_A(2), TA_A(3)});
	if (!cmp(v, vref))
	{
		std::cerr << "invalid values after resize_fit" << std::endl;
		return 1;
	}
	v.resize_no_construct(6);
	new (&v[4]) TA_A(4);
	new (&v[5]) TA_A(5);
	V vref2({TA_A(0), TA_A(1), TA_A(2), TA_A(3), TA_A(4), TA_A(5)});
	if (!cmp(v, vref2))
	{
		std::cerr << "invalid values after resize_no_construct" << std::endl;
		return 1;
	}

	return 0;
}

int
test_vector_resize()
{
	int ret = 0;

	ret |= test_vector_resize_test_no_ea<vector_int>();
	ret |= test_vector_resize_test_no_ea<vector_a>();

	ret |= test_vector_resize_test_ea<vector_int_ea>();
	ret |= test_vector_resize_test_ea<vector_a_ea>();

	return ret;
}

/*-------------------------------------------------------------------------------------------*/


template <class Iter>
int
test_vector_sort_check_vec(Iter begin, Iter end)
{
	int ret = 0;
	int i = 0;
	for (; begin != end; begin++)
	{
		if (*begin != i)
		{
			std::cerr << "error at " << i << ": " << *begin << std::endl;
			ret = 1;
		}
		i++;
	}
	return ret;
}

int
test_vector_sort()
{
	int ret = 0;
	vector_int v;
	v.resize(40);
	for (int i = 0; i < 40; i++) { v[i] = i; }

	std::random_shuffle(v.begin(), v.end());
	std::sort(v.begin(), v.end());
	ret |= test_vector_sort_check_vec(v.begin(), v.end());

	std::random_shuffle(v.rbegin(), v.rend());
	std::sort(v.rbegin(), v.rend());
	ret |= test_vector_sort_check_vec(v.rbegin(), v.rend());

	return ret;
}

/*-------------------------------------------------------------------------------------------*/


void
test_vector()
{
	assert(!test_vector_sort());
	assert(!test_vector_assign());
	assert(!test_vector_resize());
	assert(!test_vector_recommended_sizes());
	assert(!test_vector_overflow());
	assert(!test_vector_min_construct());
	assert(!test_vector_iterators());
	assert(!test_vector_instance());
	assert(!test_vector_insert());
	assert(!test_vector_forward_decl());
	assert(!test_vector_erase());
	assert(!test_vector_enhanced_allocators());
	assert(!test_vector_copy_move());
	assert(!test_vector_assign());
	assert(!test_vector_append());
}

int
main()
{
	test_vector();
	return 0;
}
