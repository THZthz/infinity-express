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
/*----------------------------------------------------------------------------*/

namespace ie {

inline void*
_malloc(size_t size)
{
	void* ptr = malloc(size);
	return ptr;
}

inline void*
_realloc(void* ptr, size_t size)
{
	void* n_ptr = realloc(ptr, size);
	assert(n_ptr);
	return n_ptr;
}

inline void
_free(void* ptr)
{
	free(ptr);
}

inline void*
_calloc(size_t n, size_t size)
{
	void* ptr = calloc(n, size);
	assert(ptr);
	return ptr;
}

} // namespace ie
/*----------------------------------------------------------------------------*/

namespace ie {

inline uint64_t
getSystemMemory()
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
getTotalMemoryUsed()
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
getProcessMemoryUsed()
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
getPhysicalMemory()
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
/*----------------------------------------------------------------------------*/
namespace ie {

template <typename T, size_t BlockSize = 4096>
class block_allocator
{
public:
	using value_type = T;
	using pointer = T*;
	using reference = T&;
	using const_pointer = const T*;
	using const_reference = const T&;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using propagate_on_container_copy_assignment = std::false_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_swap = std::true_type;

	template <typename U>
	struct rebind
	{
		typedef block_allocator<U> other;
	};

	block_allocator() noexcept;
	block_allocator(const block_allocator& memoryPool) noexcept;
	block_allocator(block_allocator&& memoryPool) noexcept;
	template <class U>
	explicit block_allocator(const block_allocator<U>& memoryPool) noexcept;

	~block_allocator() noexcept;

	block_allocator& operator=(const block_allocator& memoryPool) = delete;
	block_allocator& operator=(block_allocator&& memoryPool) noexcept;

	pointer address(reference x) const noexcept;
	const_pointer address(const_reference x) const noexcept;

	// can only allocate one object at a time. n and hint are ignored
	pointer allocate(size_type n = 1, const_pointer hint = nullptr);
	void deallocate(pointer p, size_type n = 1);

	size_type max_size() const noexcept;

	template <class U, class... Args>
	void construct(U* p, Args&&... args);
	template <class U>
	void destroy(U* p);

	template <class... Args>
	pointer new_element(Args&&... args);
	void delete_element(pointer p);

private:
	union Slot
	{
		value_type element;
		Slot* next;
	};

	typedef char* data_pointer;
	typedef Slot slot_type;
	typedef Slot* slot_pointer;

	slot_pointer m_currentBlock;
	slot_pointer m_currentSlot;
	slot_pointer m_lastSlot;
	slot_pointer m_freeSlots;

	size_type pad_pointer(data_pointer p, size_type align) const noexcept;
	void allocateBlock();

