#include <cstring>
#include <cfloat>

#include "candybox/AABB.hpp"
#include "candybox/Memory.hpp"
#include "candybox/BVH.hpp"
#include "candybox/Linear.hpp"

#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

namespace candybox {

BVH::BVH()
{
	m_root = BVH_NullIndex;

	m_nodeCapacity = 16;
	m_nodeCount = 0;
	m_nodes = (Node*)_malloc(m_nodeCapacity * sizeof(Node));
	memset(m_nodes, 0, m_nodeCapacity * sizeof(Node));

	// Build a linked list for the free list.
	for (int32_t i = 0; i < m_nodeCapacity - 1; ++i)
	{
		m_nodes[i].next = i + 1;
		m_nodes[i].height = -1;
	}
	m_nodes[m_nodeCapacity - 1].next = BVH_NullIndex;
	m_nodes[m_nodeCapacity - 1].height = -1;
	m_freeList = 0;

	m_proxyCount = 0;
}

BVH::~BVH() { _free(m_nodes); }

// Allocate a node from the pool. Grow the pool if necessary.
int32_t
BVH::allocLeaf()
{
	// Expand the node pool as needed.
	if (m_freeList == BVH_NullIndex)
	{
		assert(m_nodeCount == m_nodeCapacity);

		// The free list is empty. Rebuild a bigger pool.
		Node* oldNodes = m_nodes;
		int32_t oldCapcity = m_nodeCapacity;
		m_nodeCapacity += oldCapcity >> 1;
		m_nodes = (Node*)_malloc(m_nodeCapacity * sizeof(Node));
		memcpy(m_nodes, oldNodes, m_nodeCount * sizeof(Node));
		_free(oldNodes); //, oldCapcity * sizeof(Node));

		// Build a linked list for the free list. The parent
		// pointer becomes the "next" pointer.
		for (int32_t i = m_nodeCount; i < m_nodeCapacity - 1; ++i)
		{
			m_nodes[i].next = i + 1;
			m_nodes[i].height = -1;
		}
		m_nodes[m_nodeCapacity - 1].next = BVH_NullIndex;
		m_nodes[m_nodeCapacity - 1].height = -1;
		m_freeList = m_nodeCount;
	}

	// Peel a node off the free list.
	int32_t nodeId = m_freeList;
	Node* node = m_nodes + nodeId;
	m_freeList = node->next;
	node->setDefault();
	++m_nodeCount;
	return nodeId;
}

// Return a node to the pool.
void
BVH::freeLeaf(int32_t nodeId)
{
	assert(0 <= nodeId && nodeId < m_nodeCapacity);
	assert(0 < m_nodeCount);
	m_nodes[nodeId].next = m_freeList;
	m_nodes[nodeId].height = -1;
	m_freeList = nodeId;
	--m_nodeCount;
}

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
int32_t
BVH::balanceLeaf(int32_t iA)
{
	assert(iA != BVH_NullIndex);

	Node* A = m_nodes + iA;
	if (testEnd(A) || A->height < 2) { return iA; }

	int32_t iB = A->child1;
	int32_t iC = A->child2;
	assert(0 <= iB && iB < m_nodeCapacity);
	assert(0 <= iC && iC < m_nodeCapacity);

	Node* B = m_nodes + iB;
	Node* C = m_nodes + iC;

	int32_t balance = C->height - B->height;

	// Rotate C up
	if (balance > 1)
	{
		int32_t iF = C->child1;
		int32_t iG = C->child2;
		Node* F = m_nodes + iF;
		Node* G = m_nodes + iG;
		assert(0 <= iF && iF < m_nodeCapacity);
		assert(0 <= iG && iG < m_nodeCapacity);

		// Swap A and C
		C->child1 = iA;
		C->parent = A->parent;
		A->parent = iC;

		// A's old parent should point to C
		if (C->parent != BVH_NullIndex)
		{
			if (m_nodes[C->parent].child1 == iA) { m_nodes[C->parent].child1 = iC; }
			else
			{
				assert(m_nodes[C->parent].child2 == iA);
				m_nodes[C->parent].child2 = iC;
			}
		}
		else { m_root = iC; }

		// Rotate
		if (F->height > G->height)
		{
			C->child2 = iF;
			A->child2 = iG;
			G->parent = iA;
			A->aabb = B->aabb.combine(G->aabb);
			C->aabb = A->aabb.combine(F->aabb);

			A->categoryBits = B->categoryBits | G->categoryBits;
			C->categoryBits = A->categoryBits | F->categoryBits;

			A->height = 1 + std::max(B->height, G->height);
			C->height = 1 + std::max(A->height, F->height);
		}
		else
		{
			C->child2 = iG;
			A->child2 = iF;
			F->parent = iA;
			A->aabb = B->aabb.combine(F->aabb);
			C->aabb = A->aabb.combine(G->aabb);

			A->categoryBits = B->categoryBits | F->categoryBits;
			C->categoryBits = A->categoryBits | G->categoryBits;

			A->height = 1 + std::max(B->height, F->height);
			C->height = 1 + std::max(A->height, G->height);
		}

		return iC;
	}

	// Rotate B up
	if (balance < -1)
	{
		int32_t iD = B->child1;
		int32_t iE = B->child2;
		Node* D = m_nodes + iD;
		Node* E = m_nodes + iE;
		assert(0 <= iD && iD < m_nodeCapacity);
		assert(0 <= iE && iE < m_nodeCapacity);

		// Swap A and B
		B->child1 = iA;
		B->parent = A->parent;
		A->parent = iB;

		// A's old parent should point to B
		if (B->parent != BVH_NullIndex)
		{
			if (m_nodes[B->parent].child1 == iA) { m_nodes[B->parent].child1 = iB; }
			else
			{
				assert(m_nodes[B->parent].child2 == iA);
				m_nodes[B->parent].child2 = iB;
			}
		}
		else { m_root = iB; }

		// Rotate
		if (D->height > E->height)
		{
			B->child2 = iD;
			A->child1 = iE;
			E->parent = iA;
			A->aabb = C->aabb.combine(E->aabb);
			B->aabb = A->aabb.combine(D->aabb);

			A->categoryBits = C->categoryBits | E->categoryBits;
			B->categoryBits = A->categoryBits | D->categoryBits;

			A->height = 1 + std::max(C->height, E->height);
			B->height = 1 + std::max(A->height, D->height);
		}
		else
		{
			B->child2 = iE;
			A->child1 = iD;
			D->parent = iA;
			A->aabb = C->aabb.combine(D->aabb);
			B->aabb = A->aabb.combine(E->aabb);

			A->categoryBits = C->categoryBits | D->categoryBits;
			B->categoryBits = A->categoryBits | E->categoryBits;

			A->height = 1 + std::max(C->height, D->height);
			B->height = 1 + std::max(A->height, E->height);
		}

		return iB;
	}

	return iA;
}

void
BVH::insertLeaf(int32_t leaf)
{
	if (m_root == BVH_NullIndex)
	{
		m_root = leaf;
		m_nodes[m_root].parent = BVH_NullIndex;
		return;
	}

	// Find the best sibling for this node
	Box leafAABB = m_nodes[leaf].aabb;
	int32_t index = m_root;
	while (!testEnd(m_nodes + index))
	{
		int32_t child1 = m_nodes[index].child1;
		int32_t child2 = m_nodes[index].child2;

		float area = m_nodes[index].aabb.perimeter();

		Box combinedAABB{m_nodes[index].aabb.combine(leafAABB)};
		float combinedArea = combinedAABB.perimeter();

		// Cost of creating a new parent for this node and the new leaf
		float cost = 2.0f * combinedArea;

		// Minimum cost of pushing the leaf further down the tree
		float inheritanceCost = 2.0f * (combinedArea - area);

		// Cost of descending into child1
		float cost1;
		if (testEnd(m_nodes + child1))
		{
			Box aabb{leafAABB.combine(m_nodes[child1].aabb)};
			cost1 = aabb.perimeter() + inheritanceCost;
		}
		else
		{
			Box aabb{leafAABB.combine(m_nodes[child1].aabb)};
			float oldArea = m_nodes[child1].aabb.perimeter();
			float newArea = aabb.perimeter();
			cost1 = (newArea - oldArea) + inheritanceCost;
		}

		// Cost of descending into child2
		float cost2;
		if (testEnd(m_nodes + child2))
		{
			Box aabb{leafAABB.combine(m_nodes[child2].aabb)};
			cost2 = aabb.perimeter() + inheritanceCost;
		}
		else
		{
			Box aabb{leafAABB.combine(m_nodes[child2].aabb)};
			float oldArea = m_nodes[child2].aabb.perimeter();
			float newArea = aabb.perimeter();
			cost2 = newArea - oldArea + inheritanceCost;
		}

		// Descend according to the minimum cost.
		if (cost < cost1 && cost < cost2) { break; }

		// Descend
		if (cost1 < cost2) { index = child1; }
		else { index = child2; }
	}

	int32_t sibling = index;

	// Create a new parent.
	int32_t oldParent = m_nodes[sibling].parent;
	int32_t newParent = allocLeaf();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].userData = nullptr;
	m_nodes[newParent].aabb = leafAABB.combine(m_nodes[sibling].aabb);
	m_nodes[newParent]
	    .categoryBits = m_nodes[leaf].categoryBits | m_nodes[sibling].categoryBits;
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != BVH_NullIndex)
	{
		// The sibling was not the root.
		if (m_nodes[oldParent].child1 == sibling) { m_nodes[oldParent].child1 = newParent; }
		else { m_nodes[oldParent].child2 = newParent; }

		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
		m_root = newParent;
	}

	// Walk back up the tree fixing heights and AABBs
	index = m_nodes[leaf].parent;
	while (index != BVH_NullIndex)
	{
		index = balanceLeaf(index);

		int32_t child1 = m_nodes[index].child1;
		int32_t child2 = m_nodes[index].child2;

		assert(child1 != BVH_NullIndex);
		assert(child2 != BVH_NullIndex);

		m_nodes[index].aabb = m_nodes[child1].aabb.combine(m_nodes[child2].aabb);
		m_nodes[index]
		    .categoryBits = m_nodes[child1].categoryBits | m_nodes[child2].categoryBits;
		m_nodes[index].height = 1 + std::max(m_nodes[child1].height, m_nodes[child2].height);

		index = m_nodes[index].parent;
	}

	// validate();
}

