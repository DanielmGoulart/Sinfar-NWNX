#include <iostream>
#include <cstring>
#include <dlfcn.h>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <map>
#include <memory>
#include <sys/mman.h>
#include <execinfo.h>

using namespace std;

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

unsigned char d_jmp_code[] = "\x68\x60\x70\x80\x90"       /* push dword 0x90807060 */
                             "\xc3\x90\x90\x90\x90";//x00 /* ret , nop , nop       */

void d_enable_write (unsigned long location)
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
	d_enable_write(from_addr);
	*(long*)(from_addr+1) = (to_func-(from_addr+5));
}

int intlen = -1;
void d_redirect (long from, long to, unsigned char *d_ret_code, long len=0)
{
	// enable write to code pages
	d_enable_write (from);
	// copy orig code stub to our "ret_code"
	len = len ? len : sizeof(d_jmp_code)-1; // - trailing 0x00
	intlen = len;
	memcpy ((void *) d_ret_code, (const void *) from, len);
	// make ret code
	*(long *)(d_jmp_code + 1) = from + len;
	memcpy ((char *) d_ret_code + len, (const void *) d_jmp_code, 6);
	// make hook code
	*(long *)(d_jmp_code + 1) = to;
	memcpy ((void *) from, (const void *) d_jmp_code, 6);
}

void hook_function(long from, long to, unsigned char *d_ret_code, long len=0)
{
	if(from)
		d_redirect(from, to, d_ret_code, len);
}

//***************** Memory Leaks Profiling

void (*free_org)(void* ptr) = NULL;
void* (*malloc_org)(size_t) = NULL;
void* (*realloc_org)(void*, size_t) = NULL;
inline void init_check()
{
	if (free_org == NULL)
	{
		malloc_org = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
		//fprintf(stderr, "malloc_org found at:%x\n", (long)malloc_org);
		free_org = (void (*)(void*))dlsym(RTLD_NEXT, "free");
		//fprintf(stderr, "free_org found at:%x\n", (long)free_org);
		realloc_org = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
	}
}

