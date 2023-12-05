#ifndef IE_MEMORY_HPP
#define IE_MEMORY_HPP

#ifdef __APPLE__
#	include <malloc/malloc.h>
#else
#	include <malloc.h>
#endif

//#if defined(_WIN32) || defined(__CYGWIN__)
//#	define _WIN_
//#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#	include <windows.h>
#	include <Psapi.h>
#	undef min
#	undef max
#elif defined(__linux__)
#	include <sys/types.h>
#	include <sys/sysinfo.h>
#endif

#include <cassert>
#include <algorithm>
#include <limits>
#include <cstdint>

namespace ie {
static inline void*
_malloc(size_t size)
{
	void* ptr = malloc(size);
	assert(ptr);
	return ptr;
}

static inline void*
_realloc(void* ptr, size_t size)
{
	void* n_ptr = realloc(ptr, size);
	assert(n_ptr);
	return n_ptr;
}

static inline void
_free(void* ptr)
{
	free(ptr);
}

static inline void*
_calloc(size_t n, size_t size)
{
	void* ptr = calloc(n, size);
	assert(ptr);
	return ptr;
}
} // namespace ie

namespace ie {

uint64_t GetSystemMemory();
uint64_t GetTotalMemoryUsed();
uint64_t GetProcessMemoryUsed();
uint64_t GetPhysicalMemory();

// NOLINTBEGIN(modernize-use-nodiscard)
/// \brief BlockAllocator is mostly compliant with the C++ Standard Library allocators.
/// This means you can use it with <b>allocator_traits</b> http://www.cplusplus.com/reference/memory/allocator_traits/
/// or just like you would use the <b>std::allocator</b> http://www.cplusplus.com/reference/memory/allocator/.
/// There are some differences though:<br />
/// 	- Cannot allocate multiple objects with a single call to allocate and will
/// 	  simply ignore the count value you pass to the allocate/deallocate function.
/// 	  Fixing this is not too hard, but it would deteriorate performance and create memory
/// 	  fragmentation.
///		- This is <i>NOT</i> thread safe. You should create a different instance for each thread
/// 	  (suggested) or find some way of scheduling queries to the allocator.
///
/// <b>T</b> can be any object, while <b>BlockSize</b> needs to be at least twice the size of <b>T</b>.
///
/// <h3>Pick correct BlockSize</h3>
/// BlockSize is the size of the chunks in bytes the allocator will ask from the system.
/// It has to be large enough to contain at least two pointers or two T objects,
/// depending on which is bigger.
///
/// Picking the correct BlockSize is essential for good performance. I suggest you pick a power
/// of two, which may decrease memory fragmentation depending on your system. Also, make sure
/// that BlockSize is at least several hundred times larger than the size of T for maximum
/// performance. The idea is, the greater the BlockSize, the less calls to malloc the library
/// will make. However, picking a size too big might increase memory usage unnecessarily and
/// actually decrease the performance because malloc may need to make many system calls.
///
/// For objects that contain several pointers, the default size of 4096 bytes should be good.
/// If you need bigger object, you may need to time your code with larger sizes and see what
/// works best. Unless you will be maintaining many BlockAllocator objects, I do not think you need
/// to go smaller than 4096 bytes. Though if you are working on a more limited platform (that
/// has a compiler with C++11 support), you may need to go for smaller values.
template <typename T, size_t BlockSize = 4096>
class BlockAllocator
{
public:
	/* Member types */
	using value_type = T;
	using pointer = T*;
	using reference = T&;
	using const_pointer = const T*;
	using const_reference = const T&;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using propagate_on_container_copy_assignment = int;
	using propagate_on_container_move_assignment = int;
	using propagate_on_container_swap = int;

	template <typename U>
	struct rebind
	{
		using other = BlockAllocator<U>;
	};

	/* Member functions */
	BlockAllocator() noexcept;
	BlockAllocator(const BlockAllocator& memoryPool) noexcept;
	BlockAllocator(BlockAllocator&& memoryPool) noexcept;
	template <class U>
	explicit BlockAllocator(const BlockAllocator<U>& memoryPool) noexcept;

	~BlockAllocator() noexcept;

	BlockAllocator& operator=(const BlockAllocator& memoryPool) = delete;
	BlockAllocator& operator=(BlockAllocator&& memoryPool) noexcept;

	pointer address(reference x) const noexcept;
	const_pointer address(const_reference x) const noexcept;

	// Can only allocate one object at a time. n and hint are ignored
	pointer allocate(size_type n, const_pointer hint);
	void deallocate(pointer p, size_type n);

	size_type max_size() const noexcept;

	template <class U, class... Args>
	void construct(U* p, Args&&... args);
	template <class U>
	void destroy(U* p);

	template <class... Args>
	pointer newElement(Args&&... args);
	void deleteElement(pointer p);

private:
	union Slot
	{
		value_type element;
		Slot* next;
	};

	Slot *mCurrentBlock, mCurrentSlot, mLastSlot, mFreeSlots;

	size_type PadPointer(const char* p, size_type align) const noexcept;
	void AllocateBlock();