void
BVH::removeLeaf(int32_t leaf)
{
	if (leaf == m_root)
	{
		m_root = BVH_NullIndex;
		return;
	}

	int32_t parent = m_nodes[leaf].parent;
	int32_t grandParent = m_nodes[parent].parent;
	int32_t sibling;
	if (m_nodes[parent].child1 == leaf) { sibling = m_nodes[parent].child2; }
	else { sibling = m_nodes[parent].child1; }

	if (grandParent != BVH_NullIndex)
	{
		// Destroy parent and connect sibling to grandParent.
		if (m_nodes[grandParent].child1 == parent) { m_nodes[grandParent].child1 = sibling; }
		else { m_nodes[grandParent].child2 = sibling; }
		m_nodes[sibling].parent = grandParent;
		freeLeaf(parent);

		// Adjust ancestor bounds.
		int32_t index = grandParent;
		while (index != BVH_NullIndex)
		{
			index = balanceLeaf(index);

			int32_t child1 = m_nodes[index].child1;
			int32_t child2 = m_nodes[index].child2;

			m_nodes[index].aabb = m_nodes[child1].aabb.combine(m_nodes[child2].aabb);
			m_nodes[index]
			    .categoryBits = m_nodes[child1].categoryBits | m_nodes[child2].categoryBits;
			m_nodes[index]
			    .height = 1 + std::max(m_nodes[child1].height, m_nodes[child2].height);

			index = m_nodes[index].parent;
		}
	}
	else
	{
		m_root = sibling;
		m_nodes[sibling].parent = BVH_NullIndex;
		freeLeaf(parent);
	}

	// validate();
}

