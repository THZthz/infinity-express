#pragma once

#include <numeric>
#include <array>
#include "utils/spatial.hpp"

// example of a cache friendly allocator with contiguous memory
template <class NodeClass, bool IsDebugMode = false, size_t DefaultNodeCount = 100>
struct tree_allocator
{
	typedef NodeClass value_type;
	typedef NodeClass* ptr_type;

	enum
	{
		is_overflowable = 0
	};

	tree_allocator() : buffer(DefaultNodeCount), count(0), index(0) { }

	ptr_type allocate(int level)
	{
		if (IsDebugMode)
			std::cout << "Allocate node: " << level << " current count: " << count << "\n";
		++count;
		assert(index + 1 <= buffer.size());
		ptr_type node = &buffer[index++];
		*node = NodeClass(level);
		return node;
	}

	void deallocate(const ptr_type node, size_t /*n*/)
	{
		if (IsDebugMode)
			std::cout
			    << "Deallocate node: " << node->level << " - " << node->count
			    << " current count: " << count << "\n";

		assert(count > 0);
		if (count > 1) --count;
		else
		{
			count = 0;
			index = 0;
		}
	}

	void clear()
	{
		buffer.clear();
		count = index = 0;
	}
	void resize(size_t count) { buffer.resize(count); }

	bool overflowed() const { return false; }

	std::vector<NodeClass> buffer;
	size_t count;
	size_t index;
};

template <class NodeClass> struct heap_allocator
{
	typedef NodeClass value_type;
	typedef NodeClass* ptr_type;

	enum
	{
		is_overflowable = 0
	};

	heap_allocator() : count(0) { }

	ptr_type allocate(int level)
	{
		++count;
		return new NodeClass(level);
	}

	void deallocate(const ptr_type node)
	{
		assert(node);
		delete node;
	}

	bool overflowed() const { return false; }

	size_t count;
};










template <typename T> struct Point
{
	union
	{
		glm::vec<2, T> data;
		struct
		{
			T x, y;
		};
	};

	Point() = default;
	Point(T x0, T y0) : x(x0), y(y0) { }
	//	Point(const glm::vec<2, T>& v) : data(v) { }
	Point(const Point& other) : x(other.x), y(other.y) { }

	void set(T x, T y)
	{
		this->x = x;
		this->y = y;
	}

	inline float distance(const Point point) const
	{
		float d = float(data[0] - point.data[0]);
		d *= d;
		for (int i = 1; i < 2; i++)
		{
			float temp = float(data[i] - point.data[i]);
			d += temp * temp;
		}
		return std::sqrt(d);
	}
};

template <typename T> struct Box2
{
	glm::vec<2, T> l, u;

	explicit operator ie::TBox<int, 2>() const { return ie::TBox<int, 2>(l, u); }

	bool operator==(const Box2& other) const
	{
		return l[0] == other.l[0] && l[1] == other.l[1] && u[0] == other.u[0] &&
		       u[1] == other.u[1];
	}
};

template <typename T>
std::ostream&
operator<<(std::ostream& stream, const Box2<T>& bbox)
{
	stream << "min: " << bbox.l[0] << " " << bbox.l[1] << " ";
	stream << "max: " << bbox.u[0] << " " << bbox.u[1];
	return stream;
}

struct Object
{
	ie::TBox<int, 2> bbox;
	std::string name;

	// needed to avoid adding duplicates
	bool operator==(const Object& other) const { return name == other.name; }
};

typedef ie::RTree<int, Box2<int>, 2, 4, 2> rtree_box_t;

const Box2<int> kBoxes[] = {
    {{5, 2}, {16, 7}},      {{1, 1}, {2, 2}},      {{26, 24}, {44, 28}},
    {{22, 21}, {23, 24}},   {{16, 0}, {32, 16}},   {{0, 0}, {8, 8}},
    {{4, 4}, {6, 8}},       {{2, 1}, {2, 3}},      {{4, 2}, {8, 4}},
    {{3, 3}, {12, 16}},     {{0, 0}, {64, 32}},    {{3, 2}, {32, 35}},
    {{32, 32}, {64, 128}},  {{128, 0}, {256, 64}}, {{120, 64}, {250, 128}},
    {{123, 84}, {230, 122}}};

// use a custom indexable for custom objects
struct Indexable
{
	const glm::vec<2, int>& min(const Object& value) const { return value.bbox.getLower(); }
	const glm::vec<2, int>& max(const Object& value) const { return value.bbox.getUpper(); }
};

