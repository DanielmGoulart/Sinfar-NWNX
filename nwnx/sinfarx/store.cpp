#include "core.h"
#include "creature.h"
#include "nwscript.h"
#include "script_event.h"

using namespace nwnx::core;
using namespace nwnx::creature;
using namespace nwnx::nwscript;
using namespace nwnx::script_event;

namespace nwnx { namespace store {

unsigned char d_ret_code_storeacqitem[0x20];
int (*StoreAcquireItem_Org)(CNWSStore*, CNWSItem*, int, uint8_t, uint8_t) = (int (*)(CNWSStore*, CNWSItem*, int, uint8_t, uint8_t))&d_ret_code_storeacqitem;
int StoreAcquireItem_Hook(CNWSStore* store, CNWSItem* item, int p3, uint8_t p4, uint8_t p5)
{
	int result = StoreAcquireItem_Org(store, item, p3, p4, p5);
	if (result)
	{
		 script_event::run("mod_storeacqitem", item->obj.obj_id);
	}
	return result;
}
	
void init()
{
	hook_function(0x08085404, (unsigned long)StoreAcquireItem_Hook, d_ret_code_storeacqitem, 10);
}
REGISTER_INIT(init);
	
}
}