// Create a proxy in the tree as a leaf node. We return the index
// of the node instead of a pointer so that we can grow
// the node pool.
int32_t
BVH::add(Box aabb, uint32_t categoryBits, void* userData)
{
	assert(-HUGE_NUMBER < aabb.l.x && aabb.l.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.l.y && aabb.l.y < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.x && aabb.u.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.y && aabb.u.y < HUGE_NUMBER);

	int32_t proxyId = allocLeaf();
	Node* node = m_nodes + proxyId;

	// Fatten the aabb.
	glm::vec2 r = {AABB_EXTENSION, AABB_EXTENSION};
	node->aabb.l = m::subv(aabb.l, r);
	node->aabb.u = m::addv(aabb.u, r);
	node->userData = userData;
	node->categoryBits = categoryBits;
	node->height = 0;
	node->moved = true;

	insertLeaf(proxyId);

	m_proxyCount += 1;

	return proxyId;
}

void
BVH::remove(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	assert(testEnd(m_nodes + proxyId));

	removeLeaf(proxyId);
	freeLeaf(proxyId);

	assert(m_proxyCount > 0);
	m_proxyCount -= 1;
}

bool
BVH::move(int32_t proxyId, Box aabb)
{
	assert(-HUGE_NUMBER < aabb.l.x && aabb.l.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.l.y && aabb.l.y < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.x && aabb.u.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.y && aabb.u.y < HUGE_NUMBER);

	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	assert(testEnd(m_nodes + proxyId));

	// Extend Box
	Box fatAABB;
	glm::vec2 r = {AABB_EXTENSION, AABB_EXTENSION};
	fatAABB.l = m::subv(aabb.l, r);
	fatAABB.u = m::addv(aabb.u, r);

	Box treeAABB = m_nodes[proxyId].aabb;
	if (treeAABB.contains(aabb))
	{
		// The tree Box still contains the object, but the tree Box might be too large.
		// Perhaps the object was moving fast but has since gone to sleep.
		// The huge Box is larger than the new fat Box.
		Box hugeAABB;
		hugeAABB.l = m::muladdv(fatAABB.l, -4.0f, r);
		hugeAABB.u = m::muladdv(fatAABB.u, 4.0f, r);

		if (hugeAABB.contains(treeAABB))
		{
			// The tree Box contains the object Box and the tree Box is
			// not too large. No tree update needed.
			return false;
		}

		// Otherwise the tree Box is huge and needs to be shrunk
	}

	removeLeaf(proxyId);

	m_nodes[proxyId].aabb = fatAABB;

	insertLeaf(proxyId);

	bool alreadyMoved = m_nodes[proxyId].moved;
	m_nodes[proxyId].moved = true;

	if (alreadyMoved) { return false; }

	return true;
}

