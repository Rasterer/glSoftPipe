#pragma once


#if defined(i386) || defined(__i386__) || defined(_X86_) || defined(_M_IX86)
# define cpu_relax() asm volatile("pause\n": : :"memory")

#elif defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
# define cpu_relax() asm volatile("pause\n": : :"memory")


#elif defined(__arm__) || defined(_M_ARM)
#define cpu_relax()

#endif
