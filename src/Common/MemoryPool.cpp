#include "MemoryPool.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>
#include "common/glsp_debug.h"


namespace glsp {

/* OPT: need tuning
 * Larger means less spinlock operations,
 * but may also waste more memory at the same time.
 */
#define FREE_TO_GLOBAL_CACHE_THRESHHOLD 64


struct MemoryPoolMT::MemBlock
{
	void 	 *mData;
	void 	 *mCurrent;
	MemBlock *mNext;
};

struct MemoryPoolMT::FreeListNode
{
	FreeListNode *mNext;
	FreeListNode *mNextBatch;
};

struct MemoryPoolMT::ThreadMetaData
{
	MemBlock         *mBlocks[kBlockNum];
	FreeListNode     *mFreeLists[kBlockNum];
	int               mAllocCounter[kBlockNum];
};


constexpr int    MemoryPoolMT::kBlockSize[MemoryPoolMT::kBlockNum];
constexpr size_t MemoryPoolMT::kUnitSizes[MemoryPoolMT::kBlockNum];

// Use "__thread" instead of "thread_local" due to performance concern.
static __thread MemoryPoolMT::ThreadMetaData s_ThreadMetaData;
static std::vector<MemoryPoolMT::ThreadMetaData *> s_TMDList;
static SpinLock s_TMDListLock;


static void ByeTMD(void)
{
	for(int i = 0; i < MemoryPoolMT::kBlockNum; ++i)
	{
		MemoryPoolMT::MemBlock *pBlock = s_ThreadMetaData.mBlocks[i];

		while (pBlock)
		{
			MemoryPoolMT::MemBlock *prev = pBlock;
			free(pBlock->mData);
			GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "free register block for idx %d\n", i);

			pBlock = pBlock->mNext;
			free(prev);
		}
	}
}

MemoryPoolMT::MemoryPoolMT()
{
	std::memset(mGlobalCache, 0, sizeof(mGlobalCache));
	atexit(ByeTMD);
}

void* MemoryPoolMT::allocate(const size_t size)
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
	s_ThreadMetaData.mAllocCounter[idx]++;

	MemBlock* &pBlock = s_ThreadMetaData.mBlocks[idx];
	FreeListNode* &fl = s_ThreadMetaData.mFreeLists[idx];

	if (fl)
	{
		ptr = fl;
		fl = ((FreeListNode *)ptr)->mNext;
		return ptr;
	}

	MemBlock *blk = pBlock;
	while (blk)
	{
		if (blk && (uintptr_t)blk->mCurrent < (uintptr_t)blk->mData + kBlockSize[idx])
		{
			ptr = blk->mCurrent;
			blk->mCurrent = (void *)((uintptr_t)blk->mCurrent + kUnitSizes[idx]);
			return ptr;
		}
		blk = blk->mNext;
	}

	if (!pBlock)
	{
		auto it = std::find(s_TMDList.begin(), s_TMDList.end(), &s_ThreadMetaData);
		if (it == s_TMDList.end())
		{
			std::unique_lock<SpinLock> lk(s_TMDListLock);
			s_TMDList.push_back(&s_ThreadMetaData);
		}

		goto new_block;
	}

	{
		std::unique_lock<SpinLock> lk(mPoolLock[idx]);
		if (mGlobalCache[idx])
		{
			ptr = mGlobalCache[idx];
			mGlobalCache[idx] = mGlobalCache[idx]->mNextBatch;

			lk.unlock();
			fl = ((FreeListNode *)ptr)->mNext;
			s_ThreadMetaData.mAllocCounter[idx] -= FREE_TO_GLOBAL_CACHE_THRESHHOLD;
			return ptr;
		}
	}

new_block:
	MemBlock *pNewBlock = (MemBlock *)malloc(sizeof(MemBlock));
	pNewBlock->mData    = malloc(kBlockSize[idx]);
	assert(pNewBlock->mData);

	GLSP_DPF(GLSP_DPF_LEVEL_DEBUG, "allocate register block for idx %d\n", idx);

	pNewBlock->mCurrent = (void *)((uintptr_t)pNewBlock->mData + kUnitSizes[idx]);
	pNewBlock->mNext    = pBlock;
	pBlock              = pNewBlock;

	return pNewBlock->mData;
}

void MemoryPoolMT::deallocate(void * const p, const size_t size)
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

	((FreeListNode *const)(p))->mNext = s_ThreadMetaData.mFreeLists[idx];

	if((--s_ThreadMetaData.mAllocCounter[idx]) > (-FREE_TO_GLOBAL_CACHE_THRESHHOLD))
	{
		s_ThreadMetaData.mFreeLists[idx] = (FreeListNode *)p;
	}
	else
	{
		{
			std::lock_guard<SpinLock> lk(mPoolLock[idx]);
			((FreeListNode *const)(p))->mNextBatch = mGlobalCache[idx];
			mGlobalCache[idx] = (FreeListNode *)p;
		}
		s_ThreadMetaData.mFreeLists[idx]    = nullptr;
		s_ThreadMetaData.mAllocCounter[idx] = 0;
	}
}

void MemoryPoolMT::BoostReclaimAll()
{
	for (ThreadMetaData *tmd: s_TMDList)
	{
		for(int i = 0; i < kBlockNum; ++i)
		{
			MemBlock *pBlock = tmd->mBlocks[i];
			while (pBlock)
			{
				pBlock->mCurrent = pBlock->mData;
				pBlock           = pBlock->mNext;
			}

			tmd->mFreeLists[i]    = nullptr;
			tmd->mAllocCounter[i] = 0;
		}
	}
	std::memset(mGlobalCache, 0, sizeof(mGlobalCache));
}

} // namespace glsp

void* operator new(const size_t size, ::glsp::MemoryPoolMT &pool)
{
	return pool.allocate(size);
}
