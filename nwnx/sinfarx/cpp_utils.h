#pragma once

#include <string>
#include <cstdio>

namespace nwnx
{
namespace cpp_utils
{
	
std::string filename_from_ptr(FILE* ptr);
void my_fread(void* ptr, size_t size, size_t count, FILE* stream);
	
}
}