std::vector<Object> objects;
std::vector<Box2<int>> boxes;
rtree_box_t rtree;

void
test_rtree_create()
{
	size_t i = 0;
	// MESSAGE("Creating objects:\n");
	for (const auto& bbox : kBoxes)
	{
		boxes.push_back(bbox);
		objects.emplace_back();
		Object& obj = objects.back();

		std::stringstream ss;
		ss << "object" << i++;
		obj.name = ss.str();
		obj.bbox = (ie::TBox<int, 2>)bbox;

		// MESSAGE(obj.name << " " << obj.bbox << "\n");
	}

	// create a quad tree with the given box
	ie::TBox<int, 2> bbox(0);
	Point<int> point;
	point.set(0, 0);
	bbox.extend(point.data);
	point.set(256, 128);
	bbox.extend(point.data);

	assert(rtree.count() == 0u);
	rtree = rtree_box_t(std::begin(kBoxes), std::end(kBoxes));
	assert(rtree.count() == 16u);
	rtree.clear();
	assert(rtree.count() == 0u);

	// or construction via insert
	rtree.insert(std::begin(kBoxes), std::end(kBoxes));
	assert(rtree.count() == 16u);
	Box2<int> box = {{7, 3}, {14, 6}};
	rtree.insert(box);
	assert(rtree.count() == 17u);

	// SUBCASE("conditional insert")
	{
		// insert only if predicate is always valid
		Box2<int> box2 = {{7, 4}, {14, 6}};
		bool wasAdded = rtree.insert(box2, [&box2](const decltype(rtree)::bbox_type& bbox) {
			const decltype(rtree)::bbox_type cbbox(box2.l, box2.u);
			return !bbox.overlaps(cbbox);
		});
		assert(!wasAdded);
		assert(rtree.count() == 17u);

		wasAdded = rtree.insert(box, [&box](const decltype(rtree)::bbox_type& bbox) {
			const decltype(rtree)::bbox_type cbbox(box.l, box.u);
			return !bbox.overlaps(cbbox);
		});
		assert(!wasAdded);
		assert(rtree.count() == 17u);
	}
}

void
test_rtree_create2()
{
	Box2<int> box = {{7, 3}, {14, 6}};

	// MESSAGE("Created trees, element count: " << rtree.count() << "\n");
	assert(rtree.count() == 17u);
	rtree_box_t::bbox_type treeBBox = rtree.bbox();
	assert(treeBBox.l[0] == 0);
	assert(treeBBox.l[1] == 0);
	assert(treeBBox.u[0] == 256);
	assert(treeBBox.u[1] == 128);

	// remove an element
	bool removed = rtree.remove(box);
	assert(removed);
	assert(rtree.count() == 16u);

	std::vector<Box2<int>> results;
	removed = rtree.remove(box);
	assert(!removed);
	results.clear();
	rtree.query(ie::contains<2>(box.l, box.u), std::back_inserter(results));
	assert(results.empty());

	box = {{0, 0}, {20, 50}};
	rtree.query(ie::contains<2>(box.l, box.u), std::back_inserter(results));
	assert(!results.empty());
	assert(results.size() == 7);
}

void
test_query_for_results_within_the_search_box()
{
	Box2<int> searchBox = {{0, 0}, {8, 31}};
	std::vector<Box2<int>> results;
	rtree.query(ie::intersects<2>(searchBox.l, searchBox.u), std::back_inserter(results));

	std::stringstream resultsStream;
	for (const auto& res : results) resultsStream << res << ", ";
	assert(
	    resultsStream.str() ==
	    "min: 3 3 max: 12 16, min: 0 0 max: 64 32, min: 3 2 max: 32 "
	    "35, min: 1 1 max: 2 2, min: 2 1 max: 2 3, min: 5 2 max: 16 "
	    "7, min: 0 0 max: 8 8, min: 4 4 max: 6 8, min: 4 2 max: 8 "
	    "4, ");

	results.clear();
	rtree.query(ie::contains<2>(searchBox.l, searchBox.u), std::back_inserter(results));
	resultsStream.clear();

	for (const auto& res : results) resultsStream << res << ", ";
	assert(
	    resultsStream.str() ==
	    "min: 3 3 max: 12 16, min: 0 0 max: 64 32, min: 3 2 max: 32 "
	    "35, min: 1 1 max: 2 2, min: 2 1 max: 2 3, min: 5 2 max: 16 "
	    "7, min: 0 0 max: 8 8, min: 4 4 max: 6 8, min: 4 2 max: 8 "
	    "4, min: 1 1 max: 2 2, min: 2 1 max: 2 3, min: 0 0 max: 8 "
	    "8, min: 4 4 max: 6 8, min: 4 2 max: 8 4, ");
}

