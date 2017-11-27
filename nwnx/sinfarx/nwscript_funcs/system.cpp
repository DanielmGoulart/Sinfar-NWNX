#include "nwscript_funcs.h"
#include "../erf.h"

#include <unistd.h>
#include <signal.h>

namespace {

VM_FUNC_NEW(GetSystemTime, 24)
{
	vm_push_int(time(NULL));
}
VM_FUNC_NEW(ShutdownServer, 25)
{
	int force = vm_pop_int();
    if (force > 0)
        alarm(force);
    kill(getpid(), SIGTERM);
}
inline bool is_valid_path_for_nwscript(const char* path)
{
	if (path && *path && *path != '/')
	{
		return true;
	}
	return false;
}
VM_FUNC_NEW(FileDelete, 27)
{
	CExoString path = vm_pop_string();
	int ret = 0;
	if (erf::get_current_script_in_core() &&
		is_valid_path_for_nwscript(path.text))
	{
		ret = unlink(path.text);
		if (ret < 0)
			ret = -errno;
		else
			ret = 1;
	}
	vm_push_int(ret);
}
VM_FUNC_NEW(FileRename, 28)
{
	CExoString path_from = vm_pop_string();
	CExoString path_to = vm_pop_string();
	int ret = 0;
	if (erf::get_current_script_in_core() &&
		is_valid_path_for_nwscript(path_from.text) &&
		is_valid_path_for_nwscript(path_to.text))
	{
		ret = rename(path_from.text, path_to.text);
		if (ret < 0)
			ret = -errno;
		else
			ret = 1;
	}
	vm_push_int(ret);
}
VM_FUNC_NEW(GetProcessMemoryUsage, 29)
{
	vm_push_int(reinterpret_cast<int>(sbrk(0)) - 0x08000000);
}
	
}