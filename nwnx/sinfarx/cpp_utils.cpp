#include "cpp_utils.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

namespace nwnx
{
namespace cpp_utils
{
	
std::string filename_from_ptr(FILE* fp)
{
	char filename[0xFFF];
	char proclnk[0xFFF];
	int fno = fileno(fp);
	sprintf(proclnk, "/proc/self/fd/%d", fno);
	size_t r = readlink(proclnk, filename, 0xFFF);
	if (r < 0) r = 0;
	filename[r] = '\0';
	return filename;
}
	
void my_fread(void* ptr, size_t size, size_t count, FILE* fp)
{
	size_t read_count = fread(ptr, size, count, fp);
	if (read_count != count)
	{
		if (read_count != 0 || count*size != 0)
		{
			fprintf(stderr, "%s:trying to read %u blocks of %u bytes but read only %u blocks", filename_from_ptr(fp).c_str(), count, size, read_count);
		}
	}
}
	
}
}