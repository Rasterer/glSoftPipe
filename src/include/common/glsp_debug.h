#pragma once

#include <cstdio>
#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>


#define GLSP_DPF_LEVEL_FATAL      1
#define GLSP_DPF_LEVEL_ERROR      2
#define GLSP_DPF_LEVEL_WARNING    3
#define GLSP_DPF_LEVEL_MESSAGE    4
#define GLSP_DPF_LEVEL_DEBUG      5

/* NOTE:
 * <level>
 * Please do NOT pass other parameters except for above marcos.
 * So that we can make sure the compiler would help opitimize out
 * the branch here.
 */

#ifndef NDEBUG
#define GLSP_DPF_LEVEL_DEFAULT GLSP_DPF_LEVEL_DEBUG
#else
#define GLSP_DPF_LEVEL_DEFAULT GLSP_DPF_LEVEL_ERROR
#endif

#define GLSP_DPF(level, fmt, ...)				\
	do 											\
	{											\
		if (level <= GLSP_DPF_LEVEL_DEFAULT)	\
			std::printf("T[%#lx]: " fmt, ::syscall( __NR_gettid ), ##__VA_ARGS__);	\
	} while (0)


#ifndef NDEBUG
#define GLSP_ASSERT(cond, fmt, ...)				\
	do											\
	{											\
		if(!(cond))								\
		{										\
			std::printf(fmt, ##__VA_ARGS__);	\
			assert(false);						\
		}										\
	} while (0);
#else
#define GLSP_ASSERT(cond, fmt, ...)
#endif


