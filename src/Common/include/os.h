#pragma once

#if defined(_WIN32)
# include <Winbase.h>
# include <Processthreadsapi.h>


static inline DWORD OSGetProcessorsNum()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );

	return sysinfo.dwNumberOfProcessors;
}

# define OSGetThreadID()	GetCurrentThreadId()

#	if defined(_WIN64)
#	else
#	endif


#elif defined(__linux__)
# include <linux/unistd.h>
# include <sys/syscall.h>
# include <unistd.h>

static inline long OSGetProcessorsNum()
{
	return ::sysconf(_SC_NPROCESSORS_ONLN);
}

#define OSGetThreadID()		(::syscall(__NR_gettid))

#endif
