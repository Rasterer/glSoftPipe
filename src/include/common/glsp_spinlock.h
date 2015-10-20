#pragma once

#include <atomic>
#include <cassert>
#include <thread>

#include "glsp_defs.h"


#define cpu_relax() \
	asm volatile("pause\n": : :"memory")


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
#ifndef NDEBUG
			// Detect recursively locking
			assert(mOwner != std::this_thread::get_id());
#endif
			cpu_relax();
		}
#ifndef NDEBUG
		mOwner = std::this_thread::get_id();
#endif
	}

	bool try_lock()
	{
		if(!mLock.test_and_set())
		{
#ifndef NDEBUG
			mOwner = std::this_thread::get_id();
#endif
			return true;
		}
		else
		{
#ifndef NDEBUG
			// Detect recursively locking
			assert(mOwner != std::this_thread::get_id());
#endif
			return false;
		}
	}

	void unlock()
	{
#ifndef NDEBUG
		// Detect unlocking a lock ownerd by others
		assert(mOwner == std::this_thread::get_id());

		// Reset the owner to a nonjoinable thread's id.
		mOwner = std::thread().get_id();
#endif
		mLock.clear();
	}

private:
	std::atomic_flag    mLock;
	std::thread::id     mOwner;
};

} // namespace glsp