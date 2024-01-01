/// \file libopus_interval_tree.hpp
/// \brief A red-black self-balancing interval tree C++11 header-only implementation

#ifndef CANDYBOX_INTERVAL_TREE_HPP__
#define CANDYBOX_INTERVAL_TREE_HPP__

#include <cstdint>
#include <iostream>
#include <type_traits> // std::decay, since c++11
#include <utility> // std::forward
#include <algorithm> // std::find
#include <vector>
#include <cassert>

namespace candybox {

const float RESERVE_RATE = 0.25f;

template <typename IntervalType, typename ValueType = size_t>
struct Interval final // Make sure this struct is trivial.
{
	using interval_type = typename std::decay<IntervalType>::type;
	using value_type = typename std::decay<ValueType>::type;

	template <
	    typename I,
	    typename V = ValueType,
	    typename =
	        typename std::enable_if<std::is_scalar<typename std::decay<I>::type>::value>::type,
	    typename =
	        typename std::enable_if<std::is_scalar<typename std::decay<V>::type>::value>::type>
	Interval(I a, I b, V val = {}) : low(Min(a, b)), high(Max(a, b)), value(val)
	{
	}

	template <
	    typename I,
	    typename V = ValueType,
	    typename = typename std::enable_if<
	        !std::is_scalar<typename std::decay<I>::type>::value>::type,
	    typename = typename std::enable_if<
	        !std::is_scalar<typename std::decay<V>::type>::value>::type>
	Interval(I &&a, I &&b, V &&val = {})
	    : low(std::forward<I>(a < b ? a : b)),
	      high(std::forward<I>(b < a ? a : b)),
	      value(std::forward<V>(val))
	{
	}

	template <
	    typename I,
	    typename V = ValueType,
	    typename = typename std::enable_if<
	        !std::is_scalar<typename std::decay<I>::type>::value>::type,
	    typename =
	        typename std::enable_if<std::is_scalar<typename std::decay<V>::type>::value>::type>
	Interval(I &&a, I &&b, V val = {})
	    : low(std::forward<I>(a < b ? a : b)), high(std::forward<I>(b < a ? a : b)), value(val)
	{
	}

	template <
	    typename I,
	    typename V = ValueType,
	    typename =
	        typename std::enable_if<std::is_scalar<typename std::decay<I>::type>::value>::type,
	    typename = typename std::enable_if<
	        !std::is_scalar<typename std::decay<V>::type>::value>::type>
	Interval(I a, I b, V &&val = {})
	    : low(Min(a, b)), high(Max(a, b)), value(std::forward<V>(val))
	{
	}

	template <
	    typename V = ValueType,
	    typename =
	        typename std::enable_if<std::is_scalar<typename std::decay<V>::type>::value>::type>
	explicit Interval(const std::tuple<IntervalType, IntervalType> &interval, V val = {})
	    : Interval(std::get<0>(interval), std::get<1>(interval), val)
	{
	}

	template <
	    typename V = ValueType,
	    typename = typename std::enable_if<
	        !std::is_scalar<typename std::decay<V>::type>::value>::type>
	explicit Interval(const std::tuple<IntervalType, IntervalType> &interval, V &&val = {})
	    : Interval(std::get<0>(interval), std::get<1>(interval), std::forward<V>(val))
	{
	}

	Interval() = default;
	Interval(const Interval &) = default;
	Interval(Interval &&) noexcept = default;
	Interval &operator=(const Interval &) = default;
	Interval &operator=(Interval &&) noexcept = default;
	~Interval() = default;

	bool operator==(const Interval &other) const;
	bool operator<(const Interval &other) const;

	interval_type low;
	interval_type high;
	value_type value;
};

template <typename IntervalType, typename ValueType = size_t>
class IntervalTree
{
public:
	using Interval = Interval<IntervalType, ValueType>;
	using Intervals = std::vector<Interval>;
	using size_type = std::size_t;

	IntervalTree() : m_nill(new Node()), m_root(m_nill) { }

	template <typename Container>
	explicit IntervalTree(const Container &intervals);
	template <typename ForwardIterator>
	IntervalTree(ForwardIterator begin, const ForwardIterator &end);
	IntervalTree(const IntervalTree &other);
	IntervalTree(IntervalTree &&other) noexcept;
	IntervalTree &operator=(const IntervalTree &other);
	IntervalTree &operator=(IntervalTree &&other) noexcept;
	virtual ~IntervalTree();

