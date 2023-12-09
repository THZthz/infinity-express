#ifndef IE_BVH_HPP
#define IE_BVH_HPP

#include <cstdint>
#include "utils/AABB.hpp"

//████████╗██████╗ ███████╗███████╗
//╚══██╔══╝██╔══██╗██╔════╝██╔════╝
//   ██║   ██████╔╝█████╗  █████╗
//   ██║   ██╔══██╗██╔══╝  ██╔══╝
//   ██║   ██║  ██║███████╗███████╗
//   ╚═╝   ╚═╝  ╚═╝╚══════╝╚══════╝
namespace ie {

//! \defgroup BVH
//! @{

/// A dynamic Box tree broad-phase, inspired by Nathanael Presson's btDbvt.
/// A dynamic tree arranges data in a binary tree to accelerate
/// queries such as volume queries and ray casts. Leafs are proxies
/// with an Box. In the tree we expand the proxy Box so that the proxy Box
/// is bigger than the client object. This allows the client
/// object to move by small amounts without triggering a tree update.
///
/// Nodes are pooled and relocatable, so we use node indices rather than pointers.
class BVH
{
public:
	/// Used to detect bad values. Positions greater than about 16km will have precision
	/// problems, so 100km as a limit should be fine in all cases.
	const float LEN_UNITS_PER_METER = 1.f;

	const float HUGE_NUMBER = 100000.0f * LEN_UNITS_PER_METER;

	/// This is used to fatten AABBs in the dynamic tree. This allows proxies
	/// to move by a small amount without triggering a tree adjustment.
	/// This is in meters.
	const float AABB_EXTENSION = 0.1f * LEN_UNITS_PER_METER;



	enum Flags : int64_t
	{
		BVH_DefaultCategory = 0x00000001,
		BVH_DefaultMask = 0xFFFFFFFF,
		BVH_DefaultGroup = 0,
		BVH_NullIndex = -1,
	};

	/// Ray-cast input data. The ray extends from p1 to p1 + maxFraction * (p2 - p1).
	struct RayCast
	{
		glm::vec2 p1, p2;
		float radius{0};
		float maxFraction{0};
	};

	/// When querying functions met overlapping Box, the responding proxy will be called
	/// with the function.
	/// \return NVG_TRUE if the query should continue.
	using TreeQueryCallback = bool (*)(int32_t proxyId, void *userData, void *context);

	/// When ray hits a Box box, this function is called.
	/// \return The fraction of the new ray.
	/// 0 to terminate raycast; less than input->maxFraction to clip the ray;
	/// input->maxFraction to continue the ray cast without clipping
	using TreeRaycastCallback =
	    float (*)(const RayCast *input, int32_t proxyId, void *userData, void *context);

public:
	struct Node
	{
		void *userData; // 8

		// Enlarged Box
		Box aabb; // 16

		// If we put the most common bits in the first 16 bits, this could be 2 bytes and
		// expanded to 0xFFFF0000 | bits. Then we get partial culling in the tree traversal.
		uint32_t categoryBits; // 4

		union
		{
			int32_t parent;
			int32_t next;
		}; // 4

		int32_t child1; // 4
		int32_t child2; // 4

		// leaf = 0, free node = -1
		// If the height is more than 32k we are in big trouble
		int16_t height; // 2

		bool moved; // 1

		Node();

		void setDefault();
	};

	struct ProxyMap
	{
		void *userData;
	};

public:
	/// Constructing the tree initializes the leaf pool.
	BVH();

	/// Destroy the tree, freeing the leaf pool.
	~BVH();

	/// Create a proxy. Provide a tight fitting Box and a userData value.
	int32_t add(Box aabb, uint32_t categoryBits, void *userData);

	/// Destroy a proxy. This asserts if the id is invalid.
	void remove(int32_t proxyId);

	/// Return true if the leaf has no more sub-leaf.
	static bool testEnd(const Node *node);

	/// Move a proxy with a swepted Box. If the proxy has moved outside of its
	/// fattened Box, then the proxy is removed from the tree and re-inserted.
	/// Otherwise the function returns immediately.
	/// \return true if the proxy was re-inserted and the moved flag was previously false
	bool move(int32_t proxyId, Box aabb1);