void
test_ray_query()
{
	Point<float> rayOrigin({0.5, 0});
	Point<float> rayDir({0, 1});
	std::vector<Box2<int>> results;
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));

	std::stringstream resultsStream;
	for (const auto& res : results) resultsStream << res << ", ";
	assert(resultsStream.str() == "min: 0 0 max: 64 32, min: 0 0 max: 8 8, ");

	rayOrigin = Point<float>({100.5, 0.5});
	rayDir = Point<float>({0, 1});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));
	assert(results.empty());

	rayOrigin = Point<float>({68, 130});
	rayDir = Point<float>({120, 52});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));
	resultsStream.str("");
	for (const auto& res : results) resultsStream << res << ", ";
	assert(resultsStream.str() == "min: 32 32 max: 64 128, ");

	rayOrigin = Point<float>({11, 110});
	rayDir = Point<float>({1, 0});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));
	resultsStream.str("");
	for (const auto& res : results) resultsStream << res << ", ";
	assert(
	    resultsStream.str() ==
	    "min: 32 32 max: 64 128, min: 123 84 max: 230 122, min: 120 "
	    "64 max: 250 128, ");

	rayOrigin = Point<float>({63, 25});
	rayDir = Point<float>({0, 2});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));
	resultsStream.str("");
	for (const auto& res : results) resultsStream << res << ", ";
	assert(resultsStream.str() == "min: 32 32 max: 64 128, min: 0 0 max: 64 32, ");

	rayOrigin = Point<float>({62, 70});
	rayDir = Point<float>({0, -2});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));
	resultsStream.str("");
	for (const auto& res : results) resultsStream << res << ", ";
	assert(resultsStream.str() == "min: 32 32 max: 64 128, min: 0 0 max: 64 32, ");

	auto fnTestPredicate = [](const Box2<int>& box) { return box.l[0] == 0; };
	rayOrigin = Point<float>({62, 70});
	rayDir = Point<float>({0, -2});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results), fnTestPredicate);
	resultsStream.str("");
	for (const auto& res : results) resultsStream << res << ", ";
	assert(resultsStream.str() == "min: 0 0 max: 64 32, ");

	rayOrigin = Point<float>({65, 70});
	rayDir = Point<float>({0, -2});
	results.clear();
	rtree.rayQuery(rayOrigin.data, rayDir.data, std::back_inserter(results));
	assert(results.empty());
}

