#include "ThreadPool.h"

#include <cassert>
#include <mutex>
#include <utility>

#ifdef __linux__
#include <unistd.h>
#endif


namespace glsp {

ThreadPool::ThreadPool():
	mDoneWorks(0),
	mRunningWorks(0),
	mThreadsNum(0),
	mThreads(NULL),
	bInitialzed(false),
	bIsFinalizing(false)
{
}

ThreadPool::~ThreadPool()
{
	std::unique_lock<SpinLock> lk(mQueueLock);

	bIsFinalizing = true;

	lk.unlock();

	waitForAllTaskDone();

	mWorkQueuedCond.notify_all();

	for(int i = 0; i < mThreadsNum; ++i)
	{
		mThreads[i].join();
	}

	delete []mThreads;

	while(!mWorkPool.empty())
	{
		WorkItem *pWork = mWorkPool.top();
		mWorkPool.pop();
		delete pWork;
	}
}

bool ThreadPool::Initialize()
{
	int n;
	bool ret = true;

	n = std::thread::hardware_concurrency();

#ifdef __linux__
	if(!n)
		n = sysconf(_SC_NPROCESSORS_ONLN);
#endif

	if(!n)
	{
		bInitialzed = ret = false;
		return ret;
	}

	mThreadsNum = n;

	mThreads = new std::thread[n];

	auto workerThread = [this]
	{
		while(true)
		{
			std::unique_lock<SpinLock> lk(mQueueLock);
	
			if(mWorkQueue.empty())
			{
				mWorkQueuedCond.wait(lk);
			}

			// This thread may be wake up by terminate signal.
			if(bIsFinalizing)
				break;

			if(mWorkQueue.empty())
				continue;

			mRunningWorks++;
			WorkItem *pWork = mWorkQueue.front();
			mWorkQueue.pop_front();

			lk.unlock();

			pWork->mCallback(pWork->mData);

			lk.lock();
			mRunningWorks--;
			mDoneWorks++;
			assert(mRunningWorks >= 0 && mRunningWorks <= (uint32_t)mThreadsNum);
			mWorkPool.push(pWork);
		}
	};

	for(int i = 0; i < n; ++i)
		mThreads[i] = std::thread(workerThread);

	return bInitialzed = ret;
}

WorkItem* ThreadPool::CreateWork(const WorkItem::callback_t &work, void *data)
{
	std::lock_guard<SpinLock> lk(mQueueLock);

	if(bIsFinalizing)
		return NULL;

	WorkItem *pWork;

	if(mWorkPool.empty())
	{
		pWork = new WorkItem;
	}
	else
	{
		pWork = mWorkPool.top();
		mWorkPool.pop();
	}

	pWork->mCallback = work;
	pWork->mData	 = data;
	return pWork;
}

bool ThreadPool::AddWork(WorkItem *work)
{
	std::lock_guard<SpinLock> lk(mQueueLock);

	if(bIsFinalizing || !work)
		return false;

	mWorkQueue.push_back(work);

	mWorkQueuedCond.notify_one();

	return true;
}

} // namespace glsp