	static_assert(BlockSize >= 2 * sizeof(Slot), "BlockSize too small.");
};
// NOLINTEND(modernize-use-nodiscard)

template <typename T, size_t BlockSize>
inline typename BlockAllocator<T, BlockSize>::size_type
BlockAllocator<T, BlockSize>::PadPointer(const char* p, size_type align) const noexcept
{
	auto result = reinterpret_cast<uintptr_t>(p);
	return ((align - result) % align);
}

template <typename T, size_t BlockSize>
BlockAllocator<T, BlockSize>::BlockAllocator() noexcept
{
	mCurrentBlock = nullptr;
	mCurrentSlot.next = nullptr;
	mLastSlot.next = nullptr;
	mFreeSlots.next = nullptr;
}

template <typename T, size_t BlockSize>
BlockAllocator<T, BlockSize>::BlockAllocator(const BlockAllocator& memoryPool) noexcept
    : BlockAllocator()
{
	(void)memoryPool;
}

template <typename T, size_t BlockSize>
BlockAllocator<T, BlockSize>::BlockAllocator(BlockAllocator&& memoryPool) noexcept
{
	mCurrentBlock = memoryPool.mCurrentBlock;
	memoryPool.mCurrentBlock = nullptr;
	mCurrentSlot = memoryPool.mCurrentSlot;
	mLastSlot = memoryPool.mLastSlot;
	mFreeSlots = memoryPool.freeSlots;
}

template <typename T, size_t BlockSize>
template <class U>
BlockAllocator<T, BlockSize>::BlockAllocator(const BlockAllocator<U>& memoryPool) noexcept
    : BlockAllocator()
{
	(void)memoryPool;
}

template <typename T, size_t BlockSize>
BlockAllocator<T, BlockSize>&
BlockAllocator<T, BlockSize>::operator=(BlockAllocator&& memoryPool) noexcept
{
	if (this != &memoryPool)
	{
		std::swap(mCurrentBlock, memoryPool.mCurrentBlock);
		mCurrentSlot = memoryPool.mCurrentSlot;
		mLastSlot = memoryPool.mLastSlot;
		mFreeSlots = memoryPool.freeSlots;
	}
	return *this;
}

template <typename T, size_t BlockSize>
BlockAllocator<T, BlockSize>::~BlockAllocator() noexcept
{
	Slot* curr = mCurrentBlock;
	while (curr != nullptr)
	{
		Slot* prev = curr->next;
		operator delete(reinterpret_cast<void*>(curr));
		curr = prev;
	}
}

template <typename T, size_t BlockSize>
inline typename BlockAllocator<T, BlockSize>::pointer
BlockAllocator<T, BlockSize>::address(reference x) const noexcept
{
	return &x;
}

template <typename T, size_t BlockSize>
inline typename BlockAllocator<T, BlockSize>::const_pointer
BlockAllocator<T, BlockSize>::address(const_reference x) const noexcept
{
	return &x;
}

template <typename T, size_t BlockSize>
void
BlockAllocator<T, BlockSize>::AllocateBlock()
{
	// Allocate space for the new block and store a pointer to the previous one
	char* newBlock = reinterpret_cast<char*>(operator new(BlockSize));
	reinterpret_cast<Slot*>(newBlock)->next = mCurrentBlock;
	mCurrentBlock = reinterpret_cast<Slot*>(newBlock);
	// Pad block body to satisfy the alignment requirements for elements
	char* body = newBlock + sizeof(Slot*);
	size_type bodyPadding = PadPointer(body, alignof(Slot));
	mCurrentSlot = reinterpret_cast<Slot*>(body + bodyPadding);
	mLastSlot = reinterpret_cast<Slot*>(newBlock + BlockSize - sizeof(Slot) + 1);
}

template <typename T, size_t BlockSize>
inline typename BlockAllocator<T, BlockSize>::pointer
BlockAllocator<T, BlockSize>::allocate(size_type n, const_pointer hint)
{
	(void)n;
	(void)hint;
	if (mFreeSlots != nullptr)
	{
		auto result = reinterpret_cast<pointer>(mFreeSlots);
		mFreeSlots = mFreeSlots->next;
		return result;
	}
	else
	{
		if (mCurrentSlot >= mLastSlot) AllocateBlock();
		return reinterpret_cast<pointer>(mCurrentSlot++);
	}
}

template <typename T, size_t BlockSize>
inline void
BlockAllocator<T, BlockSize>::deallocate(pointer p, size_type n)
{
	if (p != nullptr)
	{
		reinterpret_cast<Slot*>(p)->next = mFreeSlots.next;
		mFreeSlots.next = reinterpret_cast<Slot*>(p);
	}
}

template <typename T, size_t BlockSize>
inline typename BlockAllocator<T, BlockSize>::size_type
BlockAllocator<T, BlockSize>::max_size() const noexcept
{
	size_type maxBlocks = -1 / BlockSize;
	return (BlockSize - sizeof(char*)) / sizeof(Slot) * maxBlocks;
}

template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
BlockAllocator<T, BlockSize>::construct(U* p, Args&&... args)
{
	new (p) U(std::forward<Args>(args)...);
}

template <typename T, size_t BlockSize>
template <class U>
inline void
BlockAllocator<T, BlockSize>::destroy(U* p)
{
	p->~U();
}

template <typename T, size_t BlockSize>
template <class... Args>
inline typename BlockAllocator<T, BlockSize>::pointer
BlockAllocator<T, BlockSize>::newElement(Args&&... args)
{
	pointer result = allocate();
	construct<value_type>(result, std::forward<Args>(args)...);
	return result;
}

template <typename T, size_t BlockSize>
inline void
BlockAllocator<T, BlockSize>::deleteElement(pointer p)
{
	if (p != nullptr)
	{
		p->~value_type();
		deallocate(p);
	}
}

inline uint64_t
GetSystemMemory()
{
#if defined(_WIN32) || defined(__CYGWIN__)
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	return static_cast<uint64_t>(memInfo.ullTotalPageFile);
#elif defined(__linux__)
	struct sysinfo memInfo;
	sysinfo(&memInfo);
	auto totalVirtualMem = memInfo.totalram;

	totalVirtualMem += memInfo.totalswap;
	totalVirtualMem *= memInfo.mem_unit;
	return static_cast<uint64_t>(totalVirtualMem);
#elif defined(__FreeBSD__)
	kvm_t* kd;
	u_int pageCnt;
	size_t pageCntLen = sizeof(pageCnt);
	u_int pageSize;
	struct kvm_swap kswap;
	uint64_t totalVirtualMem;

	pageSize = static_cast<u_int>(getpagesize());

	sysctlbyname("vm.stats.vm.v_page_count", &pageCnt, &pageCntLen, NULL, 0);
	totalVirtualMem = pageCnt * pageSize;

	kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
	kvm_getswapinfo(kd, &kswap, 1, 0);
	kvm_close(kd);
	totalVirtualMem += kswap.ksw_total * pageSize;

	return totalVirtualMem;
#else
	return 0;
#endif
}

inline uint64_t
GetTotalMemoryUsed()
{
#if defined(_WIN32) || defined(__CYGWIN__)
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	return static_cast<uint64_t>(memInfo.ullTotalPageFile - memInfo.ullAvailPageFile);
#elif defined(__linux__)
	struct sysinfo memInfo;
	sysinfo(&memInfo);
	auto virtualMemUsed = memInfo.totalram - memInfo.freeram;

	virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
	virtualMemUsed *= memInfo.mem_unit;

	return static_cast<uint64_t>(virtualMemUsed);
#elif defined(__FreeBSD__)
	kvm_t* kd;
	u_int pageSize;
	u_int pageCnt, freeCnt;
	size_t pageCntLen = sizeof(pageCnt);
	size_t freeCntLen = sizeof(freeCnt);
	struct kvm_swap kswap;
	uint64_t virtualMemUsed;

	pageSize = static_cast<u_int>(getpagesize());

	sysctlbyname("vm.stats.vm.v_page_count", &pageCnt, &pageCntLen, NULL, 0);
	sysctlbyname("vm.stats.vm.v_free_count", &freeCnt, &freeCntLen, NULL, 0);
	virtualMemUsed = (pageCnt - freeCnt) * pageSize;

	kd = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, "kvm_open");
	kvm_getswapinfo(kd, &kswap, 1, 0);
	kvm_close(kd);
	virtualMemUsed += kswap.ksw_used * pageSize;