int32_t
BVH::getHeight() const
{
	if (m_root == BVH_NullIndex) { return 0; }

	return m_nodes[m_root].height;
}

float
BVH::getAreaRatio() const
{
	if (m_root == BVH_NullIndex) { return 0.0f; }

	const Node* root = m_nodes + m_root;
	float rootArea = root->aabb.perimeter();

	float totalArea = 0.0f;
	for (int32_t i = 0; i < m_nodeCapacity; ++i)
	{
		const Node* node = m_nodes + i;
		if (node->height < 0 || testEnd(node) || i == m_root)
		{
			// Free node in pool
			continue;
		}

		totalArea += node->aabb.perimeter();
	}

	return totalArea / rootArea;
}

// Compute the height of a sub-tree.
int32_t
BVH::computeSubTreeHeight(int32_t nodeId) const
{
	assert(0 <= nodeId && nodeId < m_nodeCapacity);
	Node* node = m_nodes + nodeId;

	if (testEnd(node)) { return 0; }

	int32_t height1 = computeSubTreeHeight(node->child1);
	int32_t height2 = computeSubTreeHeight(node->child2);
	return 1 + std::max(height1, height2);
}

int32_t
BVH::computeHeight() const
{
	int32_t height = computeSubTreeHeight(m_root);
	return height;
}

void
BVH::validateStructure(int32_t index) const
{
	if (index == BVH_NullIndex) { return; }

	if (index == m_root) { assert(m_nodes[index].parent == BVH_NullIndex); }

	const Node* node = m_nodes + index;

	int32_t child1 = node->child1;
	int32_t child2 = node->child2;

	if (testEnd(node))
	{
		assert(child1 == BVH_NullIndex);
		assert(child2 == BVH_NullIndex);
		assert(node->height == 0);
		return;
	}

	assert(0 <= child1 && child1 < m_nodeCapacity);
	assert(0 <= child2 && child2 < m_nodeCapacity);

	assert(m_nodes[child1].parent == index);
	assert(m_nodes[child2].parent == index);

	validateStructure(child1);
	validateStructure(child2);
}

