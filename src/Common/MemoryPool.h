#pragma once

#include <cstddef>
#include "common/glsp_spinlock.h"


namespace glsp {

class RegisterPool
{
public:
	struct MemBlock;
	struct FreeListNode;
	struct ThreadMetaData;

	static const int kBlockNum = 4;
	static const int kBlockSize = 32 * 1024;
	static constexpr size_t kUnitSizes[kBlockNum] = {32, 64, 128, 256};

	static RegisterPool& get()
	{
		static RegisterPool pool;
		return pool;
	}

	void* allocate(const size_t size);
	void deallocate(void * const p, const size_t size);

private:
	RegisterPool();
	~RegisterPool() = default;


	FreeListNode *mGlobalCache;
	SpinLock      mPoolLock;
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
		return static_cast<pointer>(RegisterPool::get().allocate(n * sizeof(T)));
	}

	void deallocate(T * const p, const size_t n) const
	{
		RegisterPool::get().deallocate(p, n * sizeof(T));
	}

	template <typename U> T * allocate(const size_t n, const U * /* const hint */) const
	{
		return allocate(n);
	}

	ShaderRegisterAllocator& operator=(const ShaderRegisterAllocator&) = delete;

private:
};

} // namespace glsp
