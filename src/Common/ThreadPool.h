#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <stack>
#include <thread>

#include "common/glsp_spinlock.h"



namespace glsp {

class WorkItem;
class ThreadPool;

class WorkItem
{
public:
	friend class ThreadPool;

	typedef std::function<void (void *)> callback_t;

private:
	WorkItem() = default;
	~WorkItem() = default;

private:
	callback_t mCallback;
	void *mData;
};


class ThreadPool
{
public:
	static ThreadPool& get()
	{
		static ThreadPool instance;
		return instance;
	}

	bool IsInitialized() const { return bInitialzed; }

	WorkItem* CreateWork(const WorkItem::callback_t &fn, void *data);

	bool AddWork(WorkItem *work);

	uint32_t getDoneWorks() const { return mDoneWorks; }

	int getThreadsNumber() const { return mThreadsNum; }
	static int getThreadID();

	/* NOTE:
	 * This method should be called only after all producer threads
	 * have dispatched all pending tasks.
	 */
	void waitForAllTaskDone();

protected:
	ThreadPool();
	~ThreadPool();

	ThreadPool(const ThreadPool &rhs) = delete;
	ThreadPool& operator=(const ThreadPool &rhs) = delete;

	bool Initialize();

private:
	std::stack<WorkItem *>	mWorkPool;
	std::deque<WorkItem *>	mWorkQueue;

	uint32_t				mDoneWorks;
	uint32_t				mRunningWorks;

	// TODO (Done): replace mutex with spinlock
	SpinLock					mQueueLock;
	std::condition_variable_any	mWorkQueuedCond;
	std::condition_variable_any	mAllTaskDoneCond;

	int				mThreadsNum;
	std::thread    *mThreads;

	bool bInitialzed;
	bool bIsFinalizing;
};

} // namespace glsp