	void Swap(IntervalTree &other) noexcept;
	bool Insert(Interval interval);
	bool Remove(const Interval &interval);
	bool Contains(const Interval &interval) const;
	Intervals GetIntervals() const;
	void FindOverlapped(const Interval &interval, Intervals &out, bool boundary = true) const;
	Intervals FindOverlapped(const Interval &interval, bool boundary = true) const;
	void FindInner(const Interval &interval, Intervals &out, bool boundary = true) const;
	Intervals FindInner(const Interval &interval, bool boundary = true) const;
	void FindOuter(const Interval &interval, Intervals &out, bool boundary = true) const;
	Intervals FindOuter(const Interval &interval, bool boundary = true) const;
	void FindContains(const IntervalType &point, Intervals &out, bool boundary = true) const;
	Intervals FindContains(const IntervalType &point, bool boundary = true) const;
	size_type CountOverlapped(const Interval &interval, bool boundary = true) const;
	size_type CountInner(const Interval &interval, bool boundary = true) const;
	size_type CountOuter(const Interval &interval, bool boundary = true) const;
	size_type CountContains(const IntervalType &point, bool boundary = true) const;
	bool Empty() const;
	size_type Size() const;
	void Clear();

private:
	enum class Color : char
	{
		Black,
		Red
	};

	enum class Position : char
	{
		Left,
		Right
	};

	struct Appender final
	{
		template <typename Interval>
		void operator()(Interval &&interval)
		{
			intervals.emplace_back(std::forward<Interval>(interval));
		}

		Intervals &intervals;
	};

	struct Counter final
	{
		template <typename Interval>
		void operator()(Interval &&)
		{
			++count;
		}

		size_type &count;
	};

	struct HighComparator final
	{
		template <typename T>
		bool operator()(const T &lhs, const T &rhs) const
		{
			return (lhs.high < rhs.high);
		}
	};

	struct Node
	{
		using interval_type = typename Interval::interval_type;

		Node() = default;

		Node(Interval interval, Color col, Node *nill)
		    : color(col),
		      parent(nill),
		      left(nill),
		      right(nill),
		      high(interval.high),
		      lowest(interval.low),
		      highest(interval.high)
		{
			intervals.emplace_back(std::move(interval));
		}

		Color color = Color::Black;
		Node *parent = nullptr;
		Node *left = nullptr;
		Node *right = nullptr;

		interval_type high{};
		interval_type lowest{};
		interval_type highest{};
		Intervals intervals;
	};


	Node *CopySubtree(Node *otherNode, Node *otherNill, Node *parent) const;
	void DestroySubtree(Node *node) const;
	Node *FindNode(Node *node, const Interval &interval) const;
	Node *SiblingNode(Node *node) const;
	Node *ChildNode(Node *node, Position position) const;
	void SetChildNode(Node *node, Node *child, Position position) const;
	Position NodePosition(Node *node) const;
	void CreateChildNode(Node *parent, Interval interval, Position position);
	void DestroyNode(Node *node);
	void UpdateNodeLimits(Node *node) const;
	bool IsNodeAboutToBeDestroyed(Node *node) const;
	void SubtreeIntervals(Node *node, Intervals &out) const;
	template <typename Callback>
	void SubtreeOverlappingIntervals(
	    Node *node,
	    const Interval &interval,
	    bool boundary,
	    Callback &&callback) const;
	template <typename Callback>
	void SubtreeInnerIntervals(
	    Node *node,
	    const Interval &interval,
	    bool boundary,
	    Callback &&callback) const;
	template <typename Callback>
	void SubtreeOuterIntervals(
	    Node *node,
	    const Interval &interval,
	    bool boundary,
	    Callback &&callback) const;
	template <typename Callback>
	void SubtreeIntervalsContainPoint(
	    Node *node,
	    const IntervalType &point,
	    bool boundary,
	    Callback &&callback) const;
	bool IsNodeHasInterval(Node *node, const Interval &interval) const;
	IntervalType FindHighest(const Intervals &intervals) const;
	bool IsNotEqual(const IntervalType &lhs, const IntervalType &rhs) const;
	bool IsEqual(const IntervalType &lhs, const IntervalType &rhs) const;
	void SwapRelations(Node *node, Node *child);
	void RotateCommon(Node *node, Node *child) const;
	Node *RotateLeft(Node *node);
	Node *RotateRight(Node *node);
	Node *Rotate(Node *node);
	void InsertionFixNodeLimits(Node *node);
	void RemoveFixNodeLimits(Node *node, size_type minRange = 0);
	void InsertionFix(Node *node);
	void RemoveFix(Node *node);

