#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <boost/serialization/singleton.hpp>


namespace glsp {

class WorkItem
{
private:
	std::function<void (void *)> mCallback;
	void *data;
};

class ThreadPool: public boost::serialization::singleton<ThreadPool>
{
public:
	static ThreadPool& get()
	{
		return get_mutable_instance();
	}
	static const ThreadPool& get() const
	{
		return get_const_instance();
	}

	bool Initialize();

private:
	ThreadPool();
	~ThreadPool();

private:
	int						mWorkPoolSize;
	WorkItem 			   *mWorkPool;
	std::deque<WorkItem *>	mWorkQueue;

	std::mutex				mQueueLock;
	std::condition_variable	mCond;

	int				mThreadsNum;
	std::thread    *mThreads;

	bool bInitialzed;
};

} // namespace glsp