void
test_tree_traversal()
{
	ie::RTree<int, Object, 2, 4, 2, Indexable> rtree;
	rtree.insert(objects.begin(), objects.end());

	// SUBCASE("leaf traversal of the tree")
	{
		ie::	RTree<int, Object, 2, 4, 2, Indexable> rtree;
		rtree.insert(objects.begin(), objects.end());

		std::stringstream resultsStream;
		for (const auto& obj : objects) { resultsStream << obj.name << " "; }
		assert(
		    resultsStream.str() ==
		    "object0 object1 object2 object3 object4 object5 "
		    "object6 object7 object8 object9 object10 object11 "
		    "object12 object13 object14 object15 ");

		// gives the spatial partitioning order within the tree
		resultsStream.clear();
		for (auto it = rtree.lbegin(); it.valid(); it.next())
		{
			resultsStream << (*it).name << " ";
		}
		assert(
		    resultsStream.str() ==
		    "object0 object1 object2 object3 object4 object5 object6 object7 object8 "
		    "object9 object10 object11 object12 object13 object14 object15 object2 object3 "
		    "object12 object15 object9 object10 object11 object13 object14 object1 object7 "
		    "object0 object4 object5 object6 object8 ");
	}

	// SUBCASE("depth traversal of the tree")
	{
		assert(rtree.levels() > 0);
		std::stringstream resultsStream;
		for (auto it = rtree.dbegin(); it.valid(); it.next())
		{
			std::string parentName;
			ie::TBox<int, 2> parentBBox;

			// traverse current children of the parent node(i.e. upper level)
			for (auto nodeIt = it.child(); nodeIt.valid(); nodeIt.next())
			{
				resultsStream << "level: " << nodeIt.level() << " " << (*nodeIt).name << " | ";
				parentName += (*nodeIt).name + " + ";
				parentBBox.extend(nodeIt.bbox());
			}
			(*it).name = parentName;
			(*it).bbox = parentBBox;
			resultsStream << "level: " << it.level() << " " << parentName << " \n ";
		}
		assert(
		    resultsStream.str() ==
		    "level: 0 object2 | level: 0 object3 | level: 0 object12 | level: 0 object15 | "
		    "level: 1 object2 + object3 + object12 + object15 +  \n level: 0 object9 | level: "
		    "0 object10 | level: 0 object11 | level: 1 object9 + object10 + object11 +  \n "
		    "level: 0 object13 | level: 0 object14 | level: 1 object13 + object14 +  \n "
		    "level: 1 object2 + object3 + object12 + object15 +  | level: 1 object9 + "
		    "object10 + object11 +  | level: 1 object13 + object14 +  | level: 2 object2 + "
		    "object3 + object12 + object15 +  + object9 + object10 + object11 +  + object13 + "
		    "object14 +  +  \n level: 0 object1 | level: 0 object7 | level: 1 object1 + "
		    "object7 +  \n level: 0 object0 | level: 0 object4 | level: 1 object0 + object4 + "
		    " \n level: 0 object5 | level: 0 object6 | level: 0 object8 | level: 1 object5 + "
		    "object6 + object8 +  \n level: 1 object1 + object7 +  | level: 1 object0 + "
		    "object4 +  | level: 1 object5 + object6 + object8 +  | level: 2 object1 + "
		    "object7 +  + object0 + object4 +  + object5 + object6 + object8 +  +  \n ");
	}

	// SUBCASE("special hierarchical query")
	{
		std::vector<Object> results;
		Box2<int> searchBox = {{4, 14}, {8, 31}};
		rtree.hierachical_query(
		    ie::intersects<2>(searchBox.l, searchBox.u), std::back_inserter(results));
		std::stringstream resultsStream;
		for (const auto& res : results) resultsStream << res.name << " | ";

		assert(resultsStream.str() == "object9 | object10 | object11 | ");
	}
}

void
test_nearest_neighbor_query()
{
	ie::	RTree<int, Object, 2, 4, 2, Indexable> rtree;
	rtree.insert(objects.begin(), objects.end());
	Point<int> p = {0, 0};
	// SUBCASE("nearest neighbor radius query")
	{
		std::vector<Object> results;
		rtree.nearest(p.data, 100, std::back_inserter(results));
		std::stringstream resultsStream;
		for (const auto& res : results)
		{
			Point<int> center;
			center.data = res.bbox.center();
			resultsStream << res.name << " : " << p.distance(center) << " | ";
		}
		assert(
		    resultsStream.str() ==
		    "object1 : 1.41421 | object7 : 2.82843 | object5 : 5.65685 | object8 : 6.7082 "
		    "| object6 : 7.81025 | object0 : 10.7703 | object4 : 25.2982 | object9 : "
		    "11.4018 | object11 : 24.7588 | object10 : 35.7771 | object3 : 31.1127 | "
		    "object2 : 43.6005 | object12 : 93.2952 | ");
	}

	// SUBCASE("Nearest knn query:")
	{
		std::stringstream resultsStream;
		std::vector<Object> results;
		rtree.k_nearest(p.data, 3, std::back_inserter(results));
		for (const auto& res : results)
			resultsStream << res.name << " : " << res.bbox.distance(p.data) << " | ";
		assert(resultsStream.str() == "object10 : 0 | object5 : 0 | object1 : 1.41421 | ");
	}
}

