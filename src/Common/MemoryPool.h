#pragma once

#include <cstddef>
#include "common/glsp_spinlock.h"


namespace glsp {

/* Mutil-threaded memory pool:
 * The basic data structure is a per-thread memory free list.
 *
 * It's common that memory will be allocated from one thread,
 * and then freed in other threads.
 * So a global memory cache is instroduced to handle the memory balance.
 * Without this, the memory metrics in one thread will explode.
 *
 * The pool is implemented nearly as lock-free.
 * It only needs protection when interacting with the global cache.
 * And its probability of occurrence is below 1/FREE_TO_GLOBAL_CACHE_THRESHHOLD.
 *
 * At last, it implements a BoostReclaimAll() interface to free all memory
 * in one pass, avoid trivial free of small pieces.
 */
class MemoryPoolMT
{
public:
	struct MemBlock;
	struct FreeListNode;
	struct ThreadMetaData;

	static const int kBlockNum = 5;

	static constexpr int    kBlockSize[kBlockNum] = {8 * 1024, 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024};
	static constexpr size_t kUnitSizes[kBlockNum] = {16, 32, 64, 128, 256};

	static MemoryPoolMT& get()
	{
		static MemoryPoolMT pool;
		return pool;
	}

	void*  allocate(const size_t size);
	void deallocate(void * const p, const size_t size);

	void BoostReclaimAll();

private:
	MemoryPoolMT();
	~MemoryPoolMT() = default;


	FreeListNode *mGlobalCache[kBlockNum];
	SpinLock      mPoolLock[kBlockNum];
};

template <typename T>
class ShaderRegisterAllocator
{
public:
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	pointer address(reference r) const
	{
		return &r;
	}

	const_pointer address(const_reference s) const
	{
		return &s;
	}

	size_type max_size() const
	{
		return 0xFFFFFFFFUL / sizeof(T);
	}

	template <typename U> struct rebind {
		typedef ShaderRegisterAllocator<U> other;
	};

	bool operator!=(const ShaderRegisterAllocator& other) const
	{
		return !(*this == other);
	}

	void construct(T * const p) const
	{
		void * const pv = static_cast<void *>(p);
		new (pv) T();
	}

	bool operator==(const ShaderRegisterAllocator& other) const
	{
		return true;
	}

	ShaderRegisterAllocator() { }
	ShaderRegisterAllocator(const ShaderRegisterAllocator&) { }
	template <typename U> ShaderRegisterAllocator(const ShaderRegisterAllocator<U>&) { }
	~ShaderRegisterAllocator() { }

	pointer allocate(size_type n) const
	{
		return static_cast<pointer>(MemoryPoolMT::get().allocate(n * sizeof(T)));
	}

	void deallocate(T * const p, const size_t n) const
	{
		MemoryPoolMT::get().deallocate(p, n * sizeof(T));
	}

	template <typename U> T * allocate(const size_t n, const U * /* const hint */) const
	{
		return allocate(n);
	}

	ShaderRegisterAllocator& operator=(const ShaderRegisterAllocator&) = delete;

private:
};

} // namespace glsp


/* NOTE:
 * Like the placement new, caller needs to explicitly invoke the
 * destructor before deallocate.
 */
void* operator new(const size_t size, ::glsp::MemoryPoolMT &pool);