	Node *m_nill;
	Node *m_root;
	size_type m_size{0};
};

//██╗███╗   ███╗██████╗ ██╗
//██║████╗ ████║██╔══██╗██║
//██║██╔████╔██║██████╔╝██║
//██║██║╚██╔╝██║██╔═══╝ ██║
//██║██║ ╚═╝ ██║██║     ███████╗
//╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝

template <typename IntervalType, typename ValueType>
inline bool
Interval<IntervalType, ValueType>::operator==(const Interval &other) const
{
	return !(low < other.low || other.low < low || high < other.high || other.high < high ||
	         value < other.value || other.value < value);
}

template <typename IntervalType, typename ValueType>
inline bool
Interval<IntervalType, ValueType>::operator<(const Interval &other) const
{
	return (low < other.low || high < other.high);
}

template <typename IntervalType, typename ValueType>
template <typename Container>
IntervalTree<IntervalType, ValueType>::IntervalTree(const Container &intervals)
    : IntervalTree()
{
	for (const auto &interval : intervals) { Insert(interval); }
}

template <typename IntervalType, typename ValueType>
template <typename ForwardIterator>
IntervalTree<IntervalType, ValueType>::
    IntervalTree(ForwardIterator begin, const ForwardIterator &end)
    : IntervalTree()
{
	while (begin != end)
	{
		Insert(*begin);
		++begin;
	}
}

template <typename IntervalType, typename ValueType>
IntervalTree<IntervalType, ValueType>::~IntervalTree()
{
	if (nullptr != m_root) { DestroySubtree(m_root); }

	delete m_nill;
}

template <typename IntervalType, typename ValueType>
IntervalTree<IntervalType, ValueType>::IntervalTree(const IntervalTree &other)
    : m_nill(new Node()),
      m_root(CopySubtree(other.m_root, other.m_nill, m_nill)),
      m_size(other.m_size)
{
}

template <typename IntervalType, typename ValueType>
IntervalTree<IntervalType, ValueType>::IntervalTree(IntervalTree &&other) noexcept
    : m_nill(other.m_nill), m_root(other.m_root), m_size(other.m_size)
{
	other.m_nill = nullptr;
	other.m_root = nullptr;
}

template <typename IntervalType, typename ValueType>
IntervalTree<IntervalType, ValueType> &
IntervalTree<IntervalType, ValueType>::operator=(const IntervalTree &other)
{
	if (this != &other) { IntervalTree(other).swap(*this); }

	return *this;
}

template <typename IntervalType, typename ValueType>
IntervalTree<IntervalType, ValueType> &
IntervalTree<IntervalType, ValueType>::operator=(IntervalTree &&other) noexcept
{
	other.swap(*this);
	return *this;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::Swap(IntervalTree &other) noexcept
{
	std::swap(m_nill, other.m_nill);
	std::swap(m_root, other.m_root);
	std::swap(m_size, other.m_size);
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::Insert(Interval interval)
{
	assert(nullptr != m_root && nullptr != m_nill);

	if (m_root == m_nill)
	{
		// Tree is empty
		assert(0 == m_size);
		m_root = new Node(std::move(interval), Color::Black, m_nill);
		m_size = 1;
		return true;
	}

	Node *node = FindNode(m_root, interval);
	assert(node != m_nill);

	if (interval.low < node->intervals.front().low)
	{
		CreateChildNode(node, std::move(interval), Position::Left);
		return true;
	}

	if (node->intervals.front().low < interval.low)
	{
		CreateChildNode(node, std::move(interval), Position::Right);
		return true;
	}

	if (!IsNodeHasInterval(node, interval))
	{
		auto it = std::lower_bound(
		    node->intervals.begin(), node->intervals.end(), interval, HighComparator());

		if (node->high < interval.high) { node->high = interval.high; }

		if (node->highest < node->high)
		{
			node->highest = node->high;
			InsertionFixNodeLimits(node);
		}

		node->intervals.emplace(it, std::move(interval));
		++m_size;

		return true;
	}

	// Value already exists
	return false;
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::Remove(const Interval &interval)
{
	assert(nullptr != m_root && nullptr != m_nill);

	if (m_root == m_nill)
	{
		// Tree is empty
		assert(0 == m_size);
		return false;
	}

	auto node = FindNode(m_root, interval);
	assert(node != m_nill);

	auto it = std::find(node->intervals.begin(), node->intervals.end(), interval);
	if (it != node->intervals.cend())
	{
		node->intervals.erase(it);
		if (IsNodeAboutToBeDestroyed(node))
		{
			auto child = m_nill;
			if (node->right == m_nill) { child = node->left; }
			else if (node->left == m_nill) { child = node->right; }
			else
			{
				auto nextValueNode = node->right;
				while (nextValueNode->left != m_nill) { nextValueNode = nextValueNode->left; }
				node->intervals = std::move(nextValueNode->intervals);
				node->high = std::move(nextValueNode->high);
				removeFixNodeLimits(node);
				node = nextValueNode;
				child = nextValueNode->right;
			}

			if (child == m_nill && node->parent == m_nill)
			{
				// Node is root without children
				SwapRelations(node, child);
				DestroyNode(node);
				return true;
			}

			if (Color::Red == node->color || Color::Red == child->color)
			{
				SwapRelations(node, child);
				if (Color::Red == child->color) { child->color = Color::Black; }
			}
			else
			{
				assert(Color::Black == node->color);

				if (child == m_nill) { child = node; }
				else { SwapRelations(node, child); }

				RemoveFix(child);

				if (node->parent != m_nill)
				{
					SetChildNode(node->parent, m_nill, NodePosition(node));
				}
			}

			if (node->parent != m_nill) { removeFixNodeLimits(node->parent, 1); }
			DestroyNode(node);
		}
		else
		{
			if (IsEqual(interval.high, node->high))
			{
				node->high = FindHighest(node->intervals);
			}

			if (IsEqual(interval.high, node->highest)) { RemoveFixNodeLimits(node); }

			--m_size;
		}

		return true;
	}

	// Value not found
	return false;
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::Contains(const Interval &interval) const
{
	assert(nullptr != m_root && nullptr != m_nill);

	if (m_root == m_nill)
	{
		// Tree is empty
		assert(0 == m_size);
		return false;
	}

	auto node = findNode(m_root, interval);
	assert(node != m_nill);

	return isNodeHasInterval(node, interval);
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Intervals
IntervalTree<IntervalType, ValueType>::GetIntervals() const
{
	Intervals out;
	out.reserve(m_size);

	if (m_root != m_nill) { SubtreeIntervals(m_root, out); }

	return out;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::
    FindOverlapped(const Interval &interval, Intervals &out, bool boundary) const
{
	if (!out.empty()) { out.clear(); }

	if (m_root != m_nill)
	{
		SubtreeOverlappingIntervals(m_root, interval, boundary, Appender{out});
	}

	out.shrink_to_fit();
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Intervals
IntervalTree<IntervalType, ValueType>::FindOverlapped(const Interval &interval, bool boundary)
    const
{
	Intervals out;
	out.reserve(size_t(m_size * RESERVE_RATE));
	FindOverlapped(interval, out, boundary);
	return out;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::
    FindInner(const Interval &interval, Intervals &out, bool boundary) const
{
	if (!out.empty()) { out.clear(); }

	if (m_root != m_nill) { SubtreeInnerIntervals(m_root, interval, boundary, Appender{out}); }

	out.shrink_to_fit();
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Intervals
IntervalTree<IntervalType, ValueType>::FindInner(const Interval &interval, bool boundary) const
{
	Intervals out;
	out.reserve(size_t(m_size * RESERVE_RATE));
	FindInner(interval, out, boundary);
	return out;
}


template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::
    FindOuter(const Interval &interval, Intervals &out, bool boundary) const
{
	if (!out.empty()) { out.clear(); }

	if (m_root != m_nill) { SubtreeOuterIntervals(m_root, interval, boundary, Appender{out}); }

	out.shrink_to_fit();
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Intervals
IntervalTree<IntervalType, ValueType>::FindOuter(const Interval &interval, bool boundary) const
{
	Intervals out;
	out.reserve(size_t(m_size * RESERVE_RATE));
	FindOuter(interval, out, boundary);
	return out;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::
    FindContains(const IntervalType &point, Intervals &out, bool boundary) const
{
	if (!out.empty()) { out.clear(); }

	if (m_root != m_nill)
	{
		SubtreeIntervalsContainPoint(m_root, point, boundary, Appender{out});
	}

	out.shrink_to_fit();
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Intervals
IntervalTree<IntervalType, ValueType>::FindContains(const IntervalType &point, bool boundary)
    const
{
	Intervals out;
	out.reserve(size_t(m_size * RESERVE_RATE));
	FindContains(point, out, boundary);
	return out;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::size_type
IntervalTree<IntervalType, ValueType>::CountOverlapped(const Interval &interval, bool boundary)
    const
{
	size_type count = 0;

	if (m_root != m_nill)
	{
		SubtreeOverlappingIntervals(m_root, interval, boundary, Counter{count});
	}

	return count;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::size_type
IntervalTree<IntervalType, ValueType>::CountInner(const Interval &interval, bool boundary)
    const
{
	size_type count = 0;

	if (m_root != m_nill)
	{
		SubtreeInnerIntervals(m_root, interval, boundary, Counter{count});
	}

	return count;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::size_type
IntervalTree<IntervalType, ValueType>::CountOuter(const Interval &interval, bool boundary)
    const
{
	size_type count = 0;

	if (m_root != m_nill)
	{
		SubtreeOuterIntervals(m_root, interval, boundary, Counter{count});
	}

	return count;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::size_type
IntervalTree<IntervalType, ValueType>::CountContains(const IntervalType &point, bool boundary)
    const
{
	size_type count = 0;

	if (m_root != m_nill)
	{
		SubtreeIntervalsContainPoint(m_root, point, boundary, Counter{count});
	}

	return count;
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::Empty() const
{
	return (0 == m_size);
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::size_type
IntervalTree<IntervalType, ValueType>::Size() const
{
	return m_size;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::Clear()
{
	assert(nullptr != m_root && nullptr != m_nill);

	DestroySubtree(m_root);
	m_root = m_nill;
	m_size = 0;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::
    CopySubtree(Node *otherNode, Node *otherNill, Node *parent) const
{
	assert(nullptr != otherNode && nullptr != otherNill && nullptr != parent);

	if (otherNode == otherNill) { return m_nill; }

	auto node = new Node();
	node->intervals = otherNode->intervals;
	node->high = otherNode->high;
	node->lowest = otherNode->lowest;
	node->highest = otherNode->highest;
	node->color = otherNode->color;
	node->parent = parent;
	node->left = CopySubtree(otherNode->left, otherNill, node);
	node->right = CopySubtree(otherNode->right, otherNill, node);

	return node;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::DestroySubtree(Node *node) const
{
	assert(nullptr != node);

	if (node == m_nill) { return; }

	DestroySubtree(node->left);
	DestroySubtree(node->right);

	delete node;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::FindNode(Node *node, const Interval &interval) const
{
	assert(nullptr != node);
	assert(node != m_nill);

	auto child = m_nill;
	if (interval.low < node->intervals.front().low)
	{
		child = ChildNode(node, Position::Left);
	}
	else if (node->intervals.front().low < interval.low)
	{
		child = ChildNode(node, Position::Right);
	}
	else { return node; }

	return (child == m_nill) ? node : FindNode(child, interval);
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::SiblingNode(Node *node) const
{
	assert(nullptr != node);

	return (Position::Left == NodePosition(node))
	           ? ChildNode(node->parent, Position::Right)
	           : ChildNode(node->parent, Position::Left);
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::ChildNode(Node *node, Position position) const
{
	assert(nullptr != node);

	switch (position)
	{
		case Position::Left: return node->left;
		case Position::Right: return node->right;
		default: assert(false); return nullptr;
	}
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::SetChildNode(Node *node, Node *child, Position position)
    const
{
	assert(nullptr != node && nullptr != child);
	assert(node != m_nill);

	switch (position)
	{
		case Position::Left: node->left = child; break;
		case Position::Right: node->right = child; break;
		default: assert(false); break;
	}

	if (child != m_nill) { child->parent = node; }
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Position
IntervalTree<IntervalType, ValueType>::NodePosition(Node *node) const
{
	assert(nullptr != node && nullptr != node->parent);

	return (node->parent->left == node) ? Position::Left : Position::Right;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::
    CreateChildNode(Node *parent, Interval interval, Position position)
{
	assert(nullptr != parent);
	assert(ChildNode(parent, position) == m_nill);

	auto child = new Node(std::move(interval), Color::Red, m_nill);
	SetChildNode(parent, child, position);
	InsertionFixNodeLimits(child);
	InsertionFix(child);
	++m_size;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::DestroyNode(Node *node)
{
	--m_size;
	delete node;
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::UpdateNodeLimits(Node *node) const
{
	assert(nullptr != node);

	auto left = IsNodeAboutToBeDestroyed(node->left) ? m_nill : node->left;
	auto right = IsNodeAboutToBeDestroyed(node->right) ? m_nill : node->right;

	const auto &lowest = (left != m_nill) ? left->lowest : node->intervals.front().low;

	if (IsNotEqual(node->lowest, lowest)) { node->lowest = lowest; }

	const auto &highest = Max(left->highest, right->highest, node->high);

	if (IsNotEqual(node->highest, highest)) { node->highest = highest; }
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::IsNodeAboutToBeDestroyed(Node *node) const
{
	assert(nullptr != node);

	return (node != m_nill && node->intervals.empty());
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::SubtreeIntervals(Node *node, Intervals &out) const
{
	assert(nullptr != node);

	if (node == m_nill) { return; }

	SubtreeIntervals(node->left, out);

	out.insert(out.end(), node->intervals.begin(), node->intervals.end());

	SubtreeIntervals(node->right, out);
}

template <typename IntervalType, typename ValueType>
template <typename Callback>
void
IntervalTree<IntervalType, ValueType>::SubtreeOverlappingIntervals(
    Node *node,
    const Interval &interval,
    bool boundary,
    Callback &&callback) const
{
	assert(nullptr != node);

	if (node == m_nill) { return; }

	if (node->left != m_nill &&
	    (boundary ? !(node->left->highest < interval.low)
	              : interval.low < node->left->highest))
	{
		SubtreeOverlappingIntervals(node->left, interval, boundary, callback);
	}

	if (boundary ? !(interval.high < node->intervals.front().low)
	             : node->intervals.front().low < interval.high)
	{
		for (auto it = node->intervals.rbegin(); it != node->intervals.rend(); ++it)
		{
			if (boundary ? !(it->high < interval.low) : interval.low < it->high)
			{
				callback(*it);
			}
			else { break; }
		}

		SubtreeOverlappingIntervals(
		    node->right, interval, boundary, std::forward<Callback>(callback));
	}
}

template <typename IntervalType, typename ValueType>
template <typename Callback>
void
IntervalTree<IntervalType, ValueType>::SubtreeInnerIntervals(
    Node *node,
    const Interval &interval,
    bool boundary,
    Callback &&callback) const
{
	assert(nullptr != node);

	if (node == m_nill) { return; }

	if (boundary ? !(node->intervals.front().low < interval.low)
	             : interval.low < node->intervals.front().low)
	{
		SubtreeInnerIntervals(node->left, interval, boundary, callback);
		for (auto it = node->intervals.begin(); it != node->intervals.end(); ++it)
		{
			if (boundary ? !(interval.high < it->high) : it->high < interval.high)
			{
				callback(*it);
			}
			else { break; }
		}
	}

	if (node->right != m_nill &&
	    (boundary ? !(interval.high < node->right->lowest)
	              : node->right->lowest < interval.high))
	{
		SubtreeInnerIntervals(
		    node->right, interval, boundary, std::forward<Callback>(callback));
	}
}

template <typename IntervalType, typename ValueType>
template <typename Callback>
void
IntervalTree<IntervalType, ValueType>::SubtreeOuterIntervals(
    Node *node,
    const Interval &interval,
    bool boundary,
    Callback &&callback) const
{
	assert(nullptr != node);

	if (node == m_nill) { return; }

	if (node->left != m_nill &&
	    (boundary ? !(node->left->highest < interval.high)
	              : interval.high < node->left->highest))
	{
		SubtreeOuterIntervals(node->left, interval, boundary, callback);
	}

	if (boundary ? !(interval.low < node->intervals.front().low)
	             : node->intervals.front().low < interval.low)
	{
		for (auto it = node->intervals.rbegin(); it != node->intervals.rend(); ++it)
		{
			if (boundary ? !(it->high < interval.high) : interval.high < it->high)
			{
				callback(*it);
			}
			else { break; }
		}

		SubtreeOuterIntervals(
		    node->right, interval, boundary, std::forward<Callback>(callback));
	}
}

template <typename IntervalType, typename ValueType>
template <typename Callback>
void
IntervalTree<IntervalType, ValueType>::SubtreeIntervalsContainPoint(
    Node *node,
    const IntervalType &point,
    bool boundary,
    Callback &&callback) const
{
	assert(nullptr != node);

	if (node == m_nill) { return; }

	if (node->left != m_nill &&
	    (boundary ? !(node->left->highest < point) : point < node->left->highest))
	{
		SubtreeIntervalsContainPoint(node->left, point, boundary, callback);
	}

	if (boundary ? !(point < node->intervals.front().low)
	             : node->intervals.front().low < point)
	{
		for (auto it = node->intervals.rbegin(); it != node->intervals.rend(); ++it)
		{
			if (boundary ? !(it->high < point) : point < it->high) { callback(*it); }
			else { break; }
		}

		SubtreeIntervalsContainPoint(
		    node->right, point, boundary, std::forward<Callback>(callback));
	}
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::IsNodeHasInterval(Node *node, const Interval &interval)
    const
{
	assert(nullptr != node);

	return (node->intervals.cend() !=
	        std::find(node->intervals.cbegin(), node->intervals.cend(), interval));
}

template <typename IntervalType, typename ValueType>
IntervalType
IntervalTree<IntervalType, ValueType>::FindHighest(const Intervals &intervals) const
{
	assert(!intervals.empty());

	auto it = std::max_element(intervals.cbegin(), intervals.cend(), HighComparator());

	assert(it != intervals.cend());

	return it->high;
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::
    IsNotEqual(const IntervalType &lhs, const IntervalType &rhs) const
{
	return (lhs < rhs || rhs < lhs);
}

template <typename IntervalType, typename ValueType>
bool
IntervalTree<IntervalType, ValueType>::
    IsEqual(const IntervalType &lhs, const IntervalType &rhs) const
{
	return !IsNotEqual(lhs, rhs);
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::SwapRelations(Node *node, Node *child)
{
	assert(nullptr != node && nullptr != child);

	if (node->parent == m_nill)
	{
		if (child != m_nill) { child->parent = m_nill; }
		m_root = child;
	}
	else { SetChildNode(node->parent, child, NodePosition(node)); }
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::RotateCommon(Node *node, Node *child) const
{
	assert(nullptr != node && nullptr != child);
	assert(node != m_nill && child != m_nill);

	std::swap(node->color, child->color);

	UpdateNodeLimits(node);

	if (child->highest < node->highest) { child->highest = node->highest; }

	if (node->lowest < child->lowest) { child->lowest = node->lowest; }
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::RotateLeft(Node *node)
{
	assert(nullptr != node && nullptr != node->right);

	auto child = node->right;
	SwapRelations(node, child);
	SetChildNode(node, child->left, Position::Right);
	SetChildNode(child, node, Position::Left);
	RotateCommon(node, child);

	return child;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::RotateRight(Node *node)
{
	assert(nullptr != node && nullptr != node->left);

	auto child = node->left;
	SwapRelations(node, child);
	SetChildNode(node, child->right, Position::Left);
	SetChildNode(child, node, Position::Right);
	RotateCommon(node, child);

	return child;
}

template <typename IntervalType, typename ValueType>
typename IntervalTree<IntervalType, ValueType>::Node *
IntervalTree<IntervalType, ValueType>::Rotate(Node *node)
{
	assert(nullptr != node);

	switch (NodePosition(node))
	{
		case Position::Left: return RotateRight(node->parent);
		case Position::Right: return RotateLeft(node->parent);
		default: assert(false); return nullptr;
	}
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::InsertionFixNodeLimits(Node *node)
{
	assert(nullptr != node && nullptr != node->parent);

	while (node->parent != m_nill)
	{
		auto finish = true;

		if (node->parent->highest < node->highest)
		{
			node->parent->highest = node->highest;
			finish = false;
		}

		if (node->lowest < node->parent->lowest)
		{
			node->parent->lowest = node->lowest;
			finish = false;
		}

		if (finish) { break; }

		node = node->parent;
	}
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::RemoveFixNodeLimits(Node *node, size_type minRange)
{
	assert(nullptr != node && nullptr != node->parent);

	size_type range = 0;
	while (node != m_nill)
	{
		bool finish = (minRange < range);

		UpdateNodeLimits(node);

		if (IsNotEqual(node->highest, node->parent->highest)) { finish = false; }

		if (IsNotEqual(node->lowest, node->parent->lowest)) { finish = false; }

		if (finish) { break; }

		node = node->parent;
		++range;
	}
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::InsertionFix(Node *node)
{
	assert(nullptr != node && nullptr != node->parent);

	while (Color::Red == node->color && Color::Red == node->parent->color)
	{
		auto parent = node->parent;
		auto uncle = SiblingNode(parent);
		switch (uncle->color)
		{
			case Color::Red:
				uncle->color = Color::Black;
				parent->color = Color::Black;
				parent->parent->color = Color::Red;
				node = parent->parent;
				break;
			case Color::Black:
				if (NodePosition(node) != NodePosition(parent)) { parent = Rotate(node); }
				node = Rotate(parent);
				break;
			default: assert(false); break;
		}
	}

	if (node->parent == m_nill && Color::Black != node->color) { node->color = Color::Black; }
}

template <typename IntervalType, typename ValueType>
void
IntervalTree<IntervalType, ValueType>::RemoveFix(Node *node)
{
	assert(nullptr != node && nullptr != node->parent);

	while (Color::Black == node->color && node->parent != m_nill)
	{
		auto sibling = SiblingNode(node);
		if (Color::Red == sibling->color)
		{
			Rotate(sibling);
			sibling = SiblingNode(node);
		}

		assert(nullptr != sibling && nullptr != sibling->left && nullptr != sibling->right);
		assert(Color::Black == sibling->color);

		if (Color::Black == sibling->left->color && Color::Black == sibling->right->color)
		{
			sibling->color = Color::Red;
			node = node->parent;
		}
		else
		{
			if (Position::Left == NodePosition(sibling) &&
			    Color::Black == sibling->left->color)
			{
				sibling = RotateLeft(sibling);
			}
			else if (Position::Right == NodePosition(sibling) &&
			         Color::Black == sibling->right->color)
			{
				sibling = RotateRight(sibling);
			}
			Rotate(sibling);
			node = SiblingNode(node->parent);
		}
	}

	if (Color::Black != node->color) { node->color = Color::Black; }
}

template <typename IntervalType, typename ValueType>
void
Swap(IntervalTree<IntervalType, ValueType> &lhs, IntervalTree<IntervalType, ValueType> &rhs)
{
	lhs.Swap(rhs);
}


} // namespace candybox

template <typename IntervalType, typename ValueType>
inline std::ostream &
operator<<(std::ostream &out, const candybox::Interval<IntervalType, ValueType> &interval)
{
	out << "Interval(" << interval.low << ", " << interval.high << ")";
	if (interval.value != ValueType{}) { out << ": " << interval.value; }
	return out;
}

template <typename IntervalType, typename ValueType>
inline std::ostream &
operator<<(std::ostream &out, const candybox::IntervalTree<IntervalType, ValueType> &tree)
{
	out << "IntervalTree(" << tree.size() << ")";
	return out;
}

#endif // CANDYBOX_INTERVAL_TREE_HPP__

#if 0
int IntervalTreeExample()
{
	using namespace candybox;

	// Create an interval tree
    IntervalTree<int> tree;

    // Insert intervals to the tree
    tree.Insert({20, 30});
    tree.Insert({40, 60});
    tree.Insert({70, 90});
    tree.Insert({60, 70});
    tree.Insert({40, 90});
    tree.Insert({80, 90});

    // Wanted interval and point
    Interval<int> wantedInterval(50, 80);
    auto wantedPoint = 50;

    // Find intervals
    const auto &overlappingIntervals = tree.FindOverlapped(wantedInterval);
    const auto &innerIntervals = tree.FindInner(wantedInterval);
    const auto &outerIntervals = tree.FindOuter(wantedInterval);
    const auto &intervalsContainPoint = tree.FindContains(wantedPoint);

    // Print all intervals
    std::cout << "All intervals:" << std::endl;
    for (const auto &interval : tree.GetIntervals()) {
        std::cout << interval << std::endl;
    }
    std::cout << std::endl;

    // Print overlapping intervals
    std::cout << "Overlapping intervals for " << wantedInterval << ":" << std::endl;
    for (const auto &interval : overlappingIntervals) {
        std::cout << interval << std::endl;
    }
    std::cout << std::endl;

    // Print inner intervals
    std::cout << "Inner intervals for " << wantedInterval << ":" << std::endl;
    for (const auto &interval : innerIntervals) {
        std::cout << interval << std::endl;
    }
    std::cout << std::endl;

    // Print outer intervals
    std::cout << "Outer intervals for " << wantedInterval << ":" << std::endl;
    for (const auto &interval : outerIntervals) {
        std::cout << interval << std::endl;
    }
    std::cout << std::endl;

    // Print intervals contain the point
    std::cout << "Intervals contain the point with the value "
              << wantedPoint << ":" << std::endl;
    for (const auto &interval : intervalsContainPoint) {
        std::cout << interval << std::endl;
    }

    return 0;
}
#endif