	/// Query an Box for overlapping proxies. The callback class
	/// is called for each proxy that overlaps the supplied Box.
	void
	queryFiltered(Box aabb, uint32_t maskBits, TreeQueryCallback callback, void *context);

	/// Query an Box for overlapping proxies. The callback class
	/// is called for each proxy that overlaps the supplied Box.
	void query(Box aabb, TreeQueryCallback callback, void *context);

	/// Ray-cast against the proxies in the tree. This relies on the callback
	/// to perform a exact ray-cast in the case were the proxy contains a shape.
	/// The callback also performs the any collision filtering. This has performance
	/// roughly equal to k * log(n), where k is the number of collisions and n is the
	/// number of proxies in the tree.
	/// \param input the ray-cast input data. The ray extends from p1 to p1 + maxFraction *
	/// (p2 - p1).
	/// \param callback a callback class that is called for each proxy that is hit by the ray.
	void raycast(
	    const RayCast *input,
	    uint32_t maskBits,
	    TreeRaycastCallback callback,
	    void *context);

	/// Validate this tree. For testing.
	void validate();

	/// Compute the height of the binary tree in O(N) time. Should not be
	/// called often.
	int32_t getHeight() const;

	/// Render the maximum balance of the tree. The balance is the difference in height of the
	/// two children of a node.
	int32_t getMaxBalance() const;

	/// Render the ratio of the sum of the node areas to the root area.
	float getAreaRatio() const;

	/// Build an optimal tree. Very expensive. For testing.
	void rebuildBottomUp();

	/// Render the number of proxies created.
	int32_t getProxyCount() const;

	/// Rebuild a the tree top down using the surface area heuristic. The provide map array
	/// must have length equal to the proxy count. This map allows you to update your proxy
	/// indices since this operation invalidates the original indices. See
	/// nvgTreeGetProxyCount.
	void rebuildTopDownSAH(ProxyMap *mapArray, int32_t mapCount);

	/// Shift the world origin. Useful for large worlds.
	/// The shift formula is: position -= newOrigin
	/// \param newOrigin the new origin with respect to the old origin
	void shiftOrigin(glm::vec2 newOrigin);

	/// Render proxy user data.
	/// \return the proxy user data or 0 if the id is invalid.
	void *getUserData(int32_t proxyId);

	bool wasMoved(int32_t proxyId);

	void clearMoved(int32_t proxyId);

	/// Render the enlarged (fat) Box for a proxy.
	Box getFatAABB(int32_t proxyId);


private:
	Node *m_nodes;
	int32_t m_root;
	int32_t m_nodeCount;
	int32_t m_nodeCapacity;
	int32_t m_freeList;
	int32_t m_proxyCount;

private:
	struct TreeBin
	{
		Box aabb;
		int32_t count;
	};

	struct TreePlane
	{
		Box leftAABB;
		Box rightAABB;
		int32_t leftCount;
		int32_t rightCount;
	};

	int32_t allocLeaf();
	void freeLeaf(int32_t nodeId);
	int32_t balanceLeaf(int32_t iA);
	void insertLeaf(int32_t leaf);
	void removeLeaf(int32_t leaf);
	int32_t computeSubTreeHeight(int32_t nodeId) const;
	int32_t computeHeight() const;
	void validateStructure(int32_t index) const;
	void validateMetrics(int32_t index) const;
	int32_t binSortBoxes(
	    int32_t parentIndex,
	    Node *leaves,
	    int32_t count,
	    TreeBin *bins,
	    TreePlane *planes);
};

inline BVH::Node::Node()
    : userData(nullptr),
      categoryBits(0),
      child1(BVH_NullIndex),
      child2(BVH_NullIndex),
      height(BVH_NullIndex),
      moved(false)
{
	parent = BVH_NullIndex;
}

inline void
BVH::Node::setDefault()
{
	userData = nullptr;
	categoryBits = 0;
	child1 = BVH_NullIndex;
	child2 = BVH_NullIndex;
	parent = BVH_NullIndex;
	height = BVH_NullIndex;
	moved = false;
}

inline bool
BVH::testEnd(const Node *node)
{
	// TODO: this should work with height == 0
	return node->child1 == BVH_NullIndex;
}

//! @}

} // namespace ie

#endif // IE_BVH_HPP
