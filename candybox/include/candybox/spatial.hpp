#ifndef CANDYBOX_SPATIAL_HPP__
#define CANDYBOX_SPATIAL_HPP__

#include <cstdint>
#include <vector>
#include <queue>
#include <algorithm>
#include "candybox/AABB.hpp"

namespace candybox {
/*███████╗██████╗  █████╗ ████████╗██╗ █████╗ ██╗     */
/*██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║     */
/*███████╗██████╔╝███████║   ██║   ██║███████║██║     */
/*╚════██║██╔═══╝ ██╔══██║   ██║   ██║██╔══██║██║     */
/*███████║██║     ██║  ██║   ██║   ██║██║  ██║███████╗*/
/*╚══════╝╚═╝     ╚═╝  ╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝*/

//! \defgroup Spatial
//! @{

/// Packed Hilbert R-tree.
/// You can not add or remove items after initialization, but it is fast to create a new
/// one.
/// https://en.wikipedia.org/wiki/Hilbert_R-tree#Packed_Hilbert_R-trees
class Spatial
{
	explicit Spatial(uint32_t n);
	~Spatial();

	uint32_t add(float minx, float miny, float maxx, float maxy);

	void addAll(float *data, uint32_t nFloat);

	void finish();

	void
	search(float minx, float miny, float maxx, float maxy, std::vector<uint32_t> &results);

	void neighbors(
	    float x,
	    float y,
	    float maxDist,
	    uint32_t maxNeighbors,
	    std::vector<uint32_t> &neighbors);

private:
	float *m_boxes; ///< minx, miny, maxx, maxy
	uint32_t m_numBoxes; ///< Number of floats in "boxes".
	uint32_t *m_indices;
	uint32_t m_numItems; ///< Number of bounding boxes added.
	uint32_t m_nodeSize;
	uint32_t m_numNodes;
	uint32_t m_levelBounds[32]{};
	uint32_t m_numBounds;
	uint32_t m_pos;
	float m_minx, m_maxx, m_miny, m_maxy; ///< Bounding box of all the bboxes added.
};

//! @}
} // namespace candybox