void
BVH::validateMetrics(int32_t index) const
{
	if (index == BVH_NullIndex) { return; }

	const Node* node = m_nodes + index;

	int32_t child1 = node->child1;
	int32_t child2 = node->child2;

	if (testEnd(node))
	{
		assert(child1 == BVH_NullIndex);
		assert(child2 == BVH_NullIndex);
		assert(node->height == 0);
		return;
	}

	assert(0 <= child1 && child1 < m_nodeCapacity);
	assert(0 <= child2 && child2 < m_nodeCapacity);

	int32_t height1 = m_nodes[child1].height;
	int32_t height2 = m_nodes[child2].height;
	int32_t height;
	height = 1 + std::max(height1, height2);
	assert(node->height == height);

	Box aabb{m_nodes[child1].aabb.combine(m_nodes[child2].aabb)};

	assert(aabb.l.x == node->aabb.l.x);
	assert(aabb.l.y == node->aabb.l.y);
	assert(aabb.u.x == node->aabb.u.x);
	assert(aabb.u.y == node->aabb.u.y);

	uint32_t categoryBits = m_nodes[child1].categoryBits | m_nodes[child2].categoryBits;
	assert(node->categoryBits == categoryBits);

	validateMetrics(child1);
	validateMetrics(child2);
}

void
BVH::validate()
{
	validateStructure(m_root);
	validateMetrics(m_root);

	int32_t freeCount = 0;
	int32_t freeIndex = m_freeList;
	while (freeIndex != BVH_NullIndex)
	{
		assert(0 <= freeIndex && freeIndex < m_nodeCapacity);
		freeIndex = m_nodes[freeIndex].next;
		++freeCount;
	}

	int32_t height = getHeight();
	int32_t computedHeight = computeHeight();
	assert(height == computedHeight);

	assert(m_nodeCount + freeCount == m_nodeCapacity);
}

int32_t
BVH::getMaxBalance() const
{
	int32_t maxBalance = 0;
	for (int32_t i = 0; i < m_nodeCapacity; ++i)
	{
		const Node* node = m_nodes + i;
		if (node->height <= 1) { continue; }

		assert(testEnd(node) == false);

		int32_t child1 = node->child1;
		int32_t child2 = node->child2;
		int32_t balance = m::abs(m_nodes[child2].height - m_nodes[child1].height);
		maxBalance = std::max(maxBalance, balance);
	}

	return maxBalance;
}

void
BVH::rebuildBottomUp()
{
	auto* nodes = (int32_t*)_malloc(m_nodeCount * sizeof(int32_t));
	int32_t count = 0;

	// Build array of leaves. Free the rest.
	for (int32_t i = 0; i < m_nodeCapacity; ++i)
	{
		if (m_nodes[i].height < 0)
		{
			// free node in pool
			continue;
		}

		if (testEnd(m_nodes + i))
		{
			m_nodes[i].parent = BVH_NullIndex;
			nodes[count] = i;
			++count;
		}
		else { freeLeaf(i); }
	}

	while (count > 1)
	{
		float minCost = FLT_MAX;
		int32_t iMin = -1, jMin = -1;
		for (int32_t i = 0; i < count; ++i)
		{
			Box aabbi = m_nodes[nodes[i]].aabb;

			for (int32_t j = i + 1; j < count; ++j)
			{
				Box aabbj = m_nodes[nodes[j]].aabb;
				Box b{aabbi.combine(aabbj)};
				float cost = b.perimeter();
				if (cost < minCost)
				{
					iMin = i;
					jMin = j;
					minCost = cost;
				}
			}
		}

		int32_t index1 = nodes[iMin];
		int32_t index2 = nodes[jMin];
		Node* child1 = m_nodes + index1;
		Node* child2 = m_nodes + index2;

		int32_t parentIndex = allocLeaf();
		Node* parent = m_nodes + parentIndex;
		parent->child1 = index1;
		parent->child2 = index2;
		parent->aabb = child1->aabb.combine(child2->aabb);
		parent->categoryBits = child1->categoryBits | child2->categoryBits;
		parent->height = 1 + std::max(child1->height, child2->height);
		parent->parent = BVH_NullIndex;

		child1->parent = parentIndex;
		child2->parent = parentIndex;

		nodes[jMin] = nodes[count - 1];
		nodes[iMin] = parentIndex;
		--count;
	}

	m_root = nodes[0];
	_free(nodes); //, m_nodeCount * sizeof(Node));

	validate();
}

void
BVH::shiftOrigin(glm::vec2 newOrigin)
{
	// Build array of leaves. Free the rest.
	for (int32_t i = 0; i < m_nodeCapacity; ++i)
	{
		Node* n = m_nodes + i;
		n->aabb.l.x -= newOrigin.x;
		n->aabb.l.y -= newOrigin.y;
		n->aabb.u.x -= newOrigin.x;
		n->aabb.u.y -= newOrigin.y;
	}
}

#define STACK_SIZE 256

void
BVH::queryFiltered(Box aabb, uint32_t maskBits, TreeQueryCallback callback, void* context)
{
	int32_t stack[STACK_SIZE];
	int32_t stackCount = 0;
	stack[stackCount++] = m_root;

	while (stackCount > 0)
	{
		int32_t nodeId = stack[--stackCount];
		if (nodeId == BVH_NullIndex) { continue; }

		const Node* node = m_nodes + nodeId;

		if (node->aabb.overlaps(aabb) && (node->categoryBits & maskBits) != 0)
		{
			if (testEnd(node))
			{
				// callback to user code with proxy id
				bool proceed = callback(nodeId, node->userData, context);
				if (!proceed) return;
			}
			else
			{
				assert(stackCount <= STACK_SIZE - 2);
				// TODO log this?

				if (stackCount <= STACK_SIZE - 2)
				{
					stack[stackCount++] = node->child1;
					stack[stackCount++] = node->child2;
				}
			}
		}
	}
}

void
BVH::query(Box aabb, TreeQueryCallback callback, void* context)
{
	int32_t stack[STACK_SIZE];
	int32_t stackCount = 0;
	stack[stackCount++] = m_root;

	while (stackCount > 0)
	{
		int32_t nodeId = stack[--stackCount];
		if (nodeId == BVH_NullIndex) { continue; }

		const Node* node = m_nodes + nodeId;

		if (node->aabb.overlaps(aabb))
		{
			if (testEnd(node))
			{
				// callback to user code with proxy id
				bool proceed = callback(nodeId, node->userData, context);
				if (!proceed) return;
			}
			else
			{
				assert(stackCount <= STACK_SIZE - 2);
				// TODO log this?

				if (stackCount <= STACK_SIZE - 2)
				{
					stack[stackCount++] = node->child1;
					stack[stackCount++] = node->child2;
				}
			}
		}
	}
}

void
BVH::raycast(
    const RayCast* input,
    uint32_t maskBits,
    TreeRaycastCallback callback,
    void* context)
{
	glm::vec2 p1 = input->p1;
	glm::vec2 p2 = input->p2;
	glm::vec2 extension = {input->radius, input->radius};

	glm::vec2 r = m::normv(m::subv(p2, p1));

	// v is perpendicular to the segment.
	glm::vec2 v = m::crosssv(1.0f, r);
	glm::vec2 abs_v = m::absv(v);

	// Separating axis for segment (Gino, p80).
	// |dot(v, p1 - c)| > dot(|v|, h)

	float maxFraction = input->maxFraction;

	// Build a bounding box for the segment.
	Box segmentAABB;
	{
		// t is the endpoint of the ray
		glm::vec2 t = m::muladdv(p1, maxFraction, m::subv(p2, p1));

		// Add radius extension
		segmentAABB.l = m::subv(m::minv(p1, t), extension);
		segmentAABB.u = m::subv(m::maxv(p1, t), extension);
	}

	int32_t stack[STACK_SIZE];
	int32_t stackCount = 0;
	stack[stackCount++] = m_root;

	while (stackCount > 0)
	{
		int32_t nodeId = stack[--stackCount];
		if (nodeId == BVH_NullIndex) { continue; }

		const Node* node = m_nodes + nodeId;
		if (!node->aabb.overlaps(segmentAABB) || (node->categoryBits & maskBits) == 0)
		{
			continue;
		}

		// Separating axis for segment (Gino, p80).
		// |dot(v, p1 - c)| > dot(|v|, h)
		// radius extension is added to the node in this case
		glm::vec2 c = node->aabb.center();
		glm::vec2 h = m::addv(node->aabb.extents(), extension);
		float term1 = m::abs(m::dotv(v, m::subv(p1, c)));
		float term2 = m::dotv(abs_v, h);
		if (term2 < term1) { continue; }

		if (testEnd(node))
		{
			RayCast subInput;
			subInput.p1 = input->p1;
			subInput.p2 = input->p2;
			subInput.maxFraction = maxFraction;

			float value = callback(&subInput, nodeId, node->userData, context);
			assert(value >= 0.0f);

			if (value == 0.0f)
			{
				// The client has terminated the ray cast.
				return;
			}

			if (value < maxFraction)
			{
				// Update segment bounding box.
				maxFraction = value;
				glm::vec2 t = m::muladdv(p1, maxFraction, m::subv(p2, p1));
				segmentAABB.l = m::subv(m::minv(p1, t), extension);
				segmentAABB.u = m::subv(m::maxv(p1, t), extension);
			}
		}
		else
		{
			assert(stackCount <= STACK_SIZE - 2);
			// TODO log this?

			if (stackCount <= STACK_SIZE - 2)
			{
				// TODO just put one node on the stack, continue on a child node
				// TODO test ordering children by nearest to ray origin
				stack[stackCount++] = node->child1;
				stack[stackCount++] = node->child2;
			}
		}
	}
}

