#include "MemoryPool.h"

#include <cstring>
#include <mutex>
#include <common/glsp_debug.h>


namespace glsp {

/* OPT: need tuning
 * Larger means less spinlock operations,
 * but may also waste more memory at the same time.
 */
#define FREE_TO_GLOBAL_CACHE_THRESHHOLD 256


struct RegisterPool::MemBlock
{
	void 	 *mData;
	void 	 *mCurrent;
	MemBlock *mNext;
};

struct RegisterPool::FreeListNode
{
	FreeListNode *mNext;
	FreeListNode *mNextBatch;
};

struct RegisterPool::ThreadMetaData
{
	MemBlock         *mBlocks[kBlockNum];
	FreeListNode     *mFreeLists[kBlockNum];
	int               mAllocCounter[kBlockNum];
};


constexpr size_t RegisterPool::kUnitSizes[4];

// Use "__thread" instead of "thread_local" due to performance concern.
static __thread RegisterPool::ThreadMetaData sThreadMetaData;



static void ByeTMD(void)
{
	for(int i = 0; i < 4; ++i)
	{
		RegisterPool::MemBlock *pBlock = sThreadMetaData.mBlocks[i];

		while (pBlock)
		{
			RegisterPool::MemBlock *prev = pBlock;
			free(pBlock->mData);
			GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "free register block for idx %d\n", i);

			pBlock = pBlock->mNext;
			free(prev);
		}
	}
}

RegisterPool::RegisterPool():
	mGlobalCache(nullptr)
{
	atexit(ByeTMD);
}

void* RegisterPool::allocate(const size_t size)
{
	// No more than 16 attributes.
	assert(size <= 16 * 4 * 4);
	assert(size >= sizeof(void *) * 2);

	int idx;
	void *ptr;

	for (idx = 0; idx < kBlockNum; ++idx)
	{
		if(size <= kUnitSizes[idx])
			break;
	}
	sThreadMetaData.mAllocCounter[idx]++;

	MemBlock* &pBlock = sThreadMetaData.mBlocks[idx];
	FreeListNode* &fl = sThreadMetaData.mFreeLists[idx];

	if (fl)
	{
		ptr = fl;
		fl = ((FreeListNode *)ptr)->mNext;
		return ptr;
	}
	else if (pBlock && (uintptr_t)pBlock->mCurrent < (uintptr_t)pBlock->mData + kBlockSize)
	{
		ptr = pBlock->mCurrent;
		pBlock->mCurrent = (void *)((uintptr_t)pBlock->mCurrent + kUnitSizes[idx]);
		return ptr;
	}
	else if (!pBlock)
	{
		goto new_block;
	}

	{
		std::unique_lock<SpinLock> lk(mPoolLock);
		if (mGlobalCache)
		{
			ptr = mGlobalCache;
			mGlobalCache = mGlobalCache->mNextBatch;

			lk.unlock();
			fl = ((FreeListNode *)ptr)->mNext;
			sThreadMetaData.mAllocCounter[idx] -= FREE_TO_GLOBAL_CACHE_THRESHHOLD;
			return ptr;
		}
	}

new_block:
	MemBlock *pNewBlock = (MemBlock *)malloc(sizeof(MemBlock));
	pNewBlock->mData = malloc(kBlockSize);
	assert(pNewBlock->mData);

	GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "allocate register block for idx %d\n", idx);

	pNewBlock->mCurrent = (void *)((uintptr_t)pNewBlock->mData + kUnitSizes[idx]);
	pNewBlock->mNext = pBlock;
	pBlock = pNewBlock;

	return pNewBlock->mData;
}

void RegisterPool::deallocate(void * const p, const size_t size)
{
	// No more than 16 attributes.
	assert(size <= 16 * 4 * 4);
	assert(size >= sizeof(void *) * 2);

	int idx;

	for (idx = 0; idx < kBlockNum; ++idx)
	{
		if(size <= kUnitSizes[idx])
			break;
	}

	((FreeListNode *const)(p))->mNext = sThreadMetaData.mFreeLists[idx];

	if((--sThreadMetaData.mAllocCounter[idx]) > (-FREE_TO_GLOBAL_CACHE_THRESHHOLD))
	{
		sThreadMetaData.mFreeLists[idx] = (FreeListNode *)p;
	}
	else
	{
		{
			std::lock_guard<SpinLock> lk(mPoolLock);
			((FreeListNode *const)(p))->mNextBatch = mGlobalCache;
			mGlobalCache = (FreeListNode *)p;
		}
		sThreadMetaData.mFreeLists[idx] = nullptr;
		sThreadMetaData.mAllocCounter[idx] = 0;
	}
}

} // namespace glsp