/// \file heap.cpp
/// \brief Not type-safe minimum heap implementation.
/// Although this is a generic implementation of binary minimal heap, it is recommended to
/// implement specifically.

#include <cassert>
#include "utils/Heap.hpp"

const int MAX_ELE_SIZE = 128;

namespace ie {


// 'heap' is a heap at all indices >= startpos, except possibly for pos.  pos
// is the index of a leaf with a possibly out-of-order value.  Restore the
// heap invariant.
static void
SiftDown(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    uint32_t startPos,
    uint32_t pos,
    HeapIsSmallerCb isSmaller)
{
	char newItem[MAX_ELE_SIZE];
	memcpy(newItem, priv::EleAt(heap, eleSize, pos), eleSize);

	// Follow the path to the root, moving the parents down until finding a place newItem
	// fit.
	while (pos > startPos)
	{
		uint32_t parentPos = (pos - 1) >> 1;
		assert(pos >= 1);
		void *parent = priv::EleAt(heap, eleSize, parentPos);
		if (isSmaller(newItem, parent))
		{
			priv::SetEle(heap, eleSize, pos, parent);
			pos = parentPos;
			continue;
		}
		break;
	}

	priv::SetEle(heap, eleSize, pos, newItem);
}

// The child indices of heap index pos are already heaps, and we want to make
// a heap at index pos too.  We do this by bubbling the smaller child of
// pos up (and so on with that child's children, etc) until hitting a leaf,
// then using _siftdown to move the oddball originally at index pos into place.
//
// We *could* break out of the loop as soon as we find a pos where newitem <=
// both its children, but turns out that's not a good idea, and despite that
// many books write the algorithm that way.  During a heap pop, the last array
// element is sifted in, and that tends to be large, so that comparing it
// against values starting from the root usually doesn't pay (= usually doesn't
// get us out of the loop early).  See Knuth, Volume 3, where this is
// explained and quantified in an exercise.
//
// Cutting the # of comparisons is important, since these routines have no
// way to extract "the priority" from an array element, so that intelligence
// is likely to be hiding in custom comparison methods, or in array elements
// storing (priority, record) tuples.  Comparisons are thus potentially
// expensive.
//
// On random arrays of length 1000, making this change cut the number of
// comparisons made by heapify() a little, and those made by exhaustive
// heappop() a lot, in accord with theory.  Here are typical results from 3
// runs (3 just to demonstrate how small the variance is):
//
// Compares needed by heapify     Compares needed by 1000 heappops
// --------------------------     --------------------------------
// 1837 cut to 1663               14996 cut to 8680
// 1855 cut to 1659               14966 cut to 8678
// 1847 cut to 1660               15024 cut to 8703
//
// Building the heap by using heappush() 1000 times instead required
// 2198, 2148, and 2219 compares:  heapify() is more efficient, when
// you can use it.
//
// The total compares needed by list.sort() on the same lists were 8627,
// 8627, and 8632 (this should be compared to the sum of heapify() and
// heappop() compares):  list.sort() is (unsurprisingly!) more efficient
// for sorting.

static void
SiftUp(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    uint32_t pos,
    HeapIsSmallerCb isSmaller)
{
	uint32_t endPos = length;
	uint32_t startPos = pos;
	char newItem[MAX_ELE_SIZE];
	memcpy(newItem, priv::EleAt(heap, eleSize, pos), eleSize);

	// Bubble up the smaller child until we hit a leaf.
	uint32_t childPos = 2 * pos + 1; // Left-most child.
	while (childPos < endPos)
	{
		// Set childPos to index of smaller child.
		uint32_t rightPos = childPos + 1;
		if (rightPos < endPos &&
		    !isSmaller(priv::EleAt(heap, eleSize, childPos), priv::EleAt(heap, eleSize, rightPos)))
		{
			childPos = rightPos;
		}

		// Move the smaller child up.
		priv::SetEleByIdx(heap, eleSize, pos, childPos);

		pos = childPos;
		childPos = 2 * pos + 1;
	}

	// The lead at pos is empty now.
	// Put the newItem here, and bubble it up to its final place (by sifting its parents
	// down).
	priv::SetEle(heap, eleSize, pos, newItem);
	SiftDown(heap, eleSize, length, startPos, pos, isSmaller);
}

void
HeapPush(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    void *item,
    HeapIsSmallerCb isSmaller)
{
	// Push item onto heap, maintaining the heap invariant.
	priv::SetEle(heap, eleSize, length, item);
	length++;
	SiftDown(heap, eleSize, length, 0, length - 1, isSmaller);
}

void *
HeapPop(void *heap, uint32_t eleSize, uint32_t length, HeapIsSmallerCb isSmaller)
{
	if (length == 1)
	{
		return priv::EleAt(heap, eleSize, 0);
	}
	else if (length > 1)
	{
		char lastElt[MAX_ELE_SIZE];
		static char returnItem[MAX_ELE_SIZE];
		assert(eleSize <= MAX_ELE_SIZE);
		memcpy(lastElt, priv::EleAt(heap, eleSize, length - 1), eleSize);
		length--;

		memcpy(returnItem, priv::EleAt(heap, eleSize, 0), eleSize);
		priv::SetEle(heap, eleSize, 0, lastElt);
		SiftUp(heap, eleSize, length, 0, isSmaller);
		return returnItem;
	}
	else
	{
		return nullptr;
	}
}

void *
HeapReplace(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    void *newItem,
    void *poppedItem,
    HeapIsSmallerCb isSmaller)
{
	if (length >= 1)
	{
		memcpy(poppedItem, priv::EleAt(heap, eleSize, 0), eleSize);
		priv::SetEle(heap, eleSize, 0, newItem);
		SiftUp(heap, eleSize, length, 0, isSmaller);
		return poppedItem;
	}
	else
	{
		return nullptr;
	}
}

void *
HeapPushPop(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    void *item,
    HeapIsSmallerCb isSmaller)
{
	if (isSmaller(priv::EleAt(heap, eleSize, 0), item))
	{
		char buffer[MAX_ELE_SIZE];
		assert(MAX_ELE_SIZE >= eleSize);
		memcpy(buffer, priv::EleAt(heap, eleSize, 0), eleSize);
		priv::SetEle(heap, eleSize, 0, item);
		memcpy(item, buffer, eleSize);
		SiftUp(heap, eleSize, length, 0, isSmaller);
	}

	return item;
}

void
HeapMake(void *heap, uint32_t eleSize, uint32_t length, HeapIsSmallerCb isSmaller)
{
	uint32_t i;
	for (i = length / 2 + 1; i > 0; i--)
	{
		uint32_t pos = i - 1;
		SiftUp(heap, eleSize, length, pos, isSmaller);
	}
}

}