#define BIN_COUNT 32

// "On Fast Construction of SAH-based Bounding Volume Hierarchies" by Ingo Wald
int32_t
BVH::binSortBoxes(
    int32_t parentIndex,
    Node* leaves,
    int32_t count,
    TreeBin* bins,
    TreePlane* planes)
{
	if (count == 1)
	{
		leaves[0].parent = parentIndex;
		return (int32_t)(leaves - m_nodes);
	}

	Node* nodes = m_nodes;

	glm::vec2 center = leaves[0].aabb.center();
	Box centroidAABB;
	centroidAABB.l = center;
	centroidAABB.u = center;

	for (int32_t i = 1; i < count; ++i)
	{
		center = leaves[i].aabb.center();
		centroidAABB.l = m::minv(centroidAABB.l, center);
		centroidAABB.u = m::maxv(centroidAABB.u, center);
	}

	glm::vec2 d = m::subv(centroidAABB.u, centroidAABB.l);

	int32_t axisIndex;
	float invD;
	if (d.x > d.y)
	{
		axisIndex = 0;
		invD = d.x;
	}
	else
	{
		axisIndex = 1;
		invD = d.y;
	}

	invD = invD > 0.0f ? 1.0f / invD : 0.0f;

	for (int32_t i = 0; i < BIN_COUNT; ++i)
	{
		bins[i].aabb.l = {FLT_MAX, FLT_MAX};
		bins[i].aabb.u = {-FLT_MAX, -FLT_MAX};
		bins[i].count = 0;
	}

	float binCount = BIN_COUNT;
	float lArray[2] = {centroidAABB.l.x, centroidAABB.l.y};
	float minC = lArray[axisIndex];
	for (int32_t i = 0; i < count; ++i)
	{
		glm::vec2 c = leaves[i].aabb.center();
		float cArray[2] = {c.x, c.y};
		auto binIndex = (int32_t)(binCount * (cArray[axisIndex] - minC) * invD);
		binIndex = m::clamp(binIndex, 0, BIN_COUNT - 1);
		leaves[i].next = binIndex;
		bins[binIndex].count += 1;
		bins[binIndex].aabb = bins[binIndex].aabb.combine(leaves[i].aabb);
	}

	int32_t planeCount = BIN_COUNT - 1;

	planes[0].leftCount = bins[0].count;
	planes[0].leftAABB = bins[0].aabb;
	for (int32_t i = 1; i < planeCount; ++i)
	{
		planes[i].leftCount = planes[i - 1].leftCount + bins[i].count;
		planes[i].leftAABB = planes[i - 1].leftAABB.combine(bins[i].aabb);
	}

	planes[planeCount - 1].rightCount = bins[planeCount].count;
	planes[planeCount - 1].rightAABB = bins[planeCount].aabb;
	for (int32_t i = planeCount - 2; i >= 0; --i)
	{
		planes[i].rightCount = planes[i + 1].rightCount + bins[i + 1].count;
		planes[i].rightAABB = planes[i + 1].rightAABB.combine(bins[i + 1].aabb);
	}

	float minCost = FLT_MAX;
	int32_t bestPlane = 0;
	for (int32_t i = 0; i < planeCount; ++i)
	{
		float leftArea = planes[i].leftAABB.perimeter();
		float rightArea = planes[i].rightAABB.perimeter();
		int32_t leftCount = planes[i].leftCount;
		int32_t rightCount = planes[i].rightCount;

		float cost = leftCount * leftArea + rightCount * rightArea;
		if (cost < minCost)
		{
			bestPlane = i;
			minCost = cost;
		}
	}

	assert(m_nodeCount < m_nodeCapacity);
	int32_t nodeIndex = m_nodeCount;
	Node* node = nodes + nodeIndex;
	node->setDefault();
	node->aabb = planes[bestPlane].leftAABB.combine(planes[bestPlane].rightAABB);
	node->parent = parentIndex;
	m_nodeCount += 1;

	int32_t i1 = -1;
	for (int32_t i2 = 0; i2 < count; ++i2)
	{
		int32_t binIndex = leaves[i2].next;
		if (binIndex <= bestPlane)
		{
			++i1;
			Node temp = leaves[i1];
			leaves[i1] = leaves[i2];
			leaves[i2] = temp;
		}
	}

	int32_t leftCount = i1 + 1;
	int32_t rightCount = count - leftCount;

	if (leftCount == 0)
	{
		leftCount = 1;
		rightCount -= 1;
	}
	else if (rightCount == 0)
	{
		leftCount -= 1;
		rightCount = 1;
	}

	// Recurse
	node->child1 = binSortBoxes(nodeIndex, leaves, leftCount, bins, planes);
	node->child2 = binSortBoxes(nodeIndex, leaves + leftCount, rightCount, bins, planes);

	const Node* child1 = nodes + node->child1;
	const Node* child2 = nodes + node->child2;

	node->categoryBits = child1->categoryBits | child2->categoryBits;
	node->height = 1 + std::max(child1->height, child2->height);

	return nodeIndex;
}