extern "C" 
{

#define BACKTRACE_SIZE 10
struct BACKTRACE
{
	BACKTRACE()
	{
		memset(&stack, 0, sizeof(stack));
	}
	bool operator< (const BACKTRACE& bk2) const
  	{
		for (int i=0; i<BACKTRACE_SIZE; i++)
		{
			if (stack[i] != bk2.stack[i]) return (stack[i] < bk2.stack[i]);
		}
		return false;
	}
	void print(FILE* f) const
	{
        char** str = backtrace_symbols(stack, BACKTRACE_SIZE);
        for (int i = 2; i < BACKTRACE_SIZE; i++)
                fprintf(f, "%s\n", str[i]);
        free(str);
	}
	void* stack[BACKTRACE_SIZE];
};
struct CALLER_ALLOC_INFO
{
	CALLER_ALLOC_INFO(): alloc_count(0), total_size(0) {}
	size_t alloc_count;
	size_t total_size;

};
struct MEM_ALLOC_INFO
{
	MEM_ALLOC_INFO(): size(0) {}
	size_t size;
	BACKTRACE caller;
};
std::map<BACKTRACE, CALLER_ALLOC_INFO> alloc_info_by_caller;
std::unordered_map<void*, MEM_ALLOC_INFO> alloc_info_by_allocptr;
std::vector<long> watch_free_pointers;
std::unordered_set<long> ignore_stack_pointers;
inline bool get_stack_ignored(void* const* stack)
{
	for (int i=0; i<BACKTRACE_SIZE; i++)
	{
		if (ignore_stack_pointers.count((long)stack[i])>0)
		{
			return true;
		}
	}
	return false;
}
inline bool get_stack_has_addr(void* const* stack, long addr)
{
	for (int i=0; i<BACKTRACE_SIZE; i++)
	{
		if ((long)stack[i] == addr)
		{
			return true;
		}
	}
	return false;
}

bool hook_malloc = false;
bool check_malloc = false;

inline void malloc_custom_process(size_t size, void* result)
{
	if (check_malloc && size > 0)
	{
		MEM_ALLOC_INFO& mem_alloc_info = alloc_info_by_allocptr[result];
		mem_alloc_info.size = size;
		backtrace(mem_alloc_info.caller.stack, BACKTRACE_SIZE);
		CALLER_ALLOC_INFO& caller_alloc_info = alloc_info_by_caller[mem_alloc_info.caller];
		caller_alloc_info.total_size += size;
		caller_alloc_info.alloc_count++;
	}
}

void* malloc(size_t size)
{
	init_check();
	
	if (!hook_malloc) return malloc_org(size);

	hook_malloc = false;
	void* result = malloc_org(size);
	malloc_custom_process(size, result);
	hook_malloc = true;
	return result;
}

inline bool print_current_backtrace(void* ptr)
{
	if (check_malloc)
	{
		void* backtrace_data[BACKTRACE_SIZE];
		backtrace(backtrace_data, BACKTRACE_SIZE);
		if (!get_stack_ignored(backtrace_data))
		{
			char** str = backtrace_symbols(backtrace_data, BACKTRACE_SIZE);
			for (int i = 0; i < BACKTRACE_SIZE; i++)
					printf("%s\n", str[i]);
			free(str);
			return true;
		}
	}
}

bool check_invalid_free = false;
inline void free_custom_process(void* ptr)
{
	if (!ptr) return;
	
	for (auto iter=watch_free_pointers.begin(); iter!=watch_free_pointers.end(); iter++)
	{
		if (*(long*)*iter == (long)ptr)
		{
			if (print_current_backtrace(ptr))
			{
				printf("freeing *%lx:%lx\n", *iter, (long)ptr);
			}
		}
	}

	auto allocptr_iter = alloc_info_by_allocptr.find(ptr);
	if (allocptr_iter != alloc_info_by_allocptr.end())
	{
		MEM_ALLOC_INFO& mem_alloc_info = allocptr_iter->second;
		CALLER_ALLOC_INFO& caller_alloc_info = alloc_info_by_caller[mem_alloc_info.caller];
		caller_alloc_info.total_size -= mem_alloc_info.size;
		caller_alloc_info.alloc_count--;
		if (caller_alloc_info.alloc_count == 0)
		{
			if (caller_alloc_info.total_size != 0)
			{
				printf("caller_alloc_info.total_size != 0\n");
			}
			alloc_info_by_caller.erase(mem_alloc_info.caller);
		}
		alloc_info_by_allocptr.erase(ptr);		
	}
	else if (check_invalid_free) //will trow false positive when freeing memory allocated when check_malloc was off
	{
		if (print_current_backtrace(ptr))
		{
			printf("freeing an unknown ptr:\n");
		}
	}
}

void free(void* ptr)
{
	init_check();

	if (!hook_malloc) return free_org(ptr);
	
	hook_malloc = false;
	free_custom_process(ptr);
	free_org(ptr);
	hook_malloc = true;
}

void* realloc(void* ptr, size_t size)
{
	init_check();
	
	if (!hook_malloc) return realloc_org(ptr, size);
	
	hook_malloc = false;
	free_custom_process(ptr);
	void* result = realloc_org(ptr, size);
	malloc_custom_process(size, result);
	hook_malloc = true;
	return result;
}

void* calloc_org(size_t num, size_t size)
{
	size_t total_size = num*size;
	void* result = malloc_org(num*size);
	memset(result, 0, total_size);
	return result;
}

bool init_malloc = true;
char init_malloc_buffer[20] = {0};
void* calloc(size_t num, size_t size)
{
	if (init_malloc)
	{
		init_malloc = false;
		return init_malloc_buffer;
	}
	
	init_check();
	
	if (!hook_malloc) return calloc_org(num, size);
	
	hook_malloc = false;
	void* result = calloc_org(num, size);
	malloc_custom_process(size*num, result);
	hook_malloc = true;
	return result;
}

}

