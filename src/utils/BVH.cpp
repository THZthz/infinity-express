#include <cstring>
#include <cfloat>

#include "utils/AABB.hpp"
#include "utils/Memory.hpp"
#include "utils/BVH.hpp"
#include "utils/Linear.hpp"

#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

namespace ie {

BVH::BVH()
{
	mRoot = BVH_NullIndex;

	mNodeCapacity = 16;
	mNodeCount = 0;
	mNodes = (Node*)_malloc(mNodeCapacity * sizeof(Node));
	memset(mNodes, 0, mNodeCapacity * sizeof(Node));

	// Build a linked list for the free list.
	for (int32_t i = 0; i < mNodeCapacity - 1; ++i)
	{
		mNodes[i].next = i + 1;
		mNodes[i].height = -1;
	}
	mNodes[mNodeCapacity - 1].next = BVH_NullIndex;
	mNodes[mNodeCapacity - 1].height = -1;
	mFreeList = 0;

	mProxyCount = 0;
}

BVH::~BVH() { _free(mNodes); }

// Allocate a node from the pool. Grow the pool if necessary.
int32_t
BVH::AllocLeaf()
{
	// Expand the node pool as needed.
	if (mFreeList == BVH_NullIndex)
	{
		assert(mNodeCount == mNodeCapacity);

		// The free list is empty. Rebuild a bigger pool.
		Node* oldNodes = mNodes;
		int32_t oldCapcity = mNodeCapacity;
		mNodeCapacity += oldCapcity >> 1;
		mNodes = (Node*)_malloc(mNodeCapacity * sizeof(Node));
		memcpy(mNodes, oldNodes, mNodeCount * sizeof(Node));
		_free(oldNodes); //, oldCapcity * sizeof(Node));

		// Build a linked list for the free list. The parent
		// pointer becomes the "next" pointer.
		for (int32_t i = mNodeCount; i < mNodeCapacity - 1; ++i)
		{
			mNodes[i].next = i + 1;
			mNodes[i].height = -1;
		}
		mNodes[mNodeCapacity - 1].next = BVH_NullIndex;
		mNodes[mNodeCapacity - 1].height = -1;
		mFreeList = mNodeCount;
	}

	// Peel a node off the free list.
	int32_t nodeId = mFreeList;
	Node* node = mNodes + nodeId;
	mFreeList = node->next;
	node->SetDefault();
	++mNodeCount;
	return nodeId;
}

// Return a node to the pool.
void
BVH::FreeLeaf(int32_t nodeId)
{
	assert(0 <= nodeId && nodeId < mNodeCapacity);
	assert(0 < mNodeCount);
	mNodes[nodeId].next = mFreeList;
	mNodes[nodeId].height = -1;
	mFreeList = nodeId;
	--mNodeCount;
}

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
int32_t
BVH::BalanceLeaf(int32_t iA)
{
	assert(iA != BVH_NullIndex);

	Node* A = mNodes + iA;
	if (TestEnd(A) || A->height < 2) { return iA; }

	int32_t iB = A->child1;
	int32_t iC = A->child2;
	assert(0 <= iB && iB < mNodeCapacity);
	assert(0 <= iC && iC < mNodeCapacity);

	Node* B = mNodes + iB;
	Node* C = mNodes + iC;

	int32_t balance = C->height - B->height;

	// Rotate C up
	if (balance > 1)
	{
		int32_t iF = C->child1;
		int32_t iG = C->child2;
		Node* F = mNodes + iF;
		Node* G = mNodes + iG;
		assert(0 <= iF && iF < mNodeCapacity);
		assert(0 <= iG && iG < mNodeCapacity);

		// Swap A and C
		C->child1 = iA;
		C->parent = A->parent;
		A->parent = iC;

		// A's old parent should point to C
		if (C->parent != BVH_NullIndex)
		{
			if (mNodes[C->parent].child1 == iA) { mNodes[C->parent].child1 = iC; }
			else
			{
				assert(mNodes[C->parent].child2 == iA);
				mNodes[C->parent].child2 = iC;
			}
		}
		else { mRoot = iC; }

		// Rotate
		if (F->height > G->height)
		{
			C->child2 = iF;
			A->child2 = iG;
			G->parent = iA;
			A->aabb = B->aabb.Union(G->aabb);
			C->aabb = A->aabb.Union(F->aabb);

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
			A->aabb = B->aabb.Union(F->aabb);
			C->aabb = A->aabb.Union(G->aabb);

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
		Node* D = mNodes + iD;
		Node* E = mNodes + iE;
		assert(0 <= iD && iD < mNodeCapacity);
		assert(0 <= iE && iE < mNodeCapacity);

		// Swap A and B
		B->child1 = iA;
		B->parent = A->parent;
		A->parent = iB;

		// A's old parent should point to B
		if (B->parent != BVH_NullIndex)
		{
			if (mNodes[B->parent].child1 == iA) { mNodes[B->parent].child1 = iB; }
			else
			{
				assert(mNodes[B->parent].child2 == iA);
				mNodes[B->parent].child2 = iB;
			}
		}
		else { mRoot = iB; }

		// Rotate
		if (D->height > E->height)
		{
			B->child2 = iD;
			A->child1 = iE;
			E->parent = iA;
			A->aabb = C->aabb.Union(E->aabb);
			B->aabb = A->aabb.Union(D->aabb);

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
			A->aabb = C->aabb.Union(D->aabb);
			B->aabb = A->aabb.Union(E->aabb);

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
BVH::InsertLeaf(int32_t leaf)
{
	if (mRoot == BVH_NullIndex)
	{
		mRoot = leaf;
		mNodes[mRoot].parent = BVH_NullIndex;
		return;
	}

	// Find the best sibling for this node
	AABB leafAABB = mNodes[leaf].aabb;
	int32_t index = mRoot;
	while (!TestEnd(mNodes + index))
	{
		int32_t child1 = mNodes[index].child1;
		int32_t child2 = mNodes[index].child2;

		float area = mNodes[index].aabb.Perimeter();

		AABB combinedAABB{mNodes[index].aabb.Union(leafAABB)};
		float combinedArea = combinedAABB.Perimeter();

		// Cost of creating a new parent for this node and the new leaf
		float cost = 2.0f * combinedArea;

		// Minimum cost of pushing the leaf further down the tree
		float inheritanceCost = 2.0f * (combinedArea - area);

		// Cost of descending into child1
		float cost1;
		if (TestEnd(mNodes + child1))
		{
			AABB aabb{leafAABB.Union(mNodes[child1].aabb)};
			cost1 = aabb.Perimeter() + inheritanceCost;
		}
		else
		{
			AABB aabb{leafAABB.Union(mNodes[child1].aabb)};
			float oldArea = mNodes[child1].aabb.Perimeter();
			float newArea = aabb.Perimeter();
			cost1 = (newArea - oldArea) + inheritanceCost;
		}

		// Cost of descending into child2
		float cost2;
		if (TestEnd(mNodes + child2))
		{
			AABB aabb{leafAABB.Union(mNodes[child2].aabb)};
			cost2 = aabb.Perimeter() + inheritanceCost;
		}
		else
		{
			AABB aabb{leafAABB.Union(mNodes[child2].aabb)};
			float oldArea = mNodes[child2].aabb.Perimeter();
			float newArea = aabb.Perimeter();
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
	int32_t oldParent = mNodes[sibling].parent;
	int32_t newParent = AllocLeaf();
	mNodes[newParent].parent = oldParent;
	mNodes[newParent].userData = nullptr;
	mNodes[newParent].aabb = leafAABB.Union(mNodes[sibling].aabb);
	mNodes[newParent].categoryBits = mNodes[leaf].categoryBits | mNodes[sibling].categoryBits;
	mNodes[newParent].height = mNodes[sibling].height + 1;

	if (oldParent != BVH_NullIndex)
	{
		// The sibling was not the root.
		if (mNodes[oldParent].child1 == sibling) { mNodes[oldParent].child1 = newParent; }
		else { mNodes[oldParent].child2 = newParent; }

		mNodes[newParent].child1 = sibling;
		mNodes[newParent].child2 = leaf;
		mNodes[sibling].parent = newParent;
		mNodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		mNodes[newParent].child1 = sibling;
		mNodes[newParent].child2 = leaf;
		mNodes[sibling].parent = newParent;
		mNodes[leaf].parent = newParent;
		mRoot = newParent;
	}

	// Walk back up the tree fixing heights and AABBs
	index = mNodes[leaf].parent;
	while (index != BVH_NullIndex)
	{
		index = BalanceLeaf(index);

		int32_t child1 = mNodes[index].child1;
		int32_t child2 = mNodes[index].child2;

		assert(child1 != BVH_NullIndex);
		assert(child2 != BVH_NullIndex);

		mNodes[index].aabb = mNodes[child1].aabb.Union(mNodes[child2].aabb);
		mNodes[index].categoryBits = mNodes[child1].categoryBits | mNodes[child2].categoryBits;
		mNodes[index].height = 1 + std::max(mNodes[child1].height, mNodes[child2].height);

		index = mNodes[index].parent;
	}

	// Validate();
}

void
BVH::RemoveLeaf(int32_t leaf)
{
	if (leaf == mRoot)
	{
		mRoot = BVH_NullIndex;
		return;
	}

	int32_t parent = mNodes[leaf].parent;
	int32_t grandParent = mNodes[parent].parent;
	int32_t sibling;
	if (mNodes[parent].child1 == leaf) { sibling = mNodes[parent].child2; }
	else { sibling = mNodes[parent].child1; }

	if (grandParent != BVH_NullIndex)
	{
		// Destroy parent and connect sibling to grandParent.
		if (mNodes[grandParent].child1 == parent) { mNodes[grandParent].child1 = sibling; }
		else { mNodes[grandParent].child2 = sibling; }
		mNodes[sibling].parent = grandParent;
		FreeLeaf(parent);

		// Adjust ancestor bounds.
		int32_t index = grandParent;
		while (index != BVH_NullIndex)
		{
			index = BalanceLeaf(index);

			int32_t child1 = mNodes[index].child1;
			int32_t child2 = mNodes[index].child2;

			mNodes[index].aabb = mNodes[child1].aabb.Union(mNodes[child2].aabb);
			mNodes[index]
			    .categoryBits = mNodes[child1].categoryBits | mNodes[child2].categoryBits;
			mNodes[index].height = 1 + std::max(mNodes[child1].height, mNodes[child2].height);

			index = mNodes[index].parent;
		}
	}
	else
	{
		mRoot = sibling;
		mNodes[sibling].parent = BVH_NullIndex;
		FreeLeaf(parent);
	}

	// Validate();
}

// Create a proxy in the tree as a leaf node. We return the index
// of the node instead of a pointer so that we can grow
// the node pool.
int32_t
BVH::Add(AABB aabb, uint32_t categoryBits, void* userData)
{
	assert(-HUGE_NUMBER < aabb.l.x && aabb.l.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.l.y && aabb.l.y < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.x && aabb.u.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.y && aabb.u.y < HUGE_NUMBER);

	int32_t proxyId = AllocLeaf();
	Node* node = mNodes + proxyId;

	// Fatten the aabb.
	glm::vec2 r = {AABB_EXTENSION, AABB_EXTENSION};
	node->aabb.l = SubV(aabb.l, r);
	node->aabb.u = AddV(aabb.u, r);
	node->userData = userData;
	node->categoryBits = categoryBits;
	node->height = 0;
	node->moved = true;

	InsertLeaf(proxyId);

	mProxyCount += 1;

	return proxyId;
}

void
BVH::Remove(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < mNodeCapacity);
	assert(TestEnd(mNodes + proxyId));

	RemoveLeaf(proxyId);
	FreeLeaf(proxyId);

	assert(mProxyCount > 0);
	mProxyCount -= 1;
}

bool
BVH::Move(int32_t proxyId, AABB aabb)
{
	assert(-HUGE_NUMBER < aabb.l.x && aabb.l.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.l.y && aabb.l.y < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.x && aabb.u.x < HUGE_NUMBER);
	assert(-HUGE_NUMBER < aabb.u.y && aabb.u.y < HUGE_NUMBER);

	assert(0 <= proxyId && proxyId < mNodeCapacity);
	assert(TestEnd(mNodes + proxyId));

	// Extend AABB
	AABB fatAABB;
	glm::vec2 r = {AABB_EXTENSION, AABB_EXTENSION};
	fatAABB.l = SubV(aabb.l, r);
	fatAABB.u = AddV(aabb.u, r);

	AABB treeAABB = mNodes[proxyId].aabb;
	if (treeAABB.Contains(aabb))
	{
		// The tree AABB still contains the object, but the tree AABB might be too large.
		// Perhaps the object was moving fast but has since gone to sleep.
		// The huge AABB is larger than the new fat AABB.
		AABB hugeAABB;
		hugeAABB.l = MulAddV(fatAABB.l, -4.0f, r);
		hugeAABB.u = MulAddV(fatAABB.u, 4.0f, r);

		if (hugeAABB.Contains(treeAABB))
		{
			// The tree AABB contains the object AABB and the tree AABB is
			// not too large. No tree update needed.
			return false;
		}

		// Otherwise the tree AABB is huge and needs to be shrunk
	}

	RemoveLeaf(proxyId);

	mNodes[proxyId].aabb = fatAABB;

	InsertLeaf(proxyId);

	bool alreadyMoved = mNodes[proxyId].moved;
	mNodes[proxyId].moved = true;

	if (alreadyMoved) { return false; }

	return true;
}

int32_t
BVH::GetHeight() const
{
	if (mRoot == BVH_NullIndex) { return 0; }

	return mNodes[mRoot].height;
}

float
BVH::GetAreaRatio() const
{
	if (mRoot == BVH_NullIndex) { return 0.0f; }

	const Node* root = mNodes + mRoot;
	float rootArea = root->aabb.Perimeter();

	float totalArea = 0.0f;
	for (int32_t i = 0; i < mNodeCapacity; ++i)
	{
		const Node* node = mNodes + i;
		if (node->height < 0 || TestEnd(node) || i == mRoot)
		{
			// Free node in pool
			continue;
		}

		totalArea += node->aabb.Perimeter();
	}

	return totalArea / rootArea;
}

// Compute the height of a sub-tree.
int32_t
BVH::ComputeSubTreeHeight(int32_t nodeId) const
{
	assert(0 <= nodeId && nodeId < mNodeCapacity);
	Node* node = mNodes + nodeId;

	if (TestEnd(node)) { return 0; }

	int32_t height1 = ComputeSubTreeHeight(node->child1);
	int32_t height2 = ComputeSubTreeHeight(node->child2);
	return 1 + std::max(height1, height2);
}

int32_t
BVH::ComputeHeight() const
{
	int32_t height = ComputeSubTreeHeight(mRoot);
	return height;
}

void
BVH::ValidateStructure(int32_t index) const
{
	if (index == BVH_NullIndex) { return; }

	if (index == mRoot) { assert(mNodes[index].parent == BVH_NullIndex); }

	const Node* node = mNodes + index;

	int32_t child1 = node->child1;
	int32_t child2 = node->child2;

	if (TestEnd(node))
	{
		assert(child1 == BVH_NullIndex);
		assert(child2 == BVH_NullIndex);
		assert(node->height == 0);
		return;
	}

	assert(0 <= child1 && child1 < mNodeCapacity);
	assert(0 <= child2 && child2 < mNodeCapacity);

	assert(mNodes[child1].parent == index);
	assert(mNodes[child2].parent == index);

	ValidateStructure(child1);
	ValidateStructure(child2);
}

void
BVH::ValidateMetrics(int32_t index) const
{
	if (index == BVH_NullIndex) { return; }

	const Node* node = mNodes + index;

	int32_t child1 = node->child1;
	int32_t child2 = node->child2;

	if (TestEnd(node))
	{
		assert(child1 == BVH_NullIndex);
		assert(child2 == BVH_NullIndex);
		assert(node->height == 0);
		return;
	}

	assert(0 <= child1 && child1 < mNodeCapacity);
	assert(0 <= child2 && child2 < mNodeCapacity);

	int32_t height1 = mNodes[child1].height;
	int32_t height2 = mNodes[child2].height;
	int32_t height;
	height = 1 + std::max(height1, height2);
	assert(node->height == height);

	AABB aabb{mNodes[child1].aabb.Union(mNodes[child2].aabb)};

	assert(aabb.l.x == node->aabb.l.x);
	assert(aabb.l.y == node->aabb.l.y);
	assert(aabb.u.x == node->aabb.u.x);
	assert(aabb.u.y == node->aabb.u.y);

	uint32_t categoryBits = mNodes[child1].categoryBits | mNodes[child2].categoryBits;
	assert(node->categoryBits == categoryBits);

	ValidateMetrics(child1);
	ValidateMetrics(child2);
}

void
BVH::Validate()
{
	ValidateStructure(mRoot);
	ValidateMetrics(mRoot);

	int32_t freeCount = 0;
	int32_t freeIndex = mFreeList;
	while (freeIndex != BVH_NullIndex)
	{
		assert(0 <= freeIndex && freeIndex < mNodeCapacity);
		freeIndex = mNodes[freeIndex].next;
		++freeCount;
	}

	int32_t height = GetHeight();
	int32_t computedHeight = ComputeHeight();
	assert(height == computedHeight);

	assert(mNodeCount + freeCount == mNodeCapacity);
}

int32_t
BVH::GetMaxBalance() const
{
	int32_t maxBalance = 0;
	for (int32_t i = 0; i < mNodeCapacity; ++i)
	{
		const Node* node = mNodes + i;
		if (node->height <= 1) { continue; }

		assert(TestEnd(node) == false);

		int32_t child1 = node->child1;
		int32_t child2 = node->child2;
		int32_t balance = Abs(mNodes[child2].height - mNodes[child1].height);
		maxBalance = std::max(maxBalance, balance);
	}

	return maxBalance;
}

void
BVH::RebuildBottomUp()
{
	auto* nodes = (int32_t*)_malloc(mNodeCount * sizeof(int32_t));
	int32_t count = 0;

	// Build array of leaves. Free the rest.
	for (int32_t i = 0; i < mNodeCapacity; ++i)
	{
		if (mNodes[i].height < 0)
		{
			// free node in pool
			continue;
		}

		if (TestEnd(mNodes + i))
		{
			mNodes[i].parent = BVH_NullIndex;
			nodes[count] = i;
			++count;
		}
		else { FreeLeaf(i); }
	}

	while (count > 1)
	{
		float minCost = FLT_MAX;
		int32_t iMin = -1, jMin = -1;
		for (int32_t i = 0; i < count; ++i)
		{
			AABB aabbi = mNodes[nodes[i]].aabb;

			for (int32_t j = i + 1; j < count; ++j)
			{
				AABB aabbj = mNodes[nodes[j]].aabb;
				AABB b{aabbi.Union(aabbj)};
				float cost = b.Perimeter();
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
		Node* child1 = mNodes + index1;
		Node* child2 = mNodes + index2;

		int32_t parentIndex = AllocLeaf();
		Node* parent = mNodes + parentIndex;
		parent->child1 = index1;
		parent->child2 = index2;
		parent->aabb = child1->aabb.Union(child2->aabb);
		parent->categoryBits = child1->categoryBits | child2->categoryBits;
		parent->height = 1 + std::max(child1->height, child2->height);
		parent->parent = BVH_NullIndex;

		child1->parent = parentIndex;
		child2->parent = parentIndex;

		nodes[jMin] = nodes[count - 1];
		nodes[iMin] = parentIndex;
		--count;
	}

	mRoot = nodes[0];
	_free(nodes); //, mNodeCount * sizeof(Node));

	Validate();
}

void
BVH::ShiftOrigin(glm::vec2 newOrigin)
{
	// Build array of leaves. Free the rest.
	for (int32_t i = 0; i < mNodeCapacity; ++i)
	{
		Node* n = mNodes + i;
		n->aabb.l.x -= newOrigin.x;
		n->aabb.l.y -= newOrigin.y;
		n->aabb.u.x -= newOrigin.x;
		n->aabb.u.y -= newOrigin.y;
	}
}

#define STACK_SIZE 256

void
BVH::QueryFiltered(AABB aabb, uint32_t maskBits, TreeQueryCallback callback, void* context)
{
	int32_t stack[STACK_SIZE];
	int32_t stackCount = 0;
	stack[stackCount++] = mRoot;

	while (stackCount > 0)
	{
		int32_t nodeId = stack[--stackCount];
		if (nodeId == BVH_NullIndex) { continue; }

		const Node* node = mNodes + nodeId;

		if (node->aabb.Overlaps(aabb) && (node->categoryBits & maskBits) != 0)
		{
			if (TestEnd(node))
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
BVH::Query(AABB aabb, TreeQueryCallback callback, void* context)
{
	int32_t stack[STACK_SIZE];
	int32_t stackCount = 0;
	stack[stackCount++] = mRoot;

	while (stackCount > 0)
	{
		int32_t nodeId = stack[--stackCount];
		if (nodeId == BVH_NullIndex) { continue; }

		const Node* node = mNodes + nodeId;

		if (node->aabb.Overlaps(aabb))
		{
			if (TestEnd(node))
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
BVH::Raycast(
    const RayCast* input,
    uint32_t maskBits,
    TreeRaycastCallback callback,
    void* context)
{
	glm::vec2 p1 = input->p1;
	glm::vec2 p2 = input->p2;
	glm::vec2 extension = {input->radius, input->radius};

	glm::vec2 r = NormV(SubV(p2, p1));

	// v is perpendicular to the segment.
	glm::vec2 v = CrossSV(1.0f, r);
	glm::vec2 abs_v = AbsV(v);

	// Separating axis for segment (Gino, p80).
	// |dot(v, p1 - c)| > dot(|v|, h)

	float maxFraction = input->maxFraction;

	// Build a bounding box for the segment.
	AABB segmentAABB;
	{
		// t is the endpoint of the ray
		glm::vec2 t = MulAddV(p1, maxFraction, SubV(p2, p1));

		// Add radius extension
		segmentAABB.l = SubV(MinV(p1, t), extension);
		segmentAABB.u = SubV(MaxV(p1, t), extension);
	}

	int32_t stack[STACK_SIZE];
	int32_t stackCount = 0;
	stack[stackCount++] = mRoot;

	while (stackCount > 0)
	{
		int32_t nodeId = stack[--stackCount];
		if (nodeId == BVH_NullIndex) { continue; }

		const Node* node = mNodes + nodeId;
		if (!node->aabb.Overlaps(segmentAABB) || (node->categoryBits & maskBits) == 0)
		{
			continue;
		}

		// Separating axis for segment (Gino, p80).
		// |dot(v, p1 - c)| > dot(|v|, h)
		// radius extension is added to the node in this case
		glm::vec2 c = node->aabb.Center();
		glm::vec2 h = AddV(node->aabb.Extents(), extension);
		float term1 = Abs(DotV(v, SubV(p1, c)));
		float term2 = DotV(abs_v, h);
		if (term2 < term1) { continue; }

		if (TestEnd(node))
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
				glm::vec2 t = MulAddV(p1, maxFraction, SubV(p2, p1));
				segmentAABB.l = SubV(MinV(p1, t), extension);
				segmentAABB.u = SubV(MaxV(p1, t), extension);
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
BVH::BinSortBoxes(
    int32_t parentIndex,
    Node* leaves,
    int32_t count,
    TreeBin* bins,
    TreePlane* planes)
{
	if (count == 1)
	{
		leaves[0].parent = parentIndex;
		return (int32_t)(leaves - mNodes);
	}

	Node* nodes = mNodes;

	glm::vec2 center = leaves[0].aabb.Center();
	AABB centroidAABB;
	centroidAABB.l = center;
	centroidAABB.u = center;

	for (int32_t i = 1; i < count; ++i)
	{
		center = leaves[i].aabb.Center();
		centroidAABB.l = MinV(centroidAABB.l, center);
		centroidAABB.u = MaxV(centroidAABB.u, center);
	}

	glm::vec2 d = SubV(centroidAABB.u, centroidAABB.l);

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
		glm::vec2 c = leaves[i].aabb.Center();
		float cArray[2] = {c.x, c.y};
		auto binIndex = (int32_t)(binCount * (cArray[axisIndex] - minC) * invD);
		binIndex = Clamp(binIndex, 0, BIN_COUNT - 1);
		leaves[i].next = binIndex;
		bins[binIndex].count += 1;
		bins[binIndex].aabb = bins[binIndex].aabb.Union(leaves[i].aabb);
	}

	int32_t planeCount = BIN_COUNT - 1;

	planes[0].leftCount = bins[0].count;
	planes[0].leftAABB = bins[0].aabb;
	for (int32_t i = 1; i < planeCount; ++i)
	{
		planes[i].leftCount = planes[i - 1].leftCount + bins[i].count;
		planes[i].leftAABB = planes[i - 1].leftAABB.Union(bins[i].aabb);
	}

	planes[planeCount - 1].rightCount = bins[planeCount].count;
	planes[planeCount - 1].rightAABB = bins[planeCount].aabb;
	for (int32_t i = planeCount - 2; i >= 0; --i)
	{
		planes[i].rightCount = planes[i + 1].rightCount + bins[i + 1].count;
		planes[i].rightAABB = planes[i + 1].rightAABB.Union(bins[i + 1].aabb);
	}

	float minCost = FLT_MAX;
	int32_t bestPlane = 0;
	for (int32_t i = 0; i < planeCount; ++i)
	{
		float leftArea = planes[i].leftAABB.Perimeter();
		float rightArea = planes[i].rightAABB.Perimeter();
		int32_t leftCount = planes[i].leftCount;
		int32_t rightCount = planes[i].rightCount;

		float cost = leftCount * leftArea + rightCount * rightArea;
		if (cost < minCost)
		{
			bestPlane = i;
			minCost = cost;
		}
	}

	assert(mNodeCount < mNodeCapacity);
	int32_t nodeIndex = mNodeCount;
	Node* node = nodes + nodeIndex;
	node->SetDefault();
	node->aabb = planes[bestPlane].leftAABB.Union(planes[bestPlane].rightAABB);
	node->parent = parentIndex;
	mNodeCount += 1;

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
	node->child1 = BinSortBoxes(nodeIndex, leaves, leftCount, bins, planes);
	node->child2 = BinSortBoxes(nodeIndex, leaves + leftCount, rightCount, bins, planes);

	const Node* child1 = nodes + node->child1;
	const Node* child2 = nodes + node->child2;

	node->categoryBits = child1->categoryBits | child2->categoryBits;
	node->height = 1 + std::max(child1->height, child2->height);

	return nodeIndex;
}

int32_t
BVH::GetProxyCount()
{
	return mProxyCount;
}

void
BVH::RebuildTopDownSAH(ProxyMap* mapArray, int32_t mapCount)
{
	(void)mapCount;
	assert(mapCount == mProxyCount);

	// need a way to map proxies

	int32_t proxyCount = mProxyCount;
	int32_t initialCapacity = mNodeCapacity;

	// Ensure sufficient capacity
	int32_t requiredCapacity = 2 * proxyCount - 1;
	if (initialCapacity < 2 * proxyCount - 1)
	{
		Node* oldNodes = mNodes;

		// Additional capacity for next rebuild
		mNodeCapacity = requiredCapacity + requiredCapacity / 2;

		mNodes = (Node*)_malloc(mNodeCapacity * sizeof(Node));
		memcpy(mNodes, oldNodes, mNodeCount * sizeof(Node));
		_free(oldNodes); //, oldCapcity * sizeof(Node));
	}

	// Copy all leaf nodes to the beginning of the array
	Node* nodes = mNodes;
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
	mNodeCount = nodeCount;

	TreeBin bins[BIN_COUNT];
	TreePlane planes[BIN_COUNT - 1];
	mRoot = BinSortBoxes(BVH_NullIndex, nodes, nodeCount, bins, planes);

	nodeCount = mNodeCount;

	// Create a map for proxy nodes so the uses can get the new index
	for (int32_t i = 0; i < proxyCount; ++i)
	{
		Node* n = nodes + i;
		assert(n->userData != nullptr);
		mapArray[i].userData = n->userData;
	}

	// Fill free list
	int32_t newCapacity = mNodeCapacity;
	if (nodeCount < newCapacity)
	{
		for (int32_t i = nodeCount; i < newCapacity - 1; ++i)
		{
			mNodes[i].next = i + 1;
			mNodes[i].height = -1;
		}
		mNodes[mNodeCapacity - 1].next = BVH_NullIndex;
		mNodes[mNodeCapacity - 1].height = -1;
		mFreeList = mNodeCount;
	}
	else { mFreeList = BVH_NullIndex; }

	Validate();
}

void*
BVH::GetUserData(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < mNodeCapacity);
	return mNodes[proxyId].userData;
}

bool
BVH::WasMoved(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < mNodeCapacity);
	return mNodes[proxyId].moved;
}

void
BVH::ClearMoved(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < mNodeCapacity);
	mNodes[proxyId].moved = false;
}

AABB
BVH::GetFatAABB(int32_t proxyId)
{
	assert(0 <= proxyId && proxyId < mNodeCapacity);
	return mNodes[proxyId].aabb;
}
} // namespace ie

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif
