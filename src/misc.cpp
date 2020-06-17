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