//***************** End Memory Leek Profiling

int OnProcessKeyboardInput_SScanf(const char* src, const char* format, char* result)
{
	bool prev_hook_malloc = hook_malloc;
	hook_malloc = false;
	int ret = 0;
	if (strncmp(src, "watch", 5)==0)
	{
		long ptr;
		if (sscanf(src, "watch 0x%lx", &ptr)==1)
		{
			watch_free_pointers.push_back(ptr);
			printf("watching *0x%lx\n", ptr);
		}
	}
	else if (strncmp(src, "ignore", 6)==0)
	{
		long ptr;
		if (sscanf(src, "ignore 0x%lx", &ptr)==1)
		{
			ignore_stack_pointers.insert(ptr);
			printf("ignoring 0x%lx\n", ptr);
		}
	}
	else if (strcmp(src, "clear_watch")==0)
	{
		watch_free_pointers.clear();
		printf("watch vector cleared!\n");
	}
	else if (strcmp(src, "clear_ignore")==0)
	{
		ignore_stack_pointers.clear();
		printf("ignore set cleared!\n");
	}
	else if (strncmp(src, "print_ptr", 9)==0)
	{
		long ptr;
		if (sscanf(src, "print_ptr 0x%lx", &ptr)==1)
		{
			printf("alloc from 0x%lx:\n", ptr);
			for (auto iter=alloc_info_by_allocptr.begin(); iter!=alloc_info_by_allocptr.end(); iter++)
			{
				for (int i=0; i<BACKTRACE_SIZE; i++)
				{
					if ((long)iter->second.caller.stack[i] == ptr)
					{
						printf("   0x%lx\n", (long)iter->first);
						break;
					}
				}
			}
		}
	}
	else if (strncmp(src, "print", 5)==0)
	{
		size_t min_alloc_count = 0;
		long print_addr = 0;
		char file_name[100];
		int param_count = sscanf(src, "print %d 0x%lx %s", &min_alloc_count, &print_addr, file_name);
		FILE* f = param_count>=3 ? fopen(file_name, "w") : stdout;
		for (auto iter=alloc_info_by_caller.begin(); iter!=alloc_info_by_caller.end(); iter++)
		{
			if (iter->second.alloc_count > min_alloc_count && !get_stack_ignored(iter->first.stack) && (print_addr==0 || get_stack_has_addr(iter->first.stack, print_addr)))
			{
				fprintf(f, "%u/%u bytes allocated at:\n", iter->second.total_size, iter->second.alloc_count);
				iter->first.print(f);
				fprintf(f, "\n\n");
			}
		}
		if (param_count>=3) fclose(f);
	}
	else if (strncmp(src, "clear", 11)==0)
	{
		alloc_info_by_caller.clear();
		alloc_info_by_allocptr.clear();
		printf("alloc cleared!\n");
	}
	else if (strncmp(src, "count", 15)==0)
	{
		printf("alloc by caller:%u total:%u\n", alloc_info_by_caller.size(), alloc_info_by_allocptr.size());
	}
	else if (strcmp(src, "hook")==0)
	{
		check_malloc = true;
		printf("free/malloc hook enabled\n");
	}
	else if (strcmp(src, "toggle_check_invalid_free")==0)
	{
		check_invalid_free = !check_invalid_free;
		if (check_invalid_free)
		{
			check_malloc = true;
		}
		printf("toggle_check_invalid_free: %d\n", check_invalid_free);
	}
	else if (strcmp(src, "unhook")==0)
	{
		check_malloc = false;
		printf("free/malloc hook disabled\n");
	}
	else
	{
		ret = sscanf(src, format, result);
	}
	hook_malloc = prev_hook_malloc;
	return ret;
}

class memtest
{
public:
	memtest()
	{
		//custom keyboard input processing
		hook_call(0x0804d36d, (long)OnProcessKeyboardInput_SScanf);
		
		hook_malloc = true;
	}
	~memtest()
	{
		hook_malloc = false;
	}
};
memtest memtest;