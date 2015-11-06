#include "ThreadPool.h"

#include <cassert>
#include <mutex>
#include <utility>

#include "compiler.h"
#include "os.h"


namespace glsp {

static std::atomic_int sID(0);
static THREAD_LOCAL int s_TlsId = 0;

ThreadPool::ThreadPool():
	mDoneWorks(0),
	mRunningWorks(0),
	mThreadsNum(0),
	mThreads(NULL),
	bInitialzed(false),
	bIsFinalizing(false)
{
	int n;

	n = std::thread::hardware_concurrency();

	if (!n)
		n = OSGetProcessorsNum();

	s_TlsId = n;
	mThreadsNum = n;
	mThreads = new std::thread[n];

	Initialize();
}

ThreadPool::~ThreadPool()
{
	std::unique_lock<SpinLock> lk(mQueueLock);

	bIsFinalizing = true;

	lk.unlock();

	waitForAllTaskDone();

	mWorkQueuedCond.notify_all();

	for (int i = 0; i < mThreadsNum; ++i)
	{
		mThreads[i].join();
	}

	delete []mThreads;

	while (!mWorkPool.empty())
	{
		WorkItem *pWork = mWorkPool.top();
		mWorkPool.pop();
		delete pWork;
	}
}

bool ThreadPool::Initialize()
{
	auto workerThread = [this]
	{
		s_TlsId = sID.fetch_add(1);
		while (true)
		{
			std::unique_lock<SpinLock> lk(mQueueLock);
	
			if (mWorkQueue.empty())
			{
				mWorkQueuedCond.wait(lk);
			}

			// This thread may be wake up by terminate signal.
			if (bIsFinalizing)
				break;

			if (mWorkQueue.empty())
				continue;

			mRunningWorks++;
			WorkItem *pWork = mWorkQueue.front();
			mWorkQueue.pop_front();

			lk.unlock();

			pWork->mCallback(pWork->mData);

			lk.lock();
			mDoneWorks++;
			assert(mRunningWorks > 0 && mRunningWorks <= (uint32_t)mThreadsNum);

			if (--mRunningWorks == 0 && mWorkQueue.empty())
				mAllTaskDoneCond.notify_one();

			mWorkPool.push(pWork);
		}
	};

	for (int i = 0; i < mThreadsNum; ++i)
		mThreads[i] = std::thread(workerThread);

	return true;
}

WorkItem* ThreadPool::CreateWork(const WorkItem::callback_t &work, void *data)
{
	std::lock_guard<SpinLock> lk(mQueueLock);

	if (bIsFinalizing)
		return NULL;

	WorkItem *pWork;

	if (mWorkPool.empty())
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

	if (bIsFinalizing || !work)
		return false;

	mWorkQueue.push_back(work);

	mWorkQueuedCond.notify_one();

	return true;
}

void ThreadPool::waitForAllTaskDone()
{
	std::unique_lock<SpinLock> lk(mQueueLock);

	while (!mWorkQueue.empty() || mRunningWorks)
		mAllTaskDoneCond.wait(lk);
}

int ThreadPool::getThreadID()
{
	return s_TlsId;
}

} // namespace glsp