namespace candybox {

namespace detail {

struct dummy_iterator
{
	dummy_iterator &operator++() { return *this; }
	dummy_iterator &operator*() { return *this; }
	template <typename T> dummy_iterator &operator=(const T &) { return *this; }
};

template <class AllocatorClass>
inline typename AllocatorClass::value_type *
allocate(AllocatorClass &allocator, int level)
{
	typedef typename AllocatorClass::value_type Node;
	Node *p = allocator.allocate(1);
	// not using construct as deprecated from C++17
	new (p) Node(level);
	return p;
}

template <class AllocatorClass>
inline void
deallocate(AllocatorClass &allocator, typename AllocatorClass::value_type *node)
{
	allocator.deallocate(node, 1);
}

using std::allocator;
} // namespace detail

// Index-able getter for bbox limits for a given type
template <typename T, typename ValueType> struct Indexable
{
	const T &min(const ValueType &value) const { return value.l; }
	const T &max(const ValueType &value) const { return value.u; }
};



namespace detail {
struct intersects_tag;
struct contains_tag;
struct within_tag;

template <typename T, int Dimension, typename OperationTag> struct CheckPredicateHelper;

template <typename T, int Dimension> struct CheckPredicateHelper<T, Dimension, intersects_tag>
{
	typedef TBox<T, Dimension> box_t;

	inline bool operator()(const box_t &predicateBBox, const box_t &valueBBox) const
	{
		return predicateBBox.overlaps(valueBBox);
	}
};

template <typename T, int Dimension> struct CheckPredicateHelper<T, Dimension, contains_tag>
{
	typedef TBox<T, Dimension> box_t;

	inline bool operator()(const box_t &predicateBBox, const box_t &valueBBox) const
	{
		return predicateBBox.contains(valueBBox);
	}
};

template <typename T, int Dimension> struct CheckPredicateHelper<T, Dimension, within_tag>
{
	typedef TBox<T, Dimension> box_t;

	inline bool operator()(const box_t &predicateBBox, const box_t &valueBBox) const
	{
		return predicateBBox.contains(valueBBox.min);
	}
};

template <typename T, int Dimension, typename OperationTag>
bool
checkPredicate(const TBox<T, Dimension> &predicateBBox, const TBox<T, Dimension> &valueBBox)
{
	return CheckPredicateHelper<T, Dimension, OperationTag>()(predicateBBox, valueBBox);
}
}; // namespace detail

template <typename T, int Dimension, typename OperationTag> struct SpatialPredicate
{
	typedef TBox<T, Dimension> box_t;
	typedef OperationTag op_t;

	explicit SpatialPredicate(const box_t &bbox) : bbox(bbox) { }
	inline bool operator()(const box_t &valueBBox) const
	{
		return detail::checkPredicate<T, Dimension, OperationTag>(bbox, valueBBox);
	}

	box_t bbox;
};

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::intersects_tag>
intersects(const TBox<T, Dimension> &bbox)
{
	return SpatialPredicate<T, Dimension, detail::intersects_tag>(bbox);
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::intersects_tag>
intersects(const T min[Dimension], const T max[Dimension])
{
	typedef SpatialPredicate<T, Dimension, detail::intersects_tag> predicate_t;
	return predicate_t(typename predicate_t::box_t(min, max));
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::intersects_tag>
intersects(const glm::vec<Dimension, T> &min, const glm::vec<Dimension, T> &max)
{
	typedef SpatialPredicate<T, Dimension, detail::intersects_tag> predicate_t;
	return predicate_t(typename predicate_t::box_t(min, max));
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::contains_tag>
contains(const TBox<T, Dimension> &bbox)
{
	return SpatialPredicate<T, Dimension, detail::contains_tag>(bbox);
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::contains_tag>
contains(const T min[Dimension], const T max[Dimension])
{
	typedef SpatialPredicate<T, Dimension, detail::contains_tag> predicate_t;
	return predicate_t(typename predicate_t::box_t(min, max));
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::contains_tag>
contains(const glm::vec<Dimension, T> &min, const glm::vec<Dimension, T> &max)
{
	typedef SpatialPredicate<T, Dimension, detail::contains_tag> predicate_t;
	return predicate_t(typename predicate_t::box_t(min, max));
}

/// \note To be used with points, as it only check the min point of the bounding boxes of the
/// 	leaves.
template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::within_tag>
within(const TBox<T, Dimension> &bbox)
{
	return SpatialPredicate<T, Dimension, detail::within_tag>(bbox);
}

template <int Dimension, typename T>
SpatialPredicate<T, Dimension, detail::within_tag>
within(const T min[Dimension], const T max[Dimension])
{
	typedef SpatialPredicate<T, Dimension, detail::within_tag> predicate_t;
	return predicate_t(typename predicate_t::box_t(min, max));
}





namespace detail {

template <class NodeClass, typename CountType> class Stack
{
public:
	enum StatusType
	{
		eNormal = 0,
		eBranchTraversed,
		eNextBranch
	};

protected:
	struct StackElement
	{
		NodeClass *node;
		CountType branchIndex;
		StatusType status;

		StackElement() : status(eNormal) { }
	};

protected:
	Stack() : m_tos(0) { }

	void push(NodeClass *node, CountType branchIndex, StatusType status = eNormal)
	{
		assert(node);

		StackElement &el = m_stack[m_tos++];
		el.node = node;
		el.branchIndex = branchIndex;
		el.status = status;

		assert(m_tos <= kMaxStackSize);
	}
	StackElement &pop()
	{
		assert(m_tos > 0);

		StackElement &el = m_stack[--m_tos];
		return el;
	}

protected:
	//  Max stack size. Allows almost n^16 where n is number of branches in node
	static const int kMaxStackSize = 16;
	StackElement m_stack[kMaxStackSize]; ///< Stack as we are doing iteration
	    /// instead of recursion
	int m_tos; ///< Top Of Stack index
}; // class stack_iterator

/// May be data or may be another subtree
/// The parents level determines this.
/// If the parents level is 0, then this is data
template <typename ValueType, class BBoxClass, class NodeClass> struct Branch
{
	ValueType value;
	BBoxClass bbox;
	NodeClass *child;

#ifndef NDEBUG
	Branch() : child(NULL) { }
#endif
}; // Branch

template <typename ValueType, class BBoxClass, int max_child_items> struct Node
{
	typedef Branch<ValueType, BBoxClass, Node> branch_type;
	typedef uint32_t count_type;
	typedef BBoxClass box_type;

	count_type count; ///< Number of branches in the node
	int32_t level; ///< Leaf is zero, others positive
	ValueType values[max_child_items];
	BBoxClass bboxes[max_child_items];
	Node *children[max_child_items];

#ifdef TREE_DEBUG_TAG
	tn::string debugTags[max_child_items];
#endif

	Node() : level(0) { }

	Node(int level) : count(0), level(level) { }

	// Not a leaf, but a internal/branch node
	bool isBranch() const { return (level > 0); }

	bool isLeaf() const { return (level == 0); }

	// Find the smallest rectangle that includes all rectangles in branches of a
	// node.
	BBoxClass cover() const
	{
		BBoxClass bbox = bboxes[0];
		for (count_type index = 1; index < count; ++index) { bbox.extend(bboxes[index]); }

		return bbox;
	}
	bool addBranch(const branch_type &branch)
	{
		if (count >= max_child_items) // Split is necessary
			return false;

		values[count] = branch.value;
		children[count] = branch.child;
		bboxes[count++] = branch.bbox;
		return true;
	}

	// Disconnect a dependent node.
	// Caller must return (or stop using iteration index) after this as count has
	// changed
	void disconnectBranch(count_type index)
	{
		assert(index >= 0 && index < max_child_items);
		assert(count > 0);

		// Remove element by swapping with the last element to prevent gaps in array
		values[index] = values[--count];
		children[index] = children[count];
		bboxes[index] = bboxes[count];
	}
}; // Node


struct AlwayTruePredicate
{
	template <typename T> inline bool operator()(const T &) const { return true; }
};

struct DummyInsertPredicate
{
};

template <typename Predicate, class NodeClass> struct CheckInsertPredicateHelper
{
	inline bool operator()(const Predicate &predicate, const NodeClass &node) const
	{
		for (typename NodeClass::count_type index = 0; index < node.count; ++index)
		{
			if (!predicate(node.bboxes[index])) return false;
		}
		return true;
	}
};

template <class NodeClass> struct CheckInsertPredicateHelper<DummyInsertPredicate, NodeClass>
{
	inline bool
	operator()(const DummyInsertPredicate & /*predicate*/, const NodeClass & /*node*/) const
	{
		return true;
	};
};

template <typename Predicate, class NodeClass>
inline bool
checkInsertPredicate(const Predicate &predicate, const NodeClass &node)
{
	return CheckInsertPredicateHelper<Predicate, NodeClass>()(predicate, node);
}

template <class RTreeClass>
typename RTreeClass::node_ptr_type &
getRootNode(RTreeClass &tree)
{
	return tree.m_root;
}
} // namespace detail




namespace detail {
template <class NodeClass> class QuadTreeStack
{
protected:
	struct StackElement
	{
		NodeClass *node;
		size_t childIndex;
		int objectIndex;
	};

protected:
	QuadTreeStack() : m_tos(0) { }

	void push(NodeClass *node, size_t childIndex, int objectIndex)
	{
		assert(node);

		StackElement &el = m_stack[m_tos++];
		el.node = node;
		el.childIndex = childIndex;
		el.objectIndex = objectIndex;

		assert(m_tos <= kMaxStackchildIndexze);
	}

	void push(NodeClass *node, int objectIndex)
	{
		assert(node);

		StackElement &el = m_stack[m_tos++];
		el.node = node;
		el.objectIndex = objectIndex;

		assert(m_tos <= kMaxStackchildIndexze);
	}

	StackElement &pop()
	{
		assert(m_tos > 0);

		StackElement &el = m_stack[--m_tos];
		return el;
	}

protected:
	//  Max stack size. Allows almost n^16 where n is number of branches in node
	static const int kMaxStackchildIndexze = 16;
	StackElement m_stack[kMaxStackchildIndexze]; ///< Stack as we are doing
	/// iteration instead of recursion
	int m_tos; ///< Top Of Stack index
};

template <typename ValueType, class BBoxClass, class NodeClass> struct QuadTreeObject
{
	ValueType value;
	BBoxClass box;

	QuadTreeObject() { }

	template <typename indexable_getter>
	QuadTreeObject(ValueType value, const indexable_getter &indexable)
	    : value(value), box(indexable.min(value), indexable.max(value))
	{
	}

	inline bool operator==(const QuadTreeObject &other) const { return value == other.value; }
};

template <class T, class ValueType, int max_child_items> struct QuadTreeNode
{
	typedef TBox<T, 2> bbox_type;
	typedef typename bbox_type::tvec bbox_tvec_type;
	typedef QuadTreeObject<ValueType, bbox_type, QuadTreeNode> object_type;
	typedef std::vector<object_type> ObjectList;

	const int level;
	ValueType value;
	ObjectList objects;
	bbox_type box;
	QuadTreeNode *children[4];

	QuadTreeNode(int level);

	template <typename custom_allocator>
	void copy(const QuadTreeNode &src, custom_allocator &allocator);
	template <typename custom_allocator>
	bool insert(const object_type &obj, int &levels, custom_allocator &allocator);
	template <typename custom_allocator>
	bool remove(const object_type &obj, custom_allocator &allocator);
	template <typename Predicate, typename OutIter>
	size_t query(const Predicate &predicate, float factor, OutIter out_it) const;
	template <typename Predicate, typename OutIter>
	size_t queryHierachical(const Predicate &predicate, float factor, OutIter out_it) const;
	template <typename custom_allocator> void clear(custom_allocator &allocator);
	void translate(const T point[2]);
	size_t count() const;
	bool isEmpty() const;

	bool isBranch() const { return !isLeaf(); }

	bool isLeaf() const { return children[0] == NULL; }

	size_t objectCount() const { return objects.size(); }

	ValueType &objectValue(size_t objectIndex) { return objects[objectIndex].value; }

	const ValueType &objectValue(size_t objectIndex) const
	{
		return objects[objectIndex].value;
	}

	ValueType &objectValue(int objectIndex)
	{
		assert(objectIndex >= 0);
		return objects[(size_t)objectIndex].value;
	}

	const ValueType &objectValue(int objectIndex) const
	{
		assert(objectIndex >= 0);
		return objects[(size_t)objectIndex].value;
	}

	void updateCount()
	{
		m_count = count();
		m_invCount = 1.f / m_count;
	}

private:
	size_t m_count;
	float m_invCount;

	template <typename custom_allocator> void subdivide(custom_allocator &allocator);
	void addObject(const object_type &obj);
	bool removeObject(const object_type &obj);
	template <typename custom_allocator>
	bool addObjectsToChildren(custom_allocator &allocator);

	template <typename OutIter> void insertAll(size_t &foundCount, OutIter out_it) const;
};

#define TREE_TEMPLATE template <class T, class ValueType, int max_child_items>
#define TREE_QUAL     QuadTreeNode<T, ValueType, max_child_items>

TREE_TEMPLATE
TREE_QUAL::QuadTreeNode(int level)
    : level(level),
      children()
#ifndef NDEBUG
      ,
      m_count(0)
#endif
{
}

TREE_TEMPLATE
template <typename custom_allocator>
void
TREE_QUAL::copy(const QuadTreeNode &src, custom_allocator &allocator)
{
	assert(m_count == 0);
	value = src.value;
	objects = src.objects;
	box = src.box;
	m_count = src.m_count;
	m_invCount = src.m_invCount;

	if (src.isBranch())
	{
		for (int i = 0; i < 4; ++i)
		{
			const QuadTreeNode *srcCurrent = src.children[i];
			QuadTreeNode
			    *dstCurrent = children[i] = detail::allocate(allocator, srcCurrent->level);
			dstCurrent->copy(*srcCurrent, allocator);
		}
	}
}

TREE_TEMPLATE
template <typename custom_allocator>
bool
TREE_QUAL::insert(const object_type &obj, int &levels, custom_allocator &allocator)
{
	if (!this->box.contains(obj.box))
		// this object doesn't fit in this quadtree
		return false;

	if (isLeaf()) // No subdivision yet
	{
		if (objects.size() < max_child_items + 1)
		{
			size_t count = objects.size();
			addObject(obj);
			return objects.size() > count;
		}

		// subdivide node
		subdivide(allocator);
		if (addObjectsToChildren(allocator)) { levels = std::max(levels, level + 1); }
		else
		{
			// could not insert anything in any of the sub-trees
			for (int i = 0; i < 4; ++i)
			{
				detail::deallocate(allocator, children[i]);
				children[i] = NULL;
			}

			addObject(obj);
			return true;
		}
	}

	// try to add to children
	for (int i = 0; i < 4; ++i)
	{
		assert(children[i]);
		if (children[i]->insert(obj, levels, allocator))
		{
			updateCount();
			return true;
		}
	}
	addObject(obj);
	return true;
}

TREE_TEMPLATE
template <typename custom_allocator>
bool
TREE_QUAL::remove(const object_type &obj, custom_allocator &allocator)
{
	if (!this->box.contains(obj.box))
		// this object doesn't fit in this quadtree
		return false;

	if (isLeaf()) return removeObject(obj);

	// try to remove from one of the children
	for (int i = 0; i < 4; ++i)
	{
		assert(children[i]);
		if (children[i]->remove(obj, allocator))
		{
			updateCount();
			return true;
		}
	}
	return removeObject(obj);
}

TREE_TEMPLATE
bool
TREE_QUAL::removeObject(const object_type &obj)
{
	const auto found = std::find(objects.begin(), objects.end(), obj);
	if (found != objects.end())
	{
		objects.erase(found);
		updateCount();
		return true;
	}
	return false;
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
size_t
TREE_QUAL::query(const Predicate &predicate, float containmentFactor, OutIter out_it) const
{
	assert(m_count == count());

	size_t foundCount = 0;

	// go further into the tree
	for (typename ObjectList::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		if (predicate(it->box))
		{
			*out_it = it->value;
			++out_it;
			++foundCount;
		}
	}

	if (isLeaf())
	{
		// reached leaves
		return foundCount;
	}

	for (int i = 0; i < 4; i++)
	{
		assert(children[i]);

		QuadTreeNode &node = *children[i];
		// Break if we know that the zone is fully contained by a region
		if (predicate.bbox.overlaps(node.box))
		{
			foundCount += node.query(predicate, containmentFactor, out_it);
			if (node.box.contains(predicate.bbox)) { break; }
		}
	}

	return foundCount;
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
size_t
TREE_QUAL::
    queryHierachical(const Predicate &predicate, float containmentFactor, OutIter out_it) const
{
	assert(m_count == count());

	size_t foundCount = 0;
	if (predicate.bbox.contains(this->box) && !isEmpty())
	{
		// node is fully contained by the query
		*out_it = value;
		++out_it;
		foundCount += m_count;

		return foundCount;
	}

	const OutIter start = out_it;
	// go further into the tree
	for (typename ObjectList::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		if (predicate(it->box))
		{
			*out_it = it->value;
			++out_it;
			++foundCount;
		}
	}

	if (isLeaf())
	{
		if (foundCount)
		{
			const float factor = foundCount * m_invCount;
			if (factor > containmentFactor)
			{
				out_it = start;
				// node is fully contained by the query
				*out_it = value;
				++out_it;
				foundCount = m_count;
			}
		}

		// reached leaves
		return foundCount;
	}

	for (int i = 0; i < 4; i++)
	{
		assert(children[i]);

		QuadTreeNode &node = *children[i];
		// Break if we know that the zone is fully contained by a region
		if (predicate.bbox.overlaps(node.box))
		{
			foundCount += node.query(predicate, containmentFactor, out_it);
			if (node.box.contains(predicate.bbox)) { break; }
		}
	}

	if (foundCount)
	{
		const float factor = foundCount * m_invCount;
		if (factor > containmentFactor)
		{
			out_it = start;
			// node is fully contained by the query
			*out_it = value;
			++out_it;
			foundCount = m_count;
		}
	}

	return foundCount;
}

TREE_TEMPLATE
void
TREE_QUAL::translate(const T point[2])
{
	for (typename ObjectList::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		it->box.translate(point);
	}
	box.translate(point);

	if (isBranch())
	{
		for (int i = 0; i < 4; ++i)
		{
			if (children[i]) { children[i]->translate(point); }
		}
	}
}

TREE_TEMPLATE
size_t
TREE_QUAL::count() const
{
	size_t count = objectCount();
	if (isBranch())
	{
		for (int i = 0; i < 4; ++i) { count += children[i]->count(); }
	}
	return count;
}

TREE_TEMPLATE
bool
TREE_QUAL::isEmpty() const
{
	if (!objects.empty()) return false;

	if (isBranch())
	{
		for (int i = 0; i < 4; ++i)
		{
			if (!children[i]->isEmpty()) return false;
		}
	}
	return true;
}

TREE_TEMPLATE
template <typename custom_allocator>
void
TREE_QUAL::clear(custom_allocator &allocator)
{
	if (isLeaf()) return;

	for (int i = 0; i < 4; ++i)
	{
		children[i]->clear(allocator);
		detail::deallocate(allocator, children[i]);
	}
}

TREE_TEMPLATE
template <typename custom_allocator>
void
TREE_QUAL::subdivide(custom_allocator &allocator)
{
	for (int i = 0; i < 4; ++i)
	{
		assert(children[i] == NULL);
		children[i] = detail::allocate(allocator, level + 1);
		QuadTreeNode &node = *children[i];
		node.box = box.quad2d(static_cast<RegionType>(i));
	}
}

TREE_TEMPLATE
void
TREE_QUAL::addObject(const object_type &obj)
{
	auto found = std::find(objects.begin(), objects.end(), obj);
	if (found == objects.end())
	{
		objects.push_back(obj);
		updateCount();
	}
}

TREE_TEMPLATE
template <typename custom_allocator>
bool
TREE_QUAL::addObjectsToChildren(custom_allocator &allocator)
{
	int dummy = 0;
	const size_t prevSize = objects.size();
	for (typename ObjectList::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		for (auto &i : children)
		{
			if (i->insert(*it, dummy, allocator))
			{
				it = objects.erase(it);
				if (it == objects.end())
				{
					objects.shrink_to_fit();
					for (auto &j : children)
					{
						j->updateCount();
						assert(j->m_count == j->count());
					}

					return true;
				}
				break;
			}
		}
	}

	objects.shrink_to_fit();
	for (int i = 0; i < 4; ++i)
	{
		children[i]->updateCount();
		assert(children[i]->m_count == children[i]->count());
	}

	// return if we could insert anything in any of the sub-trees
	return prevSize != objects.size();
}

TREE_TEMPLATE
template <typename OutIter>
void
TREE_QUAL::insertAll(size_t &foundCount, OutIter out_it) const
{
	for (typename ObjectList::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		// add crossing/overlapping results
		*out_it = it->value;
		++out_it;
	}
	foundCount += objects.size();

	if (isBranch())
	{
		for (int i = 0; i < 4; ++i) { children[i]->insertAll(foundCount, out_it); }
	}
}
} // namespace detail

#undef TREE_TEMPLATE
#undef TREE_QUAL
} // namespace candybox

namespace candybox {

/**
	 \class QuadTree
	 \brief Implementation of a Quadtree for 2D space.

     It has the following properties:
	 - hierarchical, you can add values to the internal nodes
	 - custom indexable getter similar to boost's
	 - custom allocation for internal nodes

	 @tparam T                  type of the space(eg. int, float, etc.)
	 @tparam ValueType		    type of value stored in the tree's nodes, requires a equality operator.
	 @tparam max_child_items    maximum number of items per node
	 @tparam indexable_getter   the indexable getter, i.e. the getter for the
	 bounding box of a value
	 @tparam custom_allocator   the allocator class items exceeds the given factor
	 then it'll use only the parent value

	 @note It's recommended that ValueType should be a fast to copy object, eg: int,
	 id, obj*, etc.
	*/
template <
    class T,
    class ValueType,
    int max_child_items = 16,
    typename indexable_getter = Indexable<
        typename detail::QuadTreeNode<T, ValueType, max_child_items>::bbox_tvec_type,
        ValueType>,
    typename custom_allocator =
        detail::allocator<detail::QuadTreeNode<T, ValueType, max_child_items>>>
class QuadTree
{
public:
	typedef TBox<T, 2> bbox_type;
	typedef custom_allocator allocator_type;

	static const size_t max_items = max_child_items;

private:
	typedef detail::QuadTreeNode<T, ValueType, max_child_items> node_type;
	typedef detail::QuadTreeObject<ValueType, bbox_type, node_type> object_type;
	typedef node_type *node_ptr_type;

public:
	class base_iterator : public detail::QuadTreeStack<node_type>
	{
	public:
		/// Returns the depth level of the current branch.
		int level() const;

		/// Is iterator still valid.
		bool valid() const;
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		ValueType &operator*();
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		const ValueType &operator*() const;
		const bbox_type &bbox() const;

	protected:
		base_iterator(const int maxLevel);

		const int m_maxLevel;

		typedef detail::QuadTreeStack<node_type> base_type;
		using base_type::m_stack;
		using base_type::m_tos;
	};

	/**
			 @brief Iterator to traverese the items of a node of the tree.
			 */
	struct node_iterator
	{
		node_iterator();

		/// Returns the depth level of the current branch.
		int level() const;
		/// Is iterator still valid.
		bool valid() const;
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		ValueType &operator*();
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		const ValueType &operator*() const;
		const bbox_type &bbox() const;
		/// Advances to the next item.
		void next();

	private:
		node_iterator(node_ptr_type node, int maxLevel);
		node_iterator(node_ptr_type node, size_t childIndex, int maxLevel);

		const int m_maxLevel;
		size_t m_childIndex;
		int m_objectIndex;
		node_ptr_type m_node;

		friend class QuadTree;
	};

	/**
			@brief Iterator to traverse the whole tree similar to depth first search. It
			reaches the lowest level
			(i.e. leaves) and afterwards visits the higher levels until it reaches the
			root(highest) level.
			*/
	class depth_iterator : public base_iterator
	{
		typedef base_iterator base_type;
		using base_type::m_stack;
		using base_type::m_tos;
		using base_type::push;
		using base_type::pop;

	public:
		// Returns a lower level, candybox. child, node of the current branch.
		node_iterator child();
		node_iterator current();

		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		ValueType &operator*();
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		const ValueType &operator*() const;
		/// Advances to the next item.
		void next();

	private:
		depth_iterator(const int maxLevel);
		friend class QuadTree;
	};

	/**
			 @brief Iterator to traverse only the leaves of the tree similar, left to
			 right order.
			 */
	class leaf_iterator : public base_iterator
	{
		typedef base_iterator base_type;
		using base_type::m_stack;
		using base_type::m_tos;
		using base_type::push;
		using base_type::pop;

	public:
		/// Advances to the next item.
		void next();

	private:
		leaf_iterator(const int maxLevel);
		friend class QuadTree;
	};

	QuadTree(
	    const glm::vec<2, T> &min,
	    const glm::vec<2, T> &max, //
	    indexable_getter indexable = indexable_getter(), //
	    const allocator_type &allocator = allocator_type());
	template <typename Iter>
	QuadTree(
	    const glm::vec<2, T> &min,
	    const glm::vec<2, T> &max, //
	    Iter first,
	    Iter last, //
	    indexable_getter indexable = indexable_getter(), //
	    const allocator_type &allocator = allocator_type());
	QuadTree(const QuadTree &src);
#ifdef SPATIAL_TREE_USE_CPP11
	QuadTree(QuadTree &&src);
#endif
	~QuadTree();

	QuadTree &operator=(const QuadTree &rhs);
#ifdef SPATIAL_TREE_USE_CPP11
	QuadTree &operator=(QuadTree &&rhs) noexcept;
#endif

	void swap(QuadTree &other);

	///@note The tree must be empty before doing this.
	void setBox(const glm::vec<2, T> &min, const glm::vec<2, T> &max);
	template <typename Iter> void insert(Iter first, Iter last);
	void insert(const ValueType &value);
	bool remove(const ValueType &value);

	/// Translates the internal boxes with the given offset point.
	void translate(const glm::vec<2, T> &point);

	/// Special query to find all within search rectangle using the hierarchical
	/// order.
	/// @see SpatialPredicate for available predicates.
	/// \return Returns the number of entries found.
	template <typename Predicate, typename OutIter>
	size_t hierachical_query(const Predicate &predicate, OutIter out_it) const;
	///@param containment_factor the containment factor in percentage used for
	/// query, if the total number of visible.
	///@note Only used for hierarchical query.
	void setContainmentFactor(int factor);

	/// @see SpatialPredicate for available predicates.
	template <typename Predicate> bool query(const Predicate &predicate) const;
	template <typename Predicate, typename OutIter>
	size_t query(const Predicate &predicate, OutIter out_it) const;

	/// Remove all entries from tree
	void clear(bool recursiveCleanup = true);
	/// Count the data elements in this container.
	size_t count() const;
	/// Returns the number of levels(height) of the tree.
	int levels() const;
	/// Returns the bbox of the root node.
	bbox_type bbox() const;
	/// Returns the custom allocator
	allocator_type &allocator();
	const allocator_type &allocator() const;

	node_iterator root();
	depth_iterator dbegin();
	leaf_iterator lbegin();

private:
	indexable_getter m_indexable;
	mutable allocator_type m_allocator;

	size_t m_count;
	int m_levels;
	float m_factor;
	node_ptr_type m_root;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TREE_TEMPLATE                                                                         \
	template <                                                                                \
	    class T, class ValueType, int max_child_items, typename indexable_getter,             \
	    typename custom_allocator>

#define TREE_QUAL QuadTree<T, ValueType, max_child_items, indexable_getter, custom_allocator>

TREE_TEMPLATE
TREE_QUAL::QuadTree(
    const glm::vec<2, T> &min,
    const glm::vec<2, T> &max, //
    indexable_getter indexable /*= indexable_getter()*/,
    const allocator_type &allocator /*= allocator_type()*/)
    : m_indexable(indexable),
      m_allocator(allocator),
      m_count(0),
      m_levels(0),
      m_factor(60),
      m_root(NULL)
{
	static_assert(max_child_items > 1, "Invalid child size!");

	m_root = detail::allocate(m_allocator, 0);
	m_root->box.set(min, max);
}

TREE_TEMPLATE
template <typename Iter>
TREE_QUAL::QuadTree(
    const glm::vec<2, T> &min,
    const glm::vec<2, T> &max, //
    Iter first,
    Iter last, //
    indexable_getter indexable /*= indexable_getter()*/,
    const allocator_type &allocator /*= allocator_type()*/)
    : m_indexable(indexable), m_allocator(allocator), m_count(0), m_levels(0), m_factor(60)
{
	static_assert(max_child_items > 1, "Invalid child size!");

	m_root = detail::allocate(m_allocator, 0);
	m_root->box.set(min, max);

	insert(first, last);
}

TREE_TEMPLATE
TREE_QUAL::QuadTree(const QuadTree &src)
    : m_indexable(src.m_indexable),
      m_allocator(src.m_allocator),
      m_count(src.m_count),
      m_levels(src.m_levels),
      m_factor(src.m_factor),
      m_root(detail::allocate(m_allocator, 0))
{
	m_root->copy(*src.m_root, m_allocator);
}

#ifdef SPATIAL_TREE_USE_CPP11
TREE_TEMPLATE
TREE_QUAL::QuadTree(QuadTree &&src) : m_root(NULL) { swap(src); }
#endif

TREE_TEMPLATE
TREE_QUAL::~QuadTree()
{
	if (m_root) m_root->clear(m_allocator);
	detail::deallocate(m_allocator, m_root);
}

TREE_TEMPLATE
TREE_QUAL &
TREE_QUAL::operator=(const QuadTree &rhs)
{
	if (&rhs != this)
	{
		if (m_count > 0) clear(true);

		m_count = rhs.m_count;
		m_levels = rhs.m_levels;
		m_factor = rhs.m_factor;
		m_allocator = rhs.m_allocator;
		m_indexable = rhs.m_indexable;
		m_root->copy(*rhs.m_root, m_allocator);
	}
	return *this;
}

#ifdef SPATIAL_TREE_USE_CPP11
TREE_TEMPLATE
TREE_QUAL &
TREE_QUAL::operator=(QuadTree &&rhs) noexcept
{
	assert(this != &rhs);

	if (m_count > 0) clear(true);
	swap(rhs);

	return *this;
}
#endif

TREE_TEMPLATE
void
TREE_QUAL::swap(QuadTree &other)
{
	std::swap(m_root, other.m_root);
	std::swap(m_count, other.m_count);
	std::swap(m_factor, other.m_factor);
	std::swap(m_levels, other.m_levels);
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_indexable, other.m_indexable);
}

TREE_TEMPLATE
void
TREE_QUAL::setBox(const glm::vec<2, T> &min, const glm::vec<2, T> &max)
{
	assert(m_root);
	assert(m_count == 0);
	m_root->box.set(min, max);
}

TREE_TEMPLATE
template <typename Iter>
void
TREE_QUAL::insert(Iter first, Iter last)
{
	assert(m_root);

	bool success = true;
	for (Iter it = first; it != last; ++it)
	{
		const object_type obj(*it, m_indexable);
		assert(m_root->box.contains(obj.box));
		success &= m_root->insert(obj, m_levels, m_allocator);
		++m_count;
	}
	(void)(success);
	assert(success);
}

TREE_TEMPLATE
void
TREE_QUAL::insert(const ValueType &value)
{
	assert(m_root);

	const object_type obj(value, m_indexable);
	assert(m_root->box.contains(obj.box));

	bool success = m_root->insert(obj, m_levels, m_allocator);
	if (success) ++m_count;
	assert(m_count == m_root->count());
}

TREE_TEMPLATE
bool
TREE_QUAL::remove(const ValueType &value)
{
	assert(m_root);

	const object_type obj(value, m_indexable);
	assert(m_root->box.contains(obj.box));

	const bool success = m_root->remove(obj, m_allocator);
	if (success) --m_count;

	return success;
}

TREE_TEMPLATE
void
TREE_QUAL::translate(const glm::vec<2, T> &point)
{
	m_root->translate(point);
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
size_t
TREE_QUAL::hierachical_query(const Predicate &predicate, OutIter out_it) const
{
	assert(m_root);
	return m_root->queryHierachical(predicate, m_factor, out_it);
}

TREE_TEMPLATE
void
TREE_QUAL::setContainmentFactor(int factor)
{
	m_factor = factor / 100.f;
}

TREE_TEMPLATE
template <typename Predicate>
bool
TREE_QUAL::query(const Predicate &predicate) const
{
	return query(predicate, detail::dummy_iterator()) > 0;
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
size_t
TREE_QUAL::query(const Predicate &predicate, OutIter out_it) const
{
	assert(m_root);
	return m_root->query(predicate, m_factor, out_it);
}

TREE_TEMPLATE
void
TREE_QUAL::clear(bool recursiveCleanup /*= true*/)
{
	// Delete all existing nodes
	if (recursiveCleanup)
	{
		if (m_root) m_root->clear(m_allocator);
	}
	assert(m_root != nullptr);
	const bbox_type box = m_root->box;
	detail::deallocate(m_allocator, m_root);

	m_root = detail::allocate(m_allocator, 0);
	m_root->box = box;
	m_levels = 0;
	m_count = 0;
}

TREE_TEMPLATE
size_t
TREE_QUAL::count() const
{
	assert(m_root);
	assert(m_root->count() == m_count);
	return m_count;
}

TREE_TEMPLATE
int
TREE_QUAL::levels() const
{
	assert(m_root);
	return m_levels;
}

TREE_TEMPLATE
typename TREE_QUAL::bbox_type
TREE_QUAL::bbox() const
{
	assert(m_root);
	return m_root->box;
}

TREE_TEMPLATE
typename TREE_QUAL::allocator_type &
TREE_QUAL::allocator()
{
	return m_allocator;
}

TREE_TEMPLATE
const typename TREE_QUAL::allocator_type &
TREE_QUAL::allocator() const
{
	return m_allocator;
}

TREE_TEMPLATE
typename TREE_QUAL::node_iterator
TREE_QUAL::root()
{
	assert(m_root);
	return node_iterator(m_root, m_levels);
}

TREE_TEMPLATE
typename TREE_QUAL::depth_iterator
TREE_QUAL::dbegin()
{
	assert(m_root);
	depth_iterator it(m_levels);

	it.push(m_root, 0, -1);
	for (;;)
	{
		if (it.m_tos <= 0) break;

		// Copy stack top cause it may change as we use it
		const typename depth_iterator::StackElement curTos = it.pop();

		if (curTos.node->isLeaf())
		{
			if (curTos.node->objectCount())
			{
				// Visit leaf once
				it.push(curTos.node, 0);
				break;
			}
			// fallback to previous
		}
		else
		{
			// Keep walking through children while we can
			if (curTos.childIndex + 1 < 4)
			{
				// Push sibling on for future tree walk
				// This is the 'fall back' node when we finish with the current level
				it.push(curTos.node, curTos.childIndex + 1, 0);
			}
			else
			{
				// Push current node on for future objects walk
				it.push(curTos.node, 4, 0);
			}

			// Since cur node is not a leaf, push first of next level to get deeper
			// into the tree
			node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
			if (nextLevelnode) { it.push(nextLevelnode, 0, 0); }
		}
	}
	return it;
}

TREE_TEMPLATE
typename TREE_QUAL::leaf_iterator
TREE_QUAL::lbegin()
{
	assert(m_root);
	leaf_iterator it(m_levels);

	it.push(m_root, 0, -1);
	for (;;)
	{
		if (it.m_tos <= 0) break;

		// Copy stack top cause it may change as we use it
		const typename leaf_iterator::StackElement curTos = it.pop();

		if (curTos.node->isLeaf())
		{
			if (curTos.node->objectCount())
			{
				it.push(curTos.node, 0);
				break;
			}
			// Empty node, fallback to previous
		}
		else
		{
			// Keep walking through children while we can
			if (curTos.childIndex + 1 < 4)
			{
				// Push sibling on for future tree walk
				// This is the 'fall back' node when we finish with the current level
				it.push(curTos.node, curTos.childIndex + 1, -1);
			}
			else
			{
				// Push current node on for future objects walk
				it.push(curTos.node, 4, -1);
			}

			// Since cur node is not a leaf, push first of next level to get deeper
			// into the tree
			node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
			if (nextLevelnode) { it.push(nextLevelnode, 0, -1); }
		}
	}
	return it;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
TREE_QUAL::base_iterator::base_iterator(const int maxLevel) : m_maxLevel(maxLevel) { }

TREE_TEMPLATE
int
TREE_QUAL::base_iterator::level() const
{
	assert(m_tos > 0);
	const typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return m_maxLevel - curTos.node->level;
}

TREE_TEMPLATE
bool
TREE_QUAL::base_iterator::valid() const
{
	return (m_tos > 0);
}

TREE_TEMPLATE
ValueType &
TREE_QUAL::base_iterator::operator*()
{
	assert(valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->objectValue(curTos.objectIndex);
}

TREE_TEMPLATE
const ValueType &
TREE_QUAL::base_iterator::operator*() const
{
	assert(valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->objectValue(curTos.objectIndex);
}

TREE_TEMPLATE
const typename TREE_QUAL::bbox_type &
TREE_QUAL::base_iterator::bbox() const
{
	assert(valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->children[curTos.childIndex]->bbox;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
TREE_QUAL::node_iterator::node_iterator()
    : m_maxLevel(0), m_childIndex(0), m_objectIndex(0), m_node(NULL)
{
}

TREE_TEMPLATE
TREE_QUAL::node_iterator::node_iterator(node_ptr_type node, size_t childIndex, int maxLevel)
    : m_maxLevel(maxLevel),
      m_childIndex(childIndex),
      m_objectIndex(node->isLeaf() ? 0 : -1),
      m_node(node)
{
	assert(node);

	while (m_childIndex < 4 && m_node->children[m_childIndex]->isEmpty()) ++m_childIndex;
	if (m_childIndex >= 4) m_objectIndex = 0;
}

TREE_TEMPLATE
TREE_QUAL::node_iterator::node_iterator(node_ptr_type node, int maxLevel)
    : m_maxLevel(maxLevel),
      m_childIndex(node->isLeaf() ? 4 : 0),
      m_objectIndex(node->isLeaf() ? 0 : -1),
      m_node(node)
{
	assert(node);

	while (m_childIndex < 4 && m_node->children[m_childIndex]->isEmpty()) ++m_childIndex;
	if (m_childIndex >= 4) m_objectIndex = 0;
}

TREE_TEMPLATE
int
TREE_QUAL::node_iterator::level() const
{
	assert(m_node);
	return m_maxLevel - m_node->level;
}

TREE_TEMPLATE
bool
TREE_QUAL::node_iterator::valid() const
{
	assert(m_node);
	return (m_objectIndex < (int)m_node->objectCount());
}

TREE_TEMPLATE
ValueType &
TREE_QUAL::node_iterator::operator*()
{
	assert(valid());
	if (m_childIndex < 4)
	{
		assert(m_node->isBranch());
		return m_node->children[m_childIndex]->value;
	}
	else return m_node->objectValue(m_objectIndex);
}

TREE_TEMPLATE
const ValueType &
TREE_QUAL::node_iterator::operator*() const
{
	assert(valid());
	if (m_childIndex < 4)
	{
		assert(m_node->isBranch());
		return m_node->children[m_childIndex]->value;
	}
	else return m_node->objectValue(m_objectIndex);
}

TREE_TEMPLATE
const typename TREE_QUAL::bbox_type &
TREE_QUAL::node_iterator::bbox() const
{
	return m_node->box;
}

TREE_TEMPLATE
void
TREE_QUAL::node_iterator::next()
{
	assert(valid());
	if (m_childIndex < 4 && m_node->isBranch())
	{
		++m_childIndex;
		while (m_childIndex < 4 && m_node->children[m_childIndex]->isEmpty()) ++m_childIndex;

		if (m_childIndex < 4)
			// found a children which is not empty
			return;
	}
	++m_objectIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
TREE_QUAL::depth_iterator::depth_iterator(const int maxLevel) : base_type(maxLevel) { }

TREE_TEMPLATE
ValueType &
TREE_QUAL::depth_iterator::operator*()
{
	assert(!base_type::valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->value;
}

TREE_TEMPLATE
const ValueType &
TREE_QUAL::depth_iterator::operator*() const
{
	assert(!base_type::valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->value;
}

TREE_TEMPLATE
void
TREE_QUAL::depth_iterator::next()
{
	for (;;)
	{
		if (m_tos <= 0) break;

		// Copy stack top cause it may change as we use it
		const typename base_type::StackElement curTos = pop();

		if (curTos.node->isLeaf())
		{
			// Reached leaves, fall back to previous level
		}
		else
		{
			// Keep walking through children while we can
			if (curTos.childIndex + 1 < 4)
			{
				// Push sibling on for future tree walk
				// This is the 'fall back' node when we finish with the current level
				push(curTos.node, curTos.childIndex + 1, 0);
			}
			else
			{
				if (curTos.childIndex == 3)
				{
					// Visit node after visiting the last child
					push(curTos.node, 4, 0);
				}
				else if (curTos.childIndex == 4)
				{
					assert(!curTos.node->isEmpty());
					push(curTos.node, 5, 0);
					break;
				}
				else if (curTos.childIndex > 4)
					// Node visited, fallback to previous
					continue;
			}

			// Since cur node is not a leaf, push first of next level to get deeper
			// into the tree
			node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
			if (nextLevelnode->isLeaf())
			{
				push(nextLevelnode, 0, 0);
				if (nextLevelnode->objectCount())
				{
					// If we pushed on a new leaf, exit as the data is ready at TOS
					break;
				}
			}
			else { push(nextLevelnode, 0, 0); }
		}
	}
}

TREE_TEMPLATE
typename TREE_QUAL::node_iterator
TREE_QUAL::depth_iterator::child()
{
	assert(m_tos > 0);
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return node_iterator(curTos.node, base_type::m_maxLevel);
}

TREE_TEMPLATE
typename TREE_QUAL::node_iterator
TREE_QUAL::depth_iterator::current()
{
	assert(m_tos > 0);
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return node_iterator(curTos.node, 4, base_type::m_maxLevel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
TREE_QUAL::leaf_iterator::leaf_iterator(const int maxLevel) : base_type(maxLevel) { }

TREE_TEMPLATE
void
TREE_QUAL::leaf_iterator::next()
{
	for (;;)
	{
		if (m_tos <= 0) break;

		// Copy stack top cause it may change as we use it
		const typename base_type::StackElement curTos = pop();

		assert(curTos.objectIndex >= -1);
		if (size_t(curTos.objectIndex + 1) < curTos.node->objectCount())
		{
			// Keep walking through data while we can
			push(curTos.node, curTos.objectIndex + 1);
			break;
		}

		if (curTos.node->isLeaf())
		{
			// reached leaves, fall back to previous level
		}
		else
		{
			// Keep walking through children while we can
			if (curTos.childIndex + 1 < 4)
			{
				// Push sibling on for future tree walk
				// This is the 'fall back' node when we finish with the current level
				push(curTos.node, curTos.childIndex + 1, curTos.objectIndex);
			}
			else if (curTos.childIndex >= 4)
				// Node visited, fallback to previous
				continue;

			// Since cur node is not a leaf, push first of next level to get deeper
			// into the tree
			node_ptr_type nextLevelnode = curTos.node->children[curTos.childIndex];
			if (nextLevelnode->isLeaf())
			{
				push(nextLevelnode, 0, 0);
				if (nextLevelnode->objectCount())
				{
					// If we pushed on a new leaf, exit as the data is ready at TOS
					break;
				}
			}
			else { push(nextLevelnode, 0, -1); }
		}
	}
}

#undef TREE_TEMPLATE
#undef TREE_QUAL

} // namespace candybox

namespace candybox {

namespace rtree {
// Type of element that allows fractional and large values such as float or
// double, for use in volume calculations.
template <typename T> struct RealType
{
	typedef float type;
};

template <> struct RealType<double>
{
	typedef double type;
};
} // namespace rtree

/**
	 @class RTree
	 @brief Implementation of a custom RTree tree based on the version
	 by Greg Douglas at Auran and original algorithm was by Toni Gutman.
			R-Trees provide Log(n) speed rectangular indexing into multi-dimensional
	 data. Only support the quadratric split heuristic.

			It has the following properties:
			- hierarchical, you can add values to the internal branch nodes
			- custom indexable getter similar to boost's
			- configurable volume calculation
			- custom allocation for internal nodes

	 @tparam T                type of the space(eg. int, float, etc.)
	 @tparam ValueType        type of value stored in the tree's nodes
	 @tparam Dimension        number of dimensions for the spatial space of the
	 bounding boxes
	 @tparam min_child_items  m, in the range 2 <= m < M
	 @tparam max_child_items  M, in the range 2 <= m < M
	 @tparam indexable_getter the indexable getter, i.e. the getter for the bounding
	 box of a value
	 @tparam bbox_volume_mode The rectangle calculation of volume, spherical results
	 better split classification but can be slower. While the normal can be faster
	 but worse classification.
	 @tparam RealType type of element that allows fractional and large
	 values such as float or double, for use in volume calculations.
	 @tparam custom_allocator the allocator class

	 @note It's recommended that ValueType should be a fast to copy object, eg: int,
	 id, obj*, etc.
	 */
template <
    typename T, //
    typename ValueType, //
    int Dimension, //
    int max_child_items = 8, //
    int min_child_items = max_child_items / 2, //
    typename indexable_getter = Indexable<typename TBox<T, Dimension>::tvec, ValueType>, //
    int bbox_volume_mode = detail::eNormalVolume, //
    typename RealType = typename rtree::RealType<T>::type, //
    typename custom_allocator =
        detail::allocator<detail::Node<ValueType, TBox<T, Dimension>, max_child_items>>>
class RTree
{
public:
	typedef RealType real_type;
	typedef TBox<T, Dimension> bbox_type;
	typedef custom_allocator allocator_type;

	static const size_t max_items = max_child_items;
	static const size_t min_items = min_child_items;

private:
	typedef detail::Node<ValueType, bbox_type, max_child_items> node_type;
	typedef node_type *node_ptr_type;
	typedef node_type **node_dptr_type;
	typedef typename node_type::branch_type branch_type;
	typedef typename node_type::count_type count_type;

public:
	class base_iterator : public detail::Stack<node_type, count_type>
	{
	public:
		/// Returns the depth level of the current branch.
		int level() const;
		/// Is iterator still valid.
		bool valid() const;
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		ValueType &operator*();
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		const ValueType &operator*() const;
		const bbox_type &bbox() const;

#ifdef TREE_DEBUG_TAG
		std::string &tag();
#endif
	protected:
		typedef detail::Stack<node_type, count_type> base_type;
		using base_type::m_stack;
		using base_type::m_tos;
	};
	/**
			 @brief Iterator to traverse the items of a node of the tree.
			 */
	struct node_iterator
	{
		node_iterator();

		/// Returns the depth level of the current branch.
		int level() const;
		/// Is iterator still valid.
		bool valid() const;
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		ValueType &operator*();
		/// Access the current data element. Caller must be sure iterator is not
		/// NULL first.
		const ValueType &operator*() const;
		const bbox_type &bbox() const;

		/// Advances to the next item.
		void next();

#ifdef TREE_DEBUG_TAG
		std::string &tag();
#endif

	private:
		node_iterator(node_ptr_type node);

		count_type m_index;
		node_ptr_type m_node;
		friend class RTree;
	};

	/**
			 @brief Iterator to traverse the whole tree similar to depth first search. It
			 reaches the lowest level
			 (i.e. leaves) and afterwards visits the higher levels until it reaches the
			 root(highest) level.
			 */
	class depth_iterator : public base_iterator
	{
		typedef base_iterator base_type;
		using base_type::m_stack;
		using base_type::m_tos;
		using base_type::push;
		using base_type::pop;

	public:
		// Returns a lower level, candybox. child, node of the current branch.
		node_iterator child();
		node_iterator current();

		/// Advances to the next item.
		void next();

	private:
		friend class RTree;
	};

	/**
			 @brief Iterator to traverse only the leaves(i.e. level 0) of the tree
			 similar, left to right order.
			 */
	class leaf_iterator : public base_iterator
	{
		typedef base_iterator base_type;
		using base_type::m_stack;
		using base_type::m_tos;
		using base_type::push;
		using base_type::pop;

	public:
		/// Advances to the next item.
		void next();

	private:
		friend class RTree;
	};

	RTree(indexable_getter indexable = indexable_getter(),
	      const allocator_type &allocator = allocator_type(),
	      bool allocateRoot = true);
	template <typename Iter>
	RTree(
	    Iter first,
	    Iter last, //
	    indexable_getter indexable = indexable_getter(), //
	    const allocator_type &allocator = allocator_type());
	RTree(const RTree &src);
#ifdef SPATIAL_TREE_USE_CPP11
	RTree(RTree &&src);
#endif
	~RTree();

	RTree &operator=(const RTree &rhs);
#ifdef SPATIAL_TREE_USE_CPP11
	RTree &operator=(RTree &&rhs);
#endif

	void swap(RTree &other);

	template <typename Iter> void insert(Iter first, Iter last);
	void insert(const ValueType &value);
	/// Insert the value if the predicate condition is true.
	template <typename Predicate>
	bool insert(const ValueType &value, const Predicate &predicate);
	bool remove(const ValueType &value);

	/// Translates the internal boxes with the given offset point.
	void translate(const T point[Dimension]);

	/// Special query to find all within search rectangle using the hierarchical
	/// order.
	/// @see SpatialPredicate for available predicates.
	template <typename Predicate> bool hierachical_query(const Predicate &predicate) const;
	/// \return Returns the number of entries found.
	template <typename Predicate, typename OutIter>
	size_t hierachical_query(const Predicate &predicate, OutIter out_it) const;
	/// Defines the target query level, if 0 then leaf values are retrieved
	/// otherwise hierarchical node values.
	/// @note Only used for hierachical_query.
	void setQueryTargetLevel(int level);

	/// @see SpatialPredicate for available predicates.
	template <typename BoxPredicate> bool query(const BoxPredicate &predicate) const;
	template <typename BoxPredicate, typename OutIter>
	size_t query(const BoxPredicate &predicate, OutIter out_it) const;

	/// Adds the value if the predicate condition is true.
	template <typename OutIter, typename Predicate = detail::AlwayTruePredicate>
	size_t rayQuery(
	    const glm::vec<Dimension, RealType> &rayOrigin,
	    const glm::vec<Dimension, RealType> &rayDirection,
	    OutIter out_it,
	    const Predicate &predicate = detail::AlwayTruePredicate()) const;

	/// Performs a nearest neighbour search.
	/// @note Uses the distance to the center of the bbox.
	template <typename OutIter>
	size_t nearest(const glm::vec<2, T> &point, T radius, OutIter out_it) const;

	/// Performs a knn-nearest search.
	/// /// @note Uses the minimum distance to the bbox.
	template <typename OutIter>
	size_t k_nearest(const glm::vec<Dimension, T> &point, uint32_t k, OutIter out_it) const;

	/// Remove all entries from tree
	void clear(bool recursiveCleanup = true);
	/// Count the data elements in this container.
	size_t count() const;
	/// Returns the estimated internal node count for the given number of elements
	static size_t nodeCount(size_t numberOfItems);
	/// Returns the estimated branch count for the given number of elements
	static size_t branchCount(size_t numberOfItems);
	/// Returns the number of levels(height) of the tree.
	size_t levels() const;
	/// Returns the estimated depth for the given number of elements
	static double levels(size_t numberOfItems);
	/// Returns the bbox of the root node.
	bbox_type bbox() const;

	/// Returns the custom allocator
	allocator_type &allocator();
	const allocator_type &allocator() const;

	node_iterator root();
	depth_iterator dbegin();
	leaf_iterator lbegin();

	/// Returns the size in bytes of the tree
	/// @param estimate if true then it estimates the size via using the item
	/// count for guessing the internal node
	/// count, otherwise it counts the nodes which is slower
	size_t bytes(bool estimate = true) const;

private:
	/// Variables for finding a split partition
	struct BranchVars
	{
		branch_type branches[max_child_items + 1];
		bbox_type coverSplit;
		real_type coverSplitArea;
	};

	struct PartitionVars : BranchVars
	{
		enum
		{
			ePartitionUnused = -1
		};

		int partitions[max_child_items + 1];
		count_type count[2];
		bbox_type cover[2];
		real_type area[2];

		PartitionVars() : m_maxFill(max_child_items + 1), m_minFill(min_child_items) { }

		void clear()
		{
			count[0] = count[1] = 0;
			area[0] = area[1] = (real_type)0;
			for (count_type index = 0; index < m_maxFill; ++index)
			{
				partitions[index] = PartitionVars::ePartitionUnused;
			}
		}

		count_type totalCount() const { return count[0] + count[1]; }

		count_type maxFill() const { return m_maxFill; }

		count_type minFill() const { return m_minFill; }

		count_type diffFill() const { return m_maxFill - m_minFill; }

	private:
		const count_type m_maxFill;
		const count_type m_minFill;
	};

	struct BranchDistance
	{
		RealType distance;
		count_type index;

		bool operator<(const BranchDistance &other) const { return distance < other.distance; }
	};


	struct NearestDistance
	{
		RealType distance;
		node_ptr_type node;
		count_type index;

		NearestDistance(node_ptr_type node, RealType distance, count_type index = 0xFFFFFFF)
		    : node(node), distance(distance), index(index)
		{
		}

		bool operator<(const NearestDistance &other) const
		{
			return distance > other.distance;
		}

		bool isLeaf() const { return node->isLeaf(); }

		bool isObject() const { return isLeaf() && index != 0xFFFFFFF; }

		const bbox_type &object_bbox() const
		{
			assert(isObject());
			return node->bboxes[index];
		}
	};

	template <typename Predicate>
	bool insertImpl(const branch_type &branch, const Predicate &predicate, int level);
	template <typename Predicate>
	bool insertRec(
	    const branch_type &branch,
	    const Predicate &predicate,
	    node_type &node,
	    node_ptr_type &newNode,
	    bool &added,
	    int level);
	void copyRec(const node_ptr_type src, node_ptr_type dst);

	count_type pickBranch(const bbox_type &bbox, const node_type &node) const;
	void getBranches(const node_type &node, const branch_type &branch, BranchVars &branchVars)
	    const;
	bool addBranch(const branch_type &branch, node_type &node, node_dptr_type newNode) const;

	void splitNode(node_type &node, const branch_type &branch, node_dptr_type newNode) const;
	void
	loadNodes(node_type &nodeA, node_type &nodeB, const PartitionVars &partitionVars) const;
	void choosePartition(PartitionVars &partitionVars) const;
	void pickSeeds(PartitionVars &partitionVars) const;
	void classify(count_type index, int group, PartitionVars &partitionVars) const;

	bool removeImpl(const bbox_type &bbox, const ValueType &value);
	bool removeRec(
	    const bbox_type &bbox,
	    const ValueType &value,
	    node_ptr_type node,
	    std::vector<node_ptr_type> &reInsertList);
	void clearRec(const node_ptr_type node);

	template <typename Predicate, typename OutIter>
	void queryHierachicalRec(
	    node_ptr_type node,
	    const Predicate &predicate,
	    size_t &foundCount,
	    OutIter out_it) const;

	template <typename Predicate, typename OutIter>
	void queryRec(
	    node_ptr_type node,
	    const Predicate &predicate,
	    size_t &foundCount,
	    OutIter out_it) const;

	template <typename Predicate, typename OutIter>
	void rayQueryRec(
	    node_ptr_type node,
	    const glm::vec<Dimension, RealType> &rayOrigin,
	    const glm::vec<Dimension, RealType> &rayDirection,
	    const Predicate &predicate,
	    size_t &foundCount,
	    OutIter out_it) const;

	template <typename OutIter>
	void nearestRec(
	    node_ptr_type node,
	    const glm::vec<2, T> &point,
	    T radius,
	    size_t &foundCount,
	    OutIter it) const;

	void translateRec(node_type &node, const glm::vec<Dimension, T> &point);
	size_t countImpl(const node_type &node) const;
	void countRec(const node_type &node, size_t &count) const;

	inline static RealType
	distance(const glm::vec<Dimension, T> &point0, const glm::vec<Dimension, T> &point1)
	{
		RealType d = RealType(point0[0] - point1[0]);
		d *= d;
		for (int i = 1; i < Dimension; i++)
		{
			const RealType temp = RealType(point0[i] - point1[i]);
			d += temp * temp;
		}
		return d;
	}

	inline static RealType distance(const glm::vec<Dimension, T> &point, const bbox_type &bbox)
	{
		RealType d = (RealType)
		    std::max(std::max(bbox.l[0] - point[0], (T)0), point[0] - bbox.u[0]);
		d *= d;
		for (int i = 1; i < Dimension; i++)
		{
			const RealType temp = (RealType)
			    std::max(std::max(bbox.l[i] - point[i], (T)0), point[i] - bbox.u[i]);
			d += temp * temp;
		}

		return d;
	}

private:
	indexable_getter m_indexable;
	mutable allocator_type m_allocator;
	mutable PartitionVars m_parVars;
	size_t m_count;
	int m_queryTargetLevel;
	node_ptr_type m_root;

	template <class RTreeClass>
	friend typename RTreeClass::node_ptr_type &detail::getRootNode(RTreeClass &tree);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TREE_TEMPLATE                                                                         \
	template <                                                                                \
	    typename T, typename ValueType, int Dimension, int max_child_items,                   \
	    int min_child_items, typename indexable_getter, int bbox_volume_mode,                 \
	    typename RealType, typename custom_allocator>

#define TREE_QUAL                                                                             \
	RTree<T, ValueType, Dimension, max_child_items, min_child_items, indexable_getter,        \
	      bbox_volume_mode, RealType, custom_allocator>

TREE_TEMPLATE
TREE_QUAL::RTree(
    indexable_getter indexable /*= indexable_getter()*/,
    const allocator_type &allocator /*= allocator_type()*/,
    bool allocateRoot /*= true*/)
    : m_indexable(indexable),
      m_allocator(allocator),
      m_count(0),
      m_queryTargetLevel(0),
      m_root(allocateRoot ? detail::allocate(m_allocator, 0) : NULL)
{
	static_assert(max_child_items > min_child_items, "Invalid child size!");
	static_assert(min_child_items > 0, "Invalid child size!");
}

TREE_TEMPLATE
template <typename Iter>
TREE_QUAL::RTree(
    Iter first,
    Iter last,
    indexable_getter indexable /*= indexable_getter()*/,
    const allocator_type &allocator /*= allocator_type()*/)
    : m_indexable(indexable), m_allocator(allocator), m_count(0), m_queryTargetLevel(0)
{
	static_assert(max_child_items > min_child_items, "Invalid child size!");
	static_assert(min_child_items > 0, "Invalid child size!");

	m_root = detail::allocate(m_allocator, 0);
	// TODO: use packing algorithm
	insert(first, last);
}

TREE_TEMPLATE
TREE_QUAL::RTree(const RTree &src)
    : m_indexable(src.m_indexable),
      m_allocator(src.m_allocator),
      m_count(src.m_count),
      m_queryTargetLevel(src.m_queryTargetLevel),
      m_root(detail::allocate(m_allocator, 0))
{
	copyRec(src.m_root, m_root);
}

#ifdef SPATIAL_TREE_USE_CPP11
TREE_TEMPLATE
TREE_QUAL::RTree(RTree &&src) : m_root(NULL) { swap(src); }
#endif

TREE_TEMPLATE
TREE_QUAL::~RTree()
{
	if (m_root) clearRec(m_root);
}

TREE_TEMPLATE
TREE_QUAL &
TREE_QUAL::operator=(const RTree &rhs)
{
	if (&rhs != this)
	{
		if (m_count > 0) clear(true);

		m_count = rhs.m_count;
		m_queryTargetLevel = rhs.m_queryTargetLevel;
		m_allocator = rhs.m_allocator;
		m_indexable = rhs.m_indexable;
		copyRec(rhs.m_root, m_root);
	}
	return *this;
}

#ifdef SPATIAL_TREE_USE_CPP11
TREE_TEMPLATE
TREE_QUAL &
TREE_QUAL::operator=(RTree &&rhs)
{
	assert(this != &rhs);

	if (m_count > 0) clear(true);
	swap(rhs);

	return *this;
}
#endif

TREE_TEMPLATE
void
TREE_QUAL::swap(RTree &other)
{
	std::swap(m_root, other.m_root);
	std::swap(m_count, other.m_count);
	std::swap(m_queryTargetLevel, other.m_queryTargetLevel);
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_indexable, other.m_indexable);
}

TREE_TEMPLATE
template <typename Iter>
void
TREE_QUAL::insert(Iter first, Iter last)
{
	assert(m_root);

	branch_type branch;
	branch.child = NULL;

	for (Iter it = first; it != last; ++it)
	{
		const ValueType &value = *it;
		branch.value = value;
		branch.bbox.set(m_indexable.min(value), m_indexable.max(value));
		insertImpl(branch, detail::DummyInsertPredicate(), 0);

		++m_count;
	}
}

TREE_TEMPLATE
void
TREE_QUAL::insert(const ValueType &value)
{
	assert(m_root);

	branch_type branch;
	branch.value = value;
	branch.child = NULL;
	branch.bbox.set(m_indexable.min(value), m_indexable.max(value));
	insertImpl(branch, detail::DummyInsertPredicate(), 0);

	++m_count;
}

TREE_TEMPLATE
template <typename Predicate>
bool
TREE_QUAL::insert(const ValueType &value, const Predicate &predicate)
{
	assert(m_root);

	branch_type branch;
	branch.value = value;
	branch.child = NULL;
	branch.bbox.set(m_indexable.min(value), m_indexable.max(value));
	bool success = insertImpl(branch, predicate, 0);
	if (success) ++m_count;

	return success;
}

TREE_TEMPLATE
void
TREE_QUAL::translate(const T point[Dimension])
{
	assert(m_root);

	translateRec(*m_root, point);
}

TREE_TEMPLATE
bool
TREE_QUAL::remove(const ValueType &value)
{
	assert(m_root);

	const bbox_type bbox(m_indexable.min(value), m_indexable.max(value));
	if (removeImpl(bbox, value))
	{
		--m_count;
		return true;
	}
	return false;
}

TREE_TEMPLATE
template <typename Predicate>
bool
TREE_QUAL::hierachical_query(const Predicate &predicate) const
{
	return hierachical_query(predicate, detail::dummy_iterator()) > 0;
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
size_t
TREE_QUAL::hierachical_query(const Predicate &predicate, OutIter out_it) const
{
	size_t foundCount = 0;
	queryHierachicalRec(m_root, predicate, foundCount, out_it);
	return foundCount;
}

TREE_TEMPLATE
template <typename Predicate>
bool
TREE_QUAL::query(const Predicate &predicate) const
{
	return query(predicate, detail::dummy_iterator()) > 0;
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
size_t
TREE_QUAL::query(const Predicate &predicate, OutIter out_it) const
{
	size_t foundCount = 0;
	queryRec(m_root, predicate, foundCount, out_it);
	return foundCount;
}

TREE_TEMPLATE
template <typename OutIter, typename Predicate>
size_t
TREE_QUAL::rayQuery(
    const glm::vec<Dimension, RealType> &rayOrigin,
    const glm::vec<Dimension, RealType> &rayDirection,
    OutIter out_it,
    const Predicate &predicate) const
{
	size_t foundCount = 0;
	rayQueryRec(m_root, rayOrigin, rayDirection, predicate, foundCount, out_it);
	return foundCount;
}

TREE_TEMPLATE
template <typename OutIter>
size_t
TREE_QUAL::nearest(const glm::vec<2, T> &point, T radius, OutIter out_it) const
{
	size_t foundCount = 0;
	nearestRec(m_root, point, radius, foundCount, out_it);

	return foundCount;
}

TREE_TEMPLATE
template <typename OutIter>
size_t
TREE_QUAL::k_nearest(const glm::vec<Dimension, T> &point, uint32_t k, OutIter out_it) const
{
	size_t foundCount = 0;

	// incremental nearest neighbor search
	std::priority_queue<NearestDistance> queue;

	queue.emplace(m_root, 0);
	while (!queue.empty())
	{
		const NearestDistance element = queue.top();
		queue.pop();
		if (element.isObject())
		{
			if (!queue.empty() &&
			    distance(point, element.object_bbox()) > queue.top().distance)
			{
				queue.push(element);
			}
			else
			{
				*out_it = element.node->values[element.index];
				++out_it;
				if (++foundCount == k) { break; }
			}
		}
		else if (element.isLeaf())
		{
			for (count_type index = 0; index < element.node->count; ++index)
			{
				const bbox_type &bbox = element.node->bboxes[index];
				RealType d = distance(point, bbox);
				queue.emplace(element.node, d, index);
			}
		}
		else
		{
			// is a branch
			for (count_type index = 0; index < element.node->count; ++index)
			{
				const bbox_type &bbox = element.node->bboxes[index];
				RealType d = distance(point, bbox);
				queue.emplace(element.node->children[index], d);
			}
		}
	}

	return foundCount;
}

TREE_TEMPLATE
void
TREE_QUAL::setQueryTargetLevel(int level)
{
	m_queryTargetLevel = level;
}

TREE_TEMPLATE
size_t
TREE_QUAL::count() const
{
	assert(m_root);
	assert(m_count == countImpl(*m_root));

	return m_count;
}

TREE_TEMPLATE
size_t
TREE_QUAL::bytes(bool estimate /*= true*/) const
{
	size_t size = sizeof(RTree);

	if (!m_count) return size;

	if (estimate) return size + nodeCount(count()) * sizeof(node_type);

	depth_iterator it = const_cast<RTree &>(*this).dbegin();
	size_t n = 0;
	for (; !it.isNull(); ++it) { ++n; }

	return size + n * sizeof(node_type);
}

TREE_TEMPLATE
size_t
TREE_QUAL::nodeCount(size_t numberOfItems)
{
	const double k = min_child_items;
	const double invK1 = 1.0 / (k - 1.0);

	const double depth = levels(numberOfItems);
	// perfectly balanced k-ary tree formula
	double count = std::pow(k, depth + 1.0) - 1;
	count *= invK1;
	return std::max(count * 0.5, 1.0);
}

TREE_TEMPLATE
size_t
TREE_QUAL::branchCount(size_t numberOfItems)
{
	size_t count =
	    nodeCount(numberOfItems) * (min_child_items + max_child_items / min_child_items);
	return count;
}

TREE_TEMPLATE
size_t
TREE_QUAL::levels() const
{
	assert(m_root);
	return m_root->level;
}

TREE_TEMPLATE
double
TREE_QUAL::levels(size_t numberOfItems)
{
	static const double invLog = 1 / std::log(min_child_items);
	const double depth = std::log(numberOfItems) * invLog - 1;
	return depth;
}

TREE_TEMPLATE
typename TREE_QUAL::bbox_type
TREE_QUAL::bbox() const
{
	assert(m_root);
	return m_root->cover();
}

TREE_TEMPLATE
typename TREE_QUAL::allocator_type &
TREE_QUAL::allocator()
{
	return m_allocator;
}

TREE_TEMPLATE
const typename TREE_QUAL::allocator_type &
TREE_QUAL::allocator() const
{
	return m_allocator;
}

TREE_TEMPLATE
void
TREE_QUAL::countRec(const node_type &node, size_t &count) const
{
	if (node.isBranch()) // not a leaf node
	{
		for (count_type index = 0; index < node.count; ++index)
		{
			assert(node.children[index] != NULL);
			countRec(*node.children[index], count);
		}
	}
	else // A leaf node
	{
		count += node.count;
	}
}

TREE_TEMPLATE
void
TREE_QUAL::translateRec(node_type &node, const glm::vec<Dimension, T> &point)
{
	for (count_type index = 0; index < node.count; ++index)
	{
		node.bboxes[index].translate(point);
	}

	if (node.isBranch()) // not a leaf node
	{
		for (count_type index = 0; index < node.count; ++index)
		{
			assert(node.children[index] != NULL);
			translateRec(*node.children[index], point);
		}
	}
	// A leaf node
}

TREE_TEMPLATE
size_t
TREE_QUAL::countImpl(const node_type &node) const
{
	size_t count = 0;
	countRec(node, count);
	return count;
}

TREE_TEMPLATE
void
TREE_QUAL::clear(bool recursiveCleanup /*= true*/)
{
	if (recursiveCleanup && m_root) clearRec(m_root);

	m_root = detail::allocate(m_allocator, 0);
	m_count = 0;
}

TREE_TEMPLATE
void
TREE_QUAL::clearRec(const node_ptr_type node)
{
	assert(node);
	assert(node->level >= 0);

	if (node->isBranch()) // This is an internal node in the tree
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			clearRec(node->children[index]);
		}
	}
	detail::deallocate(m_allocator, node);
}

// Inserts a new data rectangle into the index structure.
// Recursively descends tree, propagates splits back up.
// Returns 0 if node was not split.  Old node updated.
// If node was split, returns 1 and sets the pointer pointed to by
// new_node to point to the new node.  Old node updated to become one of two.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
TREE_TEMPLATE
template <typename Predicate>
bool
TREE_QUAL::insertRec(
    const branch_type &branch,
    const Predicate &predicate,
    node_type &node,
    node_ptr_type &newNode,
    bool &added,
    int level)
{
	assert(level >= 0 && level <= node.level);

	// TODO: profile and test a non-recursive version

	// recurse until we reach the correct level for the new record. data records
	// will always be called with level == 0 (leaf)
	if (node.level > level)
	{
		// Still above level for insertion, go down tree recursively
		node_ptr_type otherNode = NULL;

		// find the optimal branch for this record
		const count_type index = pickBranch(branch.bbox, node);

		// recursively insert this record into the picked branch
		assert(node.children[index]);
		bool childWasSplit =
		    insertRec(branch, predicate, *node.children[index], otherNode, added, level);

		if (!childWasSplit)
		{
			// Child was not split. Merge the bounding box of the new record with the
			// existing bounding box
			if (added) node.bboxes[index] = branch.bbox.extended(node.bboxes[index]);
			return false;
		}
		else if (otherNode)
		{
			// Child was split. The old branches are now re-partitioned to two nodes
			// so we have to re-calculate the bounding boxes of each node
			node.bboxes[index] = node.children[index]->cover();
			branch_type branch;
			branch.child = otherNode;
			branch.bbox = otherNode->cover();

			// The old node is already a child of node. Now add the newly-created
			// node to node as well. node might be split because of that.
			return addBranch(branch, node, &newNode);
		}
	}
	else if (node.level == level)
	{
		// Check if we should add the branch
		if (!detail::checkInsertPredicate(predicate, node))
		{
			added = false;
			return false;
		}

		// We have reached level for insertion. Add bbox, split if necessary
		return addBranch(branch, node, &newNode);
	}

	// Should never occur
	assert(0);
	return false;
}

// Insert a data rectangle into an index structure.
// insertImpl provides for splitting the root;
// returns 1 if root was split, 0 if it was not.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
// insertRec does the recursion.
TREE_TEMPLATE
template <typename Predicate>
bool
TREE_QUAL::insertImpl(const branch_type &branch, const Predicate &predicate, int level)
{
	assert(m_root);
	assert(level >= 0 && level <= m_root->level);
#ifndef NDEBUG
	for (int index = 0; index < Dimension; ++index)
	{
		assert(branch.bbox.l[index] <= branch.bbox.u[index]);
	}
#endif

	node_ptr_type newNode = NULL;
	bool added = true;

	// Check if root was split
	if (insertRec(branch, predicate, *m_root, newNode, added, level))
	{
		// Grow tree taller and new root
		node_ptr_type newRoot = detail::allocate(m_allocator, m_root->level + 1);

		branch_type branch;
		// add old root node as a child of the new root
		branch.bbox = m_root->cover();
		branch.child = m_root;
		addBranch(branch, *newRoot, NULL);

		// add the split node as a child of the new root
		branch.bbox = newNode->cover();
		branch.child = newNode;
		addBranch(branch, *newRoot, NULL);
		// set the new root as the root node
		m_root = newRoot;
	}
	return added;
}

TREE_TEMPLATE
void
TREE_QUAL::copyRec(const node_ptr_type src, node_ptr_type dst)
{
	*dst = *src;
	for (count_type index = 0; index < src->count; ++index)
	{
		const node_ptr_type srcCurrent = src->children[index];
		if (srcCurrent)
		{
			node_ptr_type dstCurrent = dst->children[index] =
			    detail::allocate(m_allocator, srcCurrent->level);
			copyRec(srcCurrent, dstCurrent);
		}
	}
}

// Add a branch to a node.  Split the node if necessary.
// Returns 0 if node not split.  Old node updated.
// Returns 1 if node split, sets *new_node to address of new node.
// Old node updated, becomes one of two.
TREE_TEMPLATE
bool
TREE_QUAL::addBranch(const branch_type &branch, node_type &node, node_dptr_type newNode) const
{
	if (node.addBranch(branch)) { return false; }

	assert(newNode);
	splitNode(node, branch, newNode);

	return true;
}

// Pick a branch.  Pick the one that will need the smallest increase
// in area to fit the new rectangle.  This will result in the
// least total area for the covering rectangles in the current node.
// In case of a tie, pick the one which was smaller before, to get
// the best resolution when searching.
TREE_TEMPLATE
typename TREE_QUAL::count_type
TREE_QUAL::pickBranch(const bbox_type &bbox, const node_type &node) const
{
	assert(node.count);

	count_type best;
	real_type bestArea;
	real_type bestIncrease;
	{
		const bbox_type &currentBBox = node.bboxes[0];
		const real_type area = currentBBox.template volume<bbox_volume_mode, RealType>();
		const bbox_type extendedBBox = bbox.extended(currentBBox);
		const real_type increase =
		    extendedBBox.template volume<bbox_volume_mode, RealType>() - area;
		best = 0;
		bestArea = area;
		bestIncrease = increase;
	}
	for (count_type index = 1; index < node.count; ++index)
	{
		const bbox_type &currentBBox = node.bboxes[index];
		const real_type area = currentBBox.template volume<bbox_volume_mode, RealType>();
		const bbox_type extendedBBox = bbox.extended(currentBBox);
		const real_type increase =
		    extendedBBox.template volume<bbox_volume_mode, RealType>() - area;
		if ((increase == bestIncrease) && (area < bestArea))
		{
			best = index;
			bestArea = area;
			bestIncrease = increase;
		}
	}
	return best;
}

// Split a node.
// Divides the nodes branches and the extra one between two nodes.
// Old node is one of the new ones, and one really new one is created.
// Tries more than one method for choosing a partition, uses best result.
TREE_TEMPLATE
void
TREE_QUAL::splitNode(node_type &node, const branch_type &branch, node_dptr_type newNodePtr)
    const
{
	assert(newNodePtr);
	node_ptr_type &newNode = *newNodePtr;

	// Could just use local here, but member or external is faster since it is
	// reused
	m_parVars.clear();

	// Load all the branches into a buffer, initialize old node
	getBranches(node, branch, m_parVars);

	// Find partition
	choosePartition(m_parVars);

	// Create a new node to hold (about) half of the branches
	newNode = detail::allocate(m_allocator, node.level);

	assert(newNode);
	// Put branches from buffer into 2 nodes according to the chosen partition
	node.count = 0;
	loadNodes(node, *newNode, m_parVars);

	assert((node.count + newNode->count) == m_parVars.maxFill());
}

// Load branch buffer with branches from full node plus the extra branch.
TREE_TEMPLATE
void
TREE_QUAL::
    getBranches(const node_type &node, const branch_type &branch, BranchVars &branchVars) const
{
	assert(node.count == max_child_items);

	// Load the branch buffer
	for (size_t index = 0; index < max_child_items; ++index)
	{
		branch_type &branch = branchVars.branches[index];
		branch.bbox = node.bboxes[index];
		branch.value = node.values[index];
		branch.child = node.children[index];
	}
	branchVars.branches[max_child_items] = branch;

	// Calculate bbox containing all in the set
	branchVars.coverSplit = branchVars.branches[0].bbox;
	for (int index = 1; index < max_child_items + 1; ++index)
	{
		branchVars.coverSplit.extend(branchVars.branches[index].bbox);
	}
	branchVars
	    .coverSplitArea = branchVars.coverSplit.template volume<bbox_volume_mode, RealType>();
}

// Method #0 for choosing a partition:
// As the seeds for the two groups, pick the two rects that would waste the
// most area if covered by a single rectangle, i.e. evidently the worst pair
// to have in the same group.
// Of the remaining, one at a time is chosen to be put in one of the two groups.
// The one chosen is the one with the greatest difference in area expansion
// depending on which group - the bbox most strongly attracted to one group
// and repelled from the other.
// If one group gets too full (more would force other group to violate min
// fill requirement) then other group gets the rest.
// These last are the ones that can go in either group most easily.
TREE_TEMPLATE
void
TREE_QUAL::choosePartition(PartitionVars &partitionVars) const
{
	real_type biggestDiff;
	count_type chosen;
	int group, betterGroup;

	pickSeeds(partitionVars);

	while (
	    ((partitionVars.totalCount()) < partitionVars.maxFill()) &&
	    (partitionVars.count[0] < partitionVars.diffFill()) &&
	    (partitionVars.count[1] < partitionVars.diffFill()))
	{
		biggestDiff = (real_type)-1;
		for (count_type index = 0; index < partitionVars.maxFill(); ++index)
		{
			if (PartitionVars::ePartitionUnused != partitionVars.partitions[index]) continue;

			const bbox_type &curbbox_type = partitionVars.branches[index].bbox;
			bbox_type bbox0 = curbbox_type.extended(partitionVars.cover[0]);
			bbox_type bbox1 = curbbox_type.extended(partitionVars.cover[1]);
			real_type growth0 =
			    bbox0.template volume<bbox_volume_mode, RealType>() - partitionVars.area[0];
			real_type growth1 =
			    bbox1.template volume<bbox_volume_mode, RealType>() - partitionVars.area[1];
			real_type diff = growth1 - growth0;
			if (diff >= 0) { group = 0; }
			else
			{
				group = 1;
				diff = -diff;
			}

			if (diff > biggestDiff)
			{
				biggestDiff = diff;
				chosen = index;
				betterGroup = group;
			}
			else if ((diff == biggestDiff) &&
			         (partitionVars.count[group] < partitionVars.count[betterGroup]))
			{
				chosen = index;
				betterGroup = group;
			}
		}
		classify(chosen, betterGroup, partitionVars);
	}

	// If one group too full, put remaining bboxs in the other
	if (partitionVars.totalCount() < partitionVars.maxFill())
	{
		if (partitionVars.count[0] >= partitionVars.diffFill()) { group = 1; }
		else { group = 0; }
		for (count_type index = 0; index < partitionVars.maxFill(); ++index)
		{
			if (PartitionVars::ePartitionUnused == partitionVars.partitions[index])
			{
				classify(index, group, partitionVars);
			}
		}
	}

	assert(partitionVars.totalCount() == partitionVars.maxFill());
	assert((partitionVars.count[0] >= partitionVars.minFill()) &&
	       (partitionVars.count[1] >= partitionVars.minFill()));
}

// Copy branches from the buffer into two nodes according to the partition.
TREE_TEMPLATE
void
TREE_QUAL::loadNodes(node_type &nodeA, node_type &nodeB, const PartitionVars &partitionVars)
    const
{
	node_ptr_type targetNodes[] = {&nodeA, &nodeB};
	for (count_type index = 0; index < partitionVars.maxFill(); ++index)
	{
		assert(partitionVars.partitions[index] == 0 || partitionVars.partitions[index] == 1);

		int targetNodeIndex = partitionVars.partitions[index];

		// It is assured that addBranch here will not cause a node split.
		bool nodeWasSplit =
		    addBranch(partitionVars.branches[index], *targetNodes[targetNodeIndex], NULL);
		(void)(nodeWasSplit);
		assert(!nodeWasSplit);
	}
}

TREE_TEMPLATE
void
TREE_QUAL::pickSeeds(PartitionVars &partitionVars) const
{
	count_type seed0, seed1;
	real_type worst, waste;
	real_type area[max_child_items + 1];

	for (size_t index = 0; index < partitionVars.maxFill(); ++index)
	{
		area[index] =
		    partitionVars.branches[index].bbox.template volume<bbox_volume_mode, RealType>();
	}

	worst = -partitionVars.coverSplitArea - 1;
	for (count_type indexA = 0; indexA < partitionVars.maxFill() - 1; ++indexA)
	{
		for (count_type indexB = indexA + 1; indexB < partitionVars.maxFill(); ++indexB)
		{
			bbox_type onebbox_type = partitionVars.branches[indexA].bbox.extended(
			    partitionVars.branches[indexB].bbox);
			waste = onebbox_type.template volume<bbox_volume_mode, RealType>() - area[indexA] -
			        area[indexB];
			if (waste > worst)
			{
				worst = waste;
				seed0 = indexA;
				seed1 = indexB;
			}
		}
	}

	classify(seed0, 0, partitionVars);
	classify(seed1, 1, partitionVars);
}

// Put a branch in one of the groups.
TREE_TEMPLATE
void
TREE_QUAL::classify(count_type index, int group, PartitionVars &partitionVars) const
{
	assert(PartitionVars::ePartitionUnused == partitionVars.partitions[index]);

	partitionVars.partitions[index] = group;

	// Calculate combined bbox
	if (partitionVars.count[group] == 0)
	{
		partitionVars.cover[group] = partitionVars.branches[index].bbox;
	}
	else
	{
		partitionVars.cover[group] =
		    partitionVars.branches[index].bbox.extended(partitionVars.cover[group]);
	}

	// Calculate volume of combined bbox
	partitionVars.area[group] =
	    partitionVars.cover[group].template volume<bbox_volume_mode, RealType>();

	++partitionVars.count[group];
}

// Delete a data rectangle from an index structure.
// Pass in a pointer to a bbox_type, the id of the record, ptr to ptr to root
// node.
// Returns 0 if record not found, 1 if success.
// removeImpl provides for eliminating the root.
TREE_TEMPLATE
bool
TREE_QUAL::removeImpl(const bbox_type &bbox, const ValueType &value)
{
	assert(m_root);

	std::vector<node_ptr_type> reInsertList;
	if (removeRec(bbox, value, m_root, reInsertList))
	{
		// Found and deleted a data item
		// Reinsert any branches from eliminated nodes
		for (size_t i = 0; i < reInsertList.size(); ++i)
		{
			node_ptr_type tempNode = reInsertList[i];

			for (count_type index = 0; index < tempNode->count; ++index)
			{
				branch_type branch;
				branch.bbox = tempNode->bboxes[index];
				branch.value = tempNode->values[index];
				branch.child = tempNode->children[index];
				insertImpl(branch, detail::DummyInsertPredicate(), tempNode->level);
			}
			m_allocator.deallocate(tempNode, 1);
		}

		// Check for redundant root (not leaf, 1 child) and eliminate TODO replace
		// if with while? In case there is a whole branch of redundant roots...
		if (m_root->count == 1 && m_root->isBranch())
		{
			node_ptr_type tempNode = m_root->children[0];

			assert(tempNode);
			m_allocator.deallocate(m_root, 1);
			m_root = tempNode;
		}
		return true;
	}

	return false;
}

// Delete a rectangle from non-root part of an index structure.
// Called by removeImpl.  Descends tree recursively,
// merges branches on the way back up.
// Returns 0 if record not found, 1 if success.
TREE_TEMPLATE
bool
TREE_QUAL::removeRec(
    const bbox_type &bbox,
    const ValueType &value,
    node_ptr_type node,

    std::vector<node_ptr_type> &reInsertList)
{
	assert(node);
	assert(node->level >= 0);

	if (node->isBranch()) // not a leaf node
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			if (!bbox.overlaps(node->bboxes[index])) continue;

			node_ptr_type currentNode = node->children[index];
			if (removeRec(bbox, value, currentNode, reInsertList))
			{
				if (currentNode->count >= min_child_items)
				{
					// child removed, just resize parent bbox
					node->bboxes[index] = currentNode->cover();
				}
				else
				{
					// child removed, not enough entries in node, eliminate node
					reInsertList.push_back(currentNode);
					node->disconnectBranch(index);
				}
				return true;
			}
		}
	}
	else // A leaf node
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			if (node->values[index] == value)
			{
				node->disconnectBranch(index);
				return true;
			}
		}
	}
	return false;
}

// Search in an index tree or subtree for all data retangles that overlap the
// argument rectangle and stop at first node that is fully containted by the
// bbox.
TREE_TEMPLATE
template <typename Predicate, typename OutIter>
void
TREE_QUAL::queryHierachicalRec(
    node_ptr_type node,
    const Predicate &predicate,
    size_t &foundCount,
    OutIter it) const
{
	assert(node);
	assert(node->level >= 0);

	if (node->level <= m_queryTargetLevel)
	{
		// This is a target or lower level node
		for (count_type index = 0; index < node->count; ++index)
		{
			if (predicate(node->bboxes[index]))
			{
				*it = node->values[index];
				++it;
				++foundCount;
			}
		}
	}
	else
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			const bbox_type &nodeBBox = node->bboxes[index];
			// Branch is fully contained, dont search further
			if (predicate.bbox.contains(nodeBBox))
			{
				*it = node->values[index];
				++it;
				++foundCount;
			}
			else if (predicate.bbox.overlaps(nodeBBox))
			{
				queryHierachicalRec(node->children[index], predicate, foundCount, it);
			}
		}
	}
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
void
TREE_QUAL::
    queryRec(node_ptr_type node, const Predicate &predicate, size_t &foundCount, OutIter it)
        const
{
	assert(node);
	assert(node->level >= 0);

	if (node->isLeaf())
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			if (predicate(node->bboxes[index]))
			{
				*it = node->values[index];
				++it;
				++foundCount;
			}
		}
	}
	else
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			const bbox_type &nodeBBox = node->bboxes[index];

			if (predicate.bbox.overlaps(nodeBBox))
			{
				queryRec(node->children[index], predicate, foundCount, it);
			}
		}
	}
}

TREE_TEMPLATE
template <typename Predicate, typename OutIter>
void
TREE_QUAL::rayQueryRec(
    node_ptr_type node,
    const glm::vec<Dimension, RealType> &rayOrigin,
    const glm::vec<Dimension, RealType> &rayDirection,
    const Predicate &predicate,
    size_t &foundCount,
    OutIter it) const
{
	assert(node);
	assert(node->level >= 0);

	if (node->isLeaf())
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			if (predicate(node->values[index]) &&
			    node->bboxes[index].template intersectsRay<RealType>(rayOrigin, rayDirection))
			{
				*it = node->values[index];
				++it;
				++foundCount;
			}
		}
	}
	else
	{
		for (count_type index = 0; index < node->count; ++index)
		{
			const bbox_type &nodeBBox = node->bboxes[index];

			if (nodeBBox.template intersectsRay<RealType>(rayOrigin, rayDirection))
			{
				rayQueryRec(
				    node->children[index], rayOrigin, rayDirection, predicate, foundCount, it);
			}
		}
	}
}

TREE_TEMPLATE
template <typename OutIter>
void
TREE_QUAL::nearestRec(
    node_ptr_type node,
    const glm::vec<2, T> &point,
    T radius,
    size_t &foundCount,
    OutIter it) const
{
	static_assert(Dimension == 2, "Only for 2 dimensions!");

	assert(node);
	assert(node->level >= 0);

	if (node->isLeaf())
	{
		BranchDistance branches[max_child_items];

		count_type current = 0;
		for (count_type index = 0; index < node->count; ++index)
		{
			const bbox_type &nodeBBox = node->bboxes[index];

			if (nodeBBox.overlaps(point, radius))
			{
				BranchDistance &branch = branches[current++];
				branch.index = index;

				// use euclidean distance
				glm::vec<2, T> center = nodeBBox.center();
				branch.distance = distance(point, center);
			}
		}

		// sort branches based on distances
		std::sort(branches, branches + current);

		// save results
		for (count_type i = 0; i < current; ++i)
		{
			BranchDistance &branch = branches[i];

			*it = node->values[branch.index];
			++it;
			++foundCount;
		}
	}
	else
	{
		// compute distance for each branch
		BranchDistance branches[max_child_items];
		for (count_type index = 0; index < node->count; ++index)
		{
			const bbox_type &nodeBBox = node->bboxes[index];

			BranchDistance &branch = branches[index];
			// check if the radius overlaps with the node's bbox
			if (nodeBBox.overlaps(point, radius))
			{
				// use euclidean distance
				glm::vec<2, T> center = nodeBBox.center();
				branch.distance = distance(point, center);
			}
			else
				// discarded branch
				branch.distance = -1;

			branch.index = index;
		}

		// sort branches based on distances
		std::sort(branches, branches + node->count);
		// prune branches
		count_type first = node->count;
		for (count_type index = 0; index < node->count; ++index)
		{
			BranchDistance &branch = branches[index];
			if (branch.distance >= 0)
			{
				first = index;
				break;
			}
		}

		// iterate through the active branches
		for (count_type index = first; index < node->count; ++index)
		{
			BranchDistance &branch = branches[index];
			nearestRec(node->children[branch.index], point, radius, foundCount, it);
		}
	}
}

TREE_TEMPLATE
typename TREE_QUAL::node_iterator
TREE_QUAL::root()
{
	return node_iterator(m_root);
}

TREE_TEMPLATE
typename TREE_QUAL::depth_iterator
TREE_QUAL::dbegin()
{
	typedef detail::Stack<node_type, count_type> base_it_type;

	depth_iterator it;

	node_ptr_type first = m_root;
	while (first)
	{
		if (first->level == 1) // penultimate level
		{
			if (first->count) { it.push(first, 0, base_it_type::eNormal); }
			break;
		}
		else if (first->count > 0) { it.push(first, 0, base_it_type::eNormal); }

		first = first->children[0];
	}
	return it;
}

TREE_TEMPLATE
typename TREE_QUAL::leaf_iterator
TREE_QUAL::lbegin()
{
	leaf_iterator it;

	node_ptr_type first = m_root;
	while (first)
	{
		if (first->isBranch() && first->count > 1)
		{
			it.push(first, 1); // Descend sibling branch later
		}
		else if (first->isLeaf())
		{
			if (first->count) { it.push(first, 0); }
			break;
		}

		first = first->children[0];
	}
	return it;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
int
TREE_QUAL::base_iterator::level() const
{
	assert(m_tos > 0);
	const typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->level;
}

TREE_TEMPLATE
bool
TREE_QUAL::base_iterator::valid() const
{
	return (m_tos > 0);
}

TREE_TEMPLATE
ValueType &
TREE_QUAL::base_iterator::operator*()
{
	assert(valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->values[curTos.branchIndex];
}

TREE_TEMPLATE
const ValueType &
TREE_QUAL::base_iterator::operator*() const
{
	assert(valid());
	const typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->values[curTos.branchIndex];
}

TREE_TEMPLATE
const typename TREE_QUAL::bbox_type &
TREE_QUAL::base_iterator::bbox() const
{
	assert(valid());
	const typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->bboxes[curTos.branchIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
TREE_QUAL::node_iterator::node_iterator() : m_index(0), m_node(NULL) { }

TREE_TEMPLATE
TREE_QUAL::node_iterator::node_iterator(node_ptr_type node) : m_index(0), m_node(node)
{
	assert(node);
}

TREE_TEMPLATE
int
TREE_QUAL::node_iterator::level() const
{
	assert(m_node);
	return m_node->level;
}

TREE_TEMPLATE
bool
TREE_QUAL::node_iterator::valid() const
{
	assert(m_node);
	return (m_index < m_node->count);
}

TREE_TEMPLATE
ValueType &
TREE_QUAL::node_iterator::operator*()
{
	assert(valid());
	return m_node->values[m_index];
}

TREE_TEMPLATE
const ValueType &
TREE_QUAL::node_iterator::operator*() const
{
	assert(valid());
	return m_node->values[m_index];
}

TREE_TEMPLATE
const typename TREE_QUAL::bbox_type &
TREE_QUAL::node_iterator::bbox() const
{
	return m_node->bboxes[m_index];
}

TREE_TEMPLATE
void
TREE_QUAL::node_iterator::next()
{
	assert(valid());
	++m_index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
void
TREE_QUAL::depth_iterator::next()
{
	for (;;)
	{
		if (m_tos <= 0) break;

		// Copy stack top cause it may change as we use it
		const typename base_type::StackElement curTos = pop();

		if (curTos.node->level == 1) // penultimate level
		{
			// Keep walking through data while we can
			if (curTos.branchIndex + 1 < curTos.node->count)
			{
				// There is more data, just point to the next one
				push(curTos.node, curTos.branchIndex + 1);
				break;
			}

			// No more data, so it will fall back to previous level
			if (m_tos > 0) { m_stack[m_tos - 1].status = base_type::eBranchTraversed; }
		}
		else
		{
			count_type currentBranchIndex = curTos.branchIndex;
			switch (curTos.status)
			{
				case base_type::eBranchTraversed: {
					// Revisit inner branch after visiting the upper levels
					push(curTos.node, curTos.branchIndex, base_type::eNextBranch);
					return;
				}
				case base_type::eNextBranch: {
					if (curTos.branchIndex + 1 < curTos.node->count)
					{
						// Push sibling on for future tree walk
						// This is the 'fall back' node when we finish with the current level
						push(curTos.node, ++currentBranchIndex, base_type::eNormal);
					}
					else
					{
						// No more data, so it will fall back to previous level
						if (m_tos > 0)
						{
							m_stack[m_tos - 1].status = base_type::eBranchTraversed;
						}
						break;
					}
				}
				case base_type::eNormal: {
					// Since cur node is not a leaf, push first of next level to get deeper
					// into the tree
					node_ptr_type nextLevelnode = curTos.node->children[currentBranchIndex];
					push(nextLevelnode, 0, base_type::eNormal);

					while (nextLevelnode->level != 1)
					{
						nextLevelnode = nextLevelnode->children[0];
						push(nextLevelnode, 0, base_type::eNormal);
					}

					//  We pushed until traget level at TOS
					return;
				}
			}
		}
	}
}

TREE_TEMPLATE
typename TREE_QUAL::node_iterator
TREE_QUAL::depth_iterator::child()
{
	assert(m_tos > 0);
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return node_iterator(curTos.node->children[curTos.branchIndex]);
}

TREE_TEMPLATE
typename TREE_QUAL::node_iterator
TREE_QUAL::depth_iterator::current()
{
	assert(m_tos > 0);
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return node_iterator(curTos.node);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TREE_TEMPLATE
void
TREE_QUAL::leaf_iterator::next()
{
	for (;;)
	{
		if (m_tos <= 0) break;

		// Copy stack top cause it may change as we use it
		const typename base_type::StackElement curTos = pop();

		if (curTos.node->isLeaf())
		{
			// Keep walking through data while we can
			if (curTos.branchIndex + 1 < curTos.node->count)
			{
				// There is more data, just point to the next one
				push(curTos.node, curTos.branchIndex + 1);
				break;
			}
			// No more data, so it will fall back to previous level
		}
		else
		{
			if (curTos.branchIndex + 1 < curTos.node->count)
			{
				// Push sibling on for future tree walk
				// This is the 'fall back' node when we finish with the current level
				push(curTos.node, curTos.branchIndex + 1);
			}
			// Since cur node is not a leaf, push first of next level to get deeper
			// into the tree
			node_ptr_type nextLevelnode = curTos.node->children[curTos.branchIndex];
			push(nextLevelnode, 0);

			// If we pushed on a new leaf, exit as the data is ready at TOS
			if (nextLevelnode->isLeaf()) break;
		}
	}
}

#ifdef TREE_DEBUG_TAG

TREE_TEMPLATE
std::string &
TREE_QUAL::base_iterator::tag()
{
	assert(valid());
	typename base_type::StackElement &curTos = m_stack[m_tos - 1];
	return curTos.node->debugTags[curTos.branchIndex];
}

TREE_TEMPLATE
std::string &
TREE_QUAL::node_iterator::tag()
{
	assert(valid());
	return m_node->debugTags[m_index];
}

#endif

#undef TREE_TEMPLATE
#undef TREE_QUAL
} // namespace candybox

#endif // CANDYBOX_SPATIAL_HPP__
