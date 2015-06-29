#include "ThreadPool.h"


namespace glsp {

ThreadPool::ThreadPool():
	mWorkPoolSize(0),
	mWorkPool(NULL),
	mThreadsNum(0),
	mThreads(NULL)
{
}

ThreadPool::~ThreadPool()
{
}

bool ThreadPool::Initialize()
{
	int n;
	bool ret = true;

	n = std::thread::hardware_concurrency();

	if(!n)
	{
		n = sysconf(_SC_NPROCESSORS_ONLN);
		ret = false;

		goto out;
	}

	mThreadsNum = n;

	mThreads = new std::thread[n];

	auto workerFunc = [this]
	{
		while(true)
		{
			std::unique_lock<std::mutex> lock(mQueueLock);
	
			if(mWorkQueue.empty())
				mCond.wait(lock, [this]{ return !mWorkQueue.empty(); } );
	
			WorkItem *pWork = mWorkQueue.front();
			mWorkQueue = mWorkQueue.pop_front();
			lock.unlock();

			if(!pWork)
				pWork->mCallback(pWork->data);
		}
	}

	for(int i = 0; i < n; ++i)
		mThreads[i] = std::thread(workerFunc);

out:
	return bInitialzed = ret;
}



}