void
test_custom_indexable_for_array_and_indices_as_values()
{
	struct ArrayIndexable
	{
		ArrayIndexable(const std::vector<Box2<int>>& array) : array(array) { }

		const glm::vec<2, int>& min(const uint32_t index) const { return array[index].l; }
		const glm::vec<2, int>& max(const uint32_t index) const { return array[index].u; }

		const std::vector<Box2<int>>& array;
	};

	ArrayIndexable indexable(boxes);

	typedef uint32_t IndexType;
	ie::	RTree<int, IndexType, 2, 4, 2, ArrayIndexable> rtree(indexable);

	std::vector<uint32_t> indices(boxes.size());
	std::iota(indices.begin(), indices.end(), 0);
	rtree.insert(indices.begin(), indices.end());

	indices.clear();
	Box2<int> searchBox = {{0, 0}, {8, 31}};
	rtree.query(	ie::intersects<2>(searchBox.l, searchBox.u), std::back_inserter(indices));
	std::stringstream resultsStream;
	for (auto index : indices)
		resultsStream
		    << "index: " << index << " " << objects[index].name << " " << objects[index].bbox
		    << " | ";
	assert(
	    resultsStream.str() ==
	    "index: 9 object9 min: 3 3 max: 12 16 | index: 10 object10 "
	    "min: 0 0 max: 64 32 | index: 11 object11 min: 3 2 max: 32 "
	    "35 | index: 1 object1 min: 1 1 max: 2 2 | index: 7 object7 "
	    "min: 2 1 max: 2 3 | index: 0 object0 min: 5 2 max: 16 7 | "
	    "index: 5 object5 min: 0 0 max: 8 8 | index: 6 object6 min: "
	    "4 4 max: 6 8 | index: 8 object8 min: 4 2 max: 8 4 | ");
}

void
test_spatial_custom_allocator()
{
	struct ArrayIndexable
	{
		ArrayIndexable(const std::vector<Box2<int>>& array) : array(array) { }

		const glm::vec<2, int>& min(const uint32_t index) const { return array[index].l; }
		const glm::vec<2, int>& max(const uint32_t index) const { return array[index].u; }

		const std::vector<Box2<int>>& array;
	};

	const int kMaxKeysPerNode = 4;
	const int kVolumeMode =	ie::detail:: eSphericalVolume;

	typedef ie::TBox<int, 2> tree_bbox_type;
	typedef 	ie::detail::Node<uint32_t, tree_bbox_type, kMaxKeysPerNode> tree_node_type;
	typedef tree_allocator<tree_node_type> tree_allocator_type;

	typedef 	ie::RTree<
	    int, uint32_t, 2, kMaxKeysPerNode, kMaxKeysPerNode / 2, ArrayIndexable, kVolumeMode,
	    double, tree_allocator_type>
	    CustomTree;

	ArrayIndexable indexable(boxes);
	std::vector<uint32_t> indices(boxes.size() / 2);
	CustomTree rtree(indexable, tree_allocator_type(), true);

	indices.resize(boxes.size() / 2);
	std::iota(indices.begin(), indices.end(), 0);
	rtree.insert(indices.begin(), indices.end());
	std::stringstream resultsStream;
	for (auto index : indices)
		resultsStream
		    << "index: " << index << " " << objects[index].name << " " << objects[index].bbox
		    << " | ";
	assert(
	    resultsStream.str() ==
	    "index: 0 object0 min: 5 2 max: 16 7 | index: 1 object1 min: 1 1 max: 2 2 | index: "
	    "2 object2 min: 26 24 max: 44 28 | index: 3 object3 min: 22 21 max: 23 24 | index: "
	    "4 object4 min: 16 0 max: 32 16 | index: 5 object5 min: 0 0 max: 8 8 | index: 6 "
	    "object6 min: 4 4 max: 6 8 | index: 7 object7 min: 2 1 max: 2 3 | ");
}










const Box2<int> kBoxes2[] = {
    {{5, 2}, {16, 7}},      {{1, 1}, {2, 2}},      {{26, 24}, {44, 28}},
    {{22, 21}, {23, 24}},   {{16, 0}, {32, 16}},   {{0, 0}, {8, 8}},
    {{4, 4}, {6, 8}},       {{2, 1}, {2, 3}},      {{4, 2}, {8, 4}},
    {{3, 3}, {12, 16}},     {{0, 0}, {64, 32}},    {{3, 2}, {32, 35}},
    {{32, 32}, {64, 128}},  {{128, 0}, {256, 64}}, {{120, 64}, {250, 128}},
    {{123, 84}, {230, 122}}};

template <typename T>
std::ostream&
operator<<(std::ostream& os, const Box2<T> box)
{
	os << "{{" << box.min[0] << ", " << box.min[1] << "}, {" << box.max[0] << ", "
	   << box.max[1] << "}}";
	return os;
}

