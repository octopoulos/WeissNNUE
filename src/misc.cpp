#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#if defined(__linux__) && !defined(__ANDROID__)
#include <stdlib.h>
#include <sys/mman.h>
#endif

#include "misc.h"

using namespace std;


void prefetch(void* addr) {

#  if defined(__INTEL_COMPILER)
   // This hack prevents prefetches from being optimized away by
   // Intel compiler. Both MSVC and gcc seem not be affected by this.
   __asm__ ("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
  _mm_prefetch((char*)addr, _MM_HINT_T0);
#  else
  __builtin_prefetch(addr);
#  endif
}


void* aligned_malloc(size_t size, size_t align)
{
	void* p = _mm_malloc(size, align);
	if (p == nullptr)
	{
		std::cout << "info string can't allocate memory. sise = " << size << std::endl;
		exit(1);
	}
	return p;
}