	static_assert(BlockSize >= 2 * sizeof(slot_type), "BlockSize too small.");
};

template <typename T, size_t BlockSize>
inline typename block_allocator<T, BlockSize>::size_type
block_allocator<T, BlockSize>::pad_pointer(data_pointer p, size_type align) const noexcept
{
	auto result = reinterpret_cast<uintptr_t>(p);
	return ((align - result) % align);
}

template <typename T, size_t BlockSize>
block_allocator<T, BlockSize>::block_allocator() noexcept
{
	m_currentBlock = nullptr;
	m_currentSlot = nullptr;
	m_lastSlot = nullptr;
	m_freeSlots = nullptr;
}

template <typename T, size_t BlockSize>
block_allocator<T, BlockSize>::block_allocator(const block_allocator& memoryPool) noexcept
    : block_allocator()
{
}

template <typename T, size_t BlockSize>
block_allocator<T, BlockSize>::block_allocator(block_allocator&& memoryPool) noexcept
{
	m_currentBlock = memoryPool.m_currentBlock;
	memoryPool.m_currentBlock = nullptr;
	m_currentSlot = memoryPool.m_currentSlot;
	m_lastSlot = memoryPool.m_lastSlot;
	m_freeSlots = memoryPool.freeSlots;
}

template <typename T, size_t BlockSize>
template <class U>
block_allocator<T, BlockSize>::block_allocator(const block_allocator<U>& memoryPool) noexcept
    : block_allocator()
{
}

template <typename T, size_t BlockSize>
block_allocator<T, BlockSize>&
block_allocator<T, BlockSize>::operator=(block_allocator&& memoryPool) noexcept
{
	if (this != &memoryPool)
	{
		std::swap(m_currentBlock, memoryPool.m_currentBlock);
		m_currentSlot = memoryPool.m_currentSlot;
		m_lastSlot = memoryPool.m_lastSlot;
		m_freeSlots = memoryPool.freeSlots;
	}
	return *this;
}

template <typename T, size_t BlockSize>
block_allocator<T, BlockSize>::~block_allocator() noexcept
{
	slot_pointer curr = m_currentBlock;
	while (curr != nullptr)
	{
		slot_pointer prev = curr->next;
		operator delete(reinterpret_cast<void*>(curr));
		curr = prev;
	}
}

template <typename T, size_t BlockSize>
inline typename block_allocator<T, BlockSize>::pointer
block_allocator<T, BlockSize>::address(reference x) const noexcept
{
	return &x;
}

template <typename T, size_t BlockSize>
inline typename block_allocator<T, BlockSize>::const_pointer
block_allocator<T, BlockSize>::address(const_reference x) const noexcept
{
	return &x;
}

template <typename T, size_t BlockSize>
void
block_allocator<T, BlockSize>::allocateBlock()
{
	// Allocate space for the new block and store a pointer to the previous one
	auto newBlock = reinterpret_cast<data_pointer>(operator new(BlockSize));
	reinterpret_cast<slot_pointer>(newBlock)->next = m_currentBlock;
	m_currentBlock = reinterpret_cast<slot_pointer>(newBlock);
	// Pad block body to satisfy the alignment requirements for elements
	data_pointer body = newBlock + sizeof(slot_pointer);
	size_type bodyPadding = pad_pointer(body, alignof(slot_type));
	m_currentSlot = reinterpret_cast<slot_pointer>(body + bodyPadding);
	m_lastSlot = reinterpret_cast<slot_pointer>(newBlock + BlockSize - sizeof(slot_type) + 1);
}

template <typename T, size_t BlockSize>
inline typename block_allocator<T, BlockSize>::pointer
block_allocator<T, BlockSize>::allocate(size_type n, const_pointer hint)
{
	if (m_freeSlots != nullptr)
	{
		auto result = reinterpret_cast<pointer>(m_freeSlots);
		m_freeSlots = m_freeSlots->next;
		return result;
	}
	else
	{
		if (m_currentSlot >= m_lastSlot) allocateBlock();
		return reinterpret_cast<pointer>(m_currentSlot++);
	}
}

template <typename T, size_t BlockSize>
inline void
block_allocator<T, BlockSize>::deallocate(pointer p, size_type n)
{
	if (p != nullptr)
	{
		reinterpret_cast<slot_pointer>(p)->next = m_freeSlots;
		m_freeSlots = reinterpret_cast<slot_pointer>(p);
	}
}

template <typename T, size_t BlockSize>
inline typename block_allocator<T, BlockSize>::size_type
block_allocator<T, BlockSize>::max_size() const noexcept
{
	size_type maxBlocks = -1 / BlockSize;
	return (BlockSize - sizeof(data_pointer)) / sizeof(slot_type) * maxBlocks;
}

template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
block_allocator<T, BlockSize>::construct(U* p, Args&&... args)
{
	new (p) U(std::forward<Args>(args)...);
}

template <typename T, size_t BlockSize>
template <class U>
inline void
block_allocator<T, BlockSize>::destroy(U* p)
{
	p->~U();
}

template <typename T, size_t BlockSize>
template <class... Args>
inline typename block_allocator<T, BlockSize>::pointer
block_allocator<T, BlockSize>::new_element(Args&&... args)
{
	pointer result = allocate();
	construct<value_type>(result, std::forward<Args>(args)...);
	return result;
}

template <typename T, size_t BlockSize>
inline void
block_allocator<T, BlockSize>::delete_element(pointer p)
{
	if (p != nullptr)
	{
		p->~value_type();
		deallocate(p);
	}
}

} // namespace ie
/*----------------------------------------------------------------------------*/

#endif // IE_MEMORY_HPP