void
test_spatial_count()
{
	glm::vec<2, int> min{0, 0};
	glm::vec<2, int> max{1, 1};
	ie::TBox<int, 2> bbox{min, max};
	ie::QuadTree<int, Box2<int>> qtree{bbox.l, bbox.u};
	Box2<int> box{{0, 0}, {1, 1}};

	// SUBCASE("after value insert")
	{
		for (int inserts{0}; inserts < 3; ++inserts)
		{
			assert(qtree.count() <= 1);
			qtree.insert(box);
		}
	}

//	// SUBCASE("after iterator insert") // FIXME
//	{
//		std::array<Box2<int>, 2> boxes{box, box};
//
//		int expected_count{0};
//		for (int inserts{0}; inserts < 2; ++inserts)
//		{
////			CAPTURE(inserts);
//			qtree.insert(std::begin(boxes) + inserts, std::begin(boxes) + inserts + 1);
//			expected_count += inserts;
//			assert(qtree.count() == expected_count);
//		}
//	}
}

void
test_spatial_query()
{
	glm::vec<2, int> min{-2, -1};
	glm::vec<2, int> max{9, 10};
	ie::TBox<int, 2> bbox{min, max};
	ie::QuadTree<int, Box2<int>> qtree{bbox.l, bbox.u};

	// SUBCASE("with empty tree")
	{
		assert(qtree.query(intersects(bbox)) == false);
	}

	Box2<int> box{{0, 1}, {5, 6}};
	qtree.insert(box);

	// SUBCASE("with lower left corner touching")
	{
		glm::vec<2, int> corner_min{-1, 0};
		glm::vec<2, int> corner_max{0, 1};
		ie::TBox<int, 2> qbox{corner_min, corner_max};
		assert(qtree.query(intersects(qbox)) == true);
	}

	// SUBCASE("with upper right corner touching")
	{
		glm::vec<2, int> corner_min{5, 6};
		glm::vec<2, int> corner_max{6, 7};
		ie::TBox<int, 2> qbox{corner_min, corner_max};
		assert(qtree.query(intersects(qbox)) == true);
	}

	// SUBCASE("with non-intersecting box")
	{
		std::array<Box2<int>, 4> queries{
		    Box2<int>{{0, -1}, {1, 0}}, Box2<int>{{-2, 1}, {-1, 2}}, Box2<int>{{5, 7}, {6, 8}},
		    Box2<int>{{6, 5}, {7, 8}}};

		for (const auto query : queries)
		{
//			CAPTURE(query);
			ie::TBox<int, 2> qbox{query};
			assert(qtree.query(intersects(qbox)) == false);
		}
	}
}

void
test_spatial_insert()
{
	// create a quad tree with the given box
	ie::TBox<int, 2> bbox(0);
	Point<int> point;
	point.set(0, 0);
	bbox.extend(point.data);
	point.set(256, 128);
	bbox.extend(point.data);

	typedef ie::QuadTree<int, Box2<int>, 2> qtree_box_t;
	qtree_box_t qtree(bbox.l, bbox.u);

	assert(qtree.count() == 0u);
	qtree = qtree_box_t(bbox.l, bbox.u, std::begin(kBoxes2), std::end(kBoxes2));
	assert(qtree.count() == 16u);
	qtree.clear();
	assert(qtree.count() == 0u);

	// or construction via insert
	qtree.insert(std::begin(kBoxes2), std::end(kBoxes2));
	assert(qtree.count() == 16u);
	Box2<int> box = {{7, 3}, {14, 6}};
	qtree.insert(box);
	assert(qtree.count() == 17u);

	// SUBCASE("count and remove")
	{
		// remove an element
		bool removed = qtree.remove(box);
		assert(removed);

		removed = qtree.remove(box);
		assert(!removed);
		std::vector<Box2<int>> results;
		qtree.query(ie::contains<2>(box.l, box.u), std::back_inserter(results));
		assert(results.empty());

		box = {{0, 0}, {20, 50}};
		qtree.query(ie::contains<2>(box.l, box.u), std::back_inserter(results));
		assert(!results.empty());
		assert(results.size() == 7);
	}
}









void
test_spatial()
{
	test_rtree_create();
	test_rtree_create2();

	test_query_for_results_within_the_search_box();

	test_ray_query();

	test_tree_traversal();

	test_nearest_neighbor_query();

	test_custom_indexable_for_array_and_indices_as_values();

	test_spatial_custom_allocator();




	test_spatial_count();
	test_spatial_query();
	test_spatial_insert();
}