	return virtualMemUsed;
#else
	return 0;
#endif
}

inline uint64_t
GetProcessMemoryUsed()
{
#if defined(_WIN32) || defined(__CYGWIN__)
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(
	    GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&pmc), sizeof(pmc));
	return static_cast<uint64_t>(pmc.PrivateUsage);
#elif defined(__linux__)
	auto parseLine = [](char* line) -> int {
		auto i = strlen(line);

		while (*line < '0' || *line > '9') { line++; }

		line[i - 3] = '\0';
		i = atoi(line);
		return i;
	};

	auto file = fopen("/proc/self/status", "r");
	auto result = -1;
	char line[128];

	while (fgets(line, 128, file) != nullptr)
	{
		if (strncmp(line, "VmSize:", 7) == 0)
		{
			result = parseLine(line);
			break;
		}
	}

	fclose(file);
	return static_cast<uint64_t>(result) * 1024;
#else
	return 0;
#endif
}

inline uint64_t
GetPhysicalMemory()
{
#if defined(_WIN32) || defined(__CYGWIN__)
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	return static_cast<uint64_t>(memInfo.ullTotalPhys);
#elif defined(__linux__)
	struct sysinfo memInfo;
	sysinfo(&memInfo);

	auto totalPhysMem = memInfo.totalram;

	totalPhysMem *= memInfo.mem_unit;
	return static_cast<uint64_t>(totalPhysMem);
#else
	return 0;
#endif
}

} // namespace ie

#endif // IE_MEMORY_HPP
