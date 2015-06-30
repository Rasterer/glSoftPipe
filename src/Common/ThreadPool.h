#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
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

	void WaitForComplete();

private:
	WorkItem(callback_t &&fn, void *data);
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

	WorkItem* CreateWork(WorkItem::callback_t &&fn, void *data);

	bool AddWork(WorkItem *work);

	uint32_t getDoneWorks() const { return mDoneWorks; }

protected:
	ThreadPool();
	~ThreadPool();

	ThreadPool(const ThreadPool &rhs) = delete;
	ThreadPool& operator=(const ThreadPool &rhs) = delete;

private:
	int						mWorkPoolSize;
	WorkItem 			   *mWorkPool;
	std::deque<WorkItem *>	mWorkQueue;
	WorkItem			   *mLastQueuedWork;
	uint32_t				mDoneWorks;

	// TODO: replace mutex with spinlock
	std::mutex				mQueueLock;
	std::condition_variable	mWorkQueuedCond;

	int				mThreadsNum;
	std::thread    *mThreads;

	bool bInitialzed;
	bool bIsFinalizing;
	bool bAllWorkDone;
};

} // namespace glsp