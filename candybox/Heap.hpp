#ifndef IE_HEAP_HPP
#define IE_HEAP_HPP

#include <cstdint>
#include <cstring>

namespace ie
{

/*██╗  ██╗███████╗ █████╗ ██████╗ */
/*██║  ██║██╔════╝██╔══██╗██╔══██╗*/
/*███████║█████╗  ███████║██████╔╝*/
/*██╔══██║██╔══╝  ██╔══██║██╔═══╝ */
/*██║  ██║███████╗██║  ██║██║     */
/*╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝    */

namespace priv
{
static inline void *EleAt(void *heap, uint32_t eleSize, uint32_t pos) { return (char *) heap + pos * eleSize; }
static inline void SetEleByIdx(void *heap, uint32_t eleSize, uint32_t dstPos, uint32_t srcPos) { memcpy(EleAt(heap, eleSize, dstPos), EleAt(heap, eleSize, srcPos), eleSize); }
static inline void SetEle(void *heap, uint32_t eleSize, uint32_t pos, void *value) { memcpy(EleAt(heap, eleSize, pos), value, eleSize); }
}

//! \defgroup Heap
//! @{

typedef int (*HeapIsSmallerCb)(const void *a, const void *b);

static inline void *
HeapExtractMin(void *heap, uint32_t eleSize, uint32_t length)
{
	if (length > 0) return priv::EleAt(heap, eleSize, 0);
	return nullptr;
}

/// Push an element onto the heap.
/// After push the element to heap, length will be add by 1.
/// \param heap Should be able to hold "length + 1" elements.
void
HeapPush(void *heap, uint32_t eleSize, uint32_t length, void *item, HeapIsSmallerCb isSmaller);

/// Pop an element off the heap.
/// The popped element will be stayed at heap[length - 1].
/// Notice that length will be minus by 1 after poping.
void *HeapPop(void *heap, uint32_t eleSize, uint32_t length, HeapIsSmallerCb isSmaller);

/// Pop and return the current smallest element, and add the newItem.
/// This is moew efficient than nvgHeapPop followed by nvgHeapPush, and can be moew
/// appropriate when using a fixed size heap.
/// Notice that the value returned may be larger than newItem! That constrains resonnable
/// uses of this routine unless written as a part of conditional replacement:
/// 	if (item > heap[0]) item = replace(heap, item);
/// \return If successfully replaced, return the poppedItem, otherwise return NULL.
void *HeapReplace(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    void *newItem,
    void *poppedItem,
    HeapIsSmallerCb isSmaller);

/// Fast version of a HeapPush followed by a HeapPop.
/// \param item Pointer to the item to insert. The content of "item" will be
/// replaced by the samllest element in the heap.
/// \return "item" no matter it is smaller, equal or greater than the minimum of the heap.
void *HeapPushPop(
    void *heap,
    uint32_t eleSize,
    uint32_t length,
    void *item,
    HeapIsSmallerCb isSmaller);

/// Transform array into heap, in-place, in O(length) time.
void HeapMake(void *heap, uint32_t eleSize, uint32_t length, HeapIsSmallerCb isSmaller);

//! @}

} // namespace ie

#endif // IE_HEAP_HPP