int32_t
BVH::getProxyCount() const
{
	return m_proxyCount;
}

void
BVH::rebuildTopDownSAH(ProxyMap* mapArray, int32_t mapCount)
{
	(void)mapCount;
	assert(mapCount == m_proxyCount);

	// need a way to map proxies

	int32_t proxyCount = m_proxyCount;
	int32_t initialCapacity = m_nodeCapacity;

	// Ensure sufficient capacity
	int32_t requiredCapacity = 2 * proxyCount - 1;
	if (initialCapacity < 2 * proxyCount - 1)
	{
		Node* oldNodes = m_nodes;

		// Additional capacity for next rebuild
		m_nodeCapacity = requiredCapacity + requiredCapacity / 2;

		m_nodes = (Node*)_malloc(m_nodeCapacity * sizeof(Node));
		memcpy(m_nodes, oldNodes, m_nodeCount * sizeof(Node));
		_free(oldNodes); //, oldCapcity * sizeof(Node));
	}

	// Copy all leaf nodes to the beginning of the array
	Node* nodes = m_nodes;
	int32_t nodeCount = 0, k = 0;
	while (nodeCount < proxyCount)
	{
		while (nodes[k].height != 0 && k < initialCapacity) { k += 1; }

		if (k == initialCapacity) { break; }

		assert(nodes[k].height == 0);
		nodes[nodeCount] = nodes[k];
		nodes[nodeCount].parent = BVH_NullIndex;
		nodes[nodeCount].moved = false;

		nodeCount += 1;
		k += 1;
	}

	assert(nodeCount == proxyCount);
	m_nodeCount = nodeCount;

	TreeBin bins[BIN_COUNT];
	TreePlane planes[BIN_COUNT - 1];
	m_root = binSortBoxes(BVH_NullIndex, nodes, nodeCount, bins, planes);

	nodeCount = m_nodeCount;

	// Create a map for proxy nodes so the uses can get the new index
	for (int32_t i = 0; i < proxyCount; ++i)
	{
		Node* n = nodes + i;
		assert(n->userData != nullptr);
		mapArray[i].userData = n->userData;
	}

	// Fill free list
	int32_t newCapacity = m_nodeCapacity;
	if (nodeCount < newCapacity)
	{
		for (int32_t i = nodeCount; i < newCapacity - 1; ++i)
		{
			m_nodes[i].next = i + 1;
			m_nodes[i].height = -1;
		}
		m_nodes[m_nodeCapacity - 1].next = BVH_NullIndex;
		m_nodes[m_nodeCapacity - 1].height = -1;
		m_freeList = m_nodeCount;
	}
	else { m_freeList = BVH_NullIndex; }

	validate();
}

void*
BVH::getUserData(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	return m_nodes[proxyId].userData;
}

bool
BVH::wasMoved(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	return m_nodes[proxyId].moved;
}

void
BVH::clearMoved(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	m_nodes[proxyId].moved = false;
}

Box
BVH::getFatAABB(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	return m_nodes[proxyId].aabb;
}
} // namespace candybox

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif
