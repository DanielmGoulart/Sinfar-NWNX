#include "utils.h"
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

namespace nwncx
{
namespace utils
{
	void enable_write(long location)
	{
		char *page;
		page = (char *) location;
		page = (char *) (((int) page + PAGESIZE - 1) & ~(PAGESIZE - 1));
		page -= PAGESIZE;
		if (mprotect (page, PAGESIZE, PROT_WRITE | PROT_READ | PROT_EXEC))
			perror ("mprotect");
	}
	
	void hook_call(long from_addr, long to_func)
	{
		enable_write(from_addr);
		*(long*)(from_addr+1) = (to_func-(from_addr+5));
	}
	
	void* hook_function(long from, long to, int len)
	{
		//jump back to original func
		char* ret_code = (char*)malloc(len+6);
		memcpy ((void*)ret_code, (void*)from, len);
		*(char*)(ret_code+len) = 0x68; //jmp
		*(long*)(ret_code+len+1) = from + len;
		*(char*)(ret_code+len+5) = 0xc3; //ret
		//jump to hook func
		enable_write(from);
		*(char*)from = 0x68; //jmp
		*(long*)(from+1) = to;
		*(char*)(from+5) = 0xc3; //ret
		//return the seemless original function
		return ret_code;
	}
}
}
