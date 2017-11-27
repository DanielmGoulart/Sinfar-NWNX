#include "core.h"
#include "creature.h"
#include "nwscript.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;

namespace nwnx { namespace workaround {

int ComputeInterAreaPath_Hook(CNWSModule* module, CPathfindInformation* path_finding_info)
{
	return 0;
}

int SaveGame_Hook(CServerExoAppInternal* server_internal, uint32_t, CExoString*, CExoString*, CNWSPlayer*, int, CExoString*)
{
	//do not save the module
	return 0;
}

void init()
{
	//do not save the game
	hook_function(0x0809902C, (uint32_t)SaveGame_Hook, d_ret_code_nouse, 12);
	
	//do not inter area path find ... the module is too big for that
	hook_function(0x081b3d34, (unsigned long)ComputeInterAreaPath_Hook, d_ret_code_nouse, 12);	
}
REGISTER_INIT(init);
	
}
}