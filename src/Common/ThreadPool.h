#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stack>
#include <thread>
#include <boost/serialization/singleton.hpp>


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


class ThreadPool: public boost::serialization::singleton<ThreadPool>
{
public:
	static ThreadPool& get()
	{
		return get_mutable_instance();
	}

	bool Initialize();

	bool IsInitialized() const { return bInitialzed; }

	WorkItem* CreateWork(const WorkItem::callback_t &fn, void *data);

	bool AddWork(WorkItem *work);

	uint32_t getDoneWorks() const { return mDoneWorks; }

	/* NOTE:
	 * This method should be called only after all producer threads
	 * have dispatched all pending tasks.
	 */
	void waitForAllTaskDone() const
	{
		while(!mWorkQueue.empty() || mRunningWorks)
			std::this_thread::yield();
	}

protected:
	ThreadPool();
	~ThreadPool();

	ThreadPool(const ThreadPool &rhs) = delete;
	ThreadPool& operator=(const ThreadPool &rhs) = delete;

private:
	std::stack<WorkItem *>	mWorkPool;
	std::deque<WorkItem *>	mWorkQueue;

	uint32_t				mDoneWorks;
	uint32_t				mRunningWorks;

	// TODO: replace mutex with spinlock
	std::mutex				mQueueLock;
	std::condition_variable	mWorkQueuedCond;

	int				mThreadsNum;
	std::thread    *mThreads;

	bool bInitialzed;
	bool bIsFinalizing;
};

} // namespace glsp