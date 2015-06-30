#include "ThreadPool.h"

#include <cassert>
#include <utility>

#ifdef __linux__
#include <unistd.h>
#endif


namespace glsp {

WorkItem::WorkItem(callback_t &&fn, void *data):
	mCallback(std::move(fn)),
	mData(data)
{
}

ThreadPool::ThreadPool():
	mWorkPoolSize(0),
	mWorkPool(NULL),
	mLastQueuedWork(NULL),
	mDoneWorks(0),
	mThreadsNum(0),
	mThreads(NULL),
	bInitialzed(false),
	bIsFinalizing(false),
	bAllWorkDone(false)
{
}

ThreadPool::~ThreadPool()
{
	std::unique_lock<std::mutex> lk(mQueueLock);

	bIsFinalizing = true;

	lk.unlock();

	while(!bAllWorkDone)
	{
		std::this_thread::yield();
	}

	mWorkQueuedCond.notify_all();

	for(int i = 0; i < mThreadsNum; ++i)
	{
		mThreads[i].join();
	}

	delete []mThreads;
	delete []mWorkPool;
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

	// Alloc double size of the threads number
	mWorkPool = (WorkItem *)malloc(sizeof(WorkItem) * n * 2);

	auto workerThread = [this]
	{
		while(true)
		{
			std::unique_lock<std::mutex> lk(mQueueLock);
	
			if(mWorkQueue.empty())
			{
				mWorkQueuedCond.wait(lk);
			}

			// This thread may be wake up by terminate signal.
			if(bIsFinalizing)
				break;

			WorkItem *pWork = mWorkQueue.front();
			mWorkQueue.pop_front();
			lk.unlock();

			assert(pWork);

			pWork->mCallback(pWork->mData);

			lk.lock();
			mDoneWorks++;

			if(mLastQueuedWork == pWork)
				bAllWorkDone = true;

			lk.unlock();
			delete pWork;
		}
	};

	for(int i = 0; i < n; ++i)
		mThreads[i] = std::thread(workerThread);

	return bInitialzed = ret;
}

WorkItem* ThreadPool::CreateWork(WorkItem::callback_t &&work, void *data)
{
	// FIXME:
	// Placement new is also thread safe
	// But it's this very peculiarity, the performance may be bad.
	while(true)
	{
		WorkItem *pWork = new(mWorkPool) WorkItem(std::move(work), data);

		if(pWork)
			return pWork;

		// FIXME: find a better way
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

bool ThreadPool::AddWork(WorkItem *work)
{
	std::unique_lock<std::mutex> lk(mQueueLock);

	if(bIsFinalizing || !work)
		return false;

	bAllWorkDone = false;
	mLastQueuedWork = work;
	mWorkQueue.push_back(work);

	mWorkQueuedCond.notify_one();

	return true;
}

} // namespace glsp