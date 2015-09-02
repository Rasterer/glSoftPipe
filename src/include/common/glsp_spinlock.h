#pragma once

#include <atomic>
#include <cassert>
#include <thread>

#include "glsp_defs.h"


namespace glsp {


/* DESIGN NOTES:
 * 1. SpinLock is a Lockable defined in STD,
 * so that it can collaborate with std::condition_variable_any.
 *
 * 2. It does NOT support recursively locking.
 */
class SpinLock
{
public:
	SpinLock():
		mLock(ATOMIC_FLAG_INIT)
	{
	}
	~SpinLock() = default;

	void lock()
	{
		while(mLock.test_and_set())
		{
			// Detect recursively locking
			assert(mOwner != std::this_thread::get_id());
		}

		mOwner = std::this_thread::get_id();
	}

	bool try_lock()
	{
		if(!mLock.test_and_set())
		{
			mOwner = std::this_thread::get_id();
			return true;
		}
		else
		{
			// Detect recursively locking
			assert(mOwner != std::this_thread::get_id());
			return false;
		}
	}

	void unlock()
	{
		// Detect unlocking a lock ownerd by others
		assert(mOwner == std::this_thread::get_id());

		// Reset the owner to a nonjoinable thread's id.
		mOwner = std::thread().get_id();
		mLock.clear();
	}

private:
	std::atomic_flag    mLock;
	std::thread::id     mOwner;
};

} // namespace glsp