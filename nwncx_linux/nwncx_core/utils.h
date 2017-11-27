#pragma once

#include <stdint.h>

namespace nwncx
{
namespace utils
{
	void enable_write(long location);
	void hook_call(long from_addr, long to_func);
	void* hook_function(long from, long to, int len);
}
}