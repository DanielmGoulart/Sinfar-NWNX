#include "nwscript_funcs.h"

namespace {

uint32_t current_area_index = -1;
void vm_func_push_next_area()
{
    CNWSModule* module = get_module();
    uint32_t result = OBJECT_INVALID;
    if (module && current_area_index<module->mod_areas.len)
    {
        result = module->mod_areas.data[current_area_index];
        current_area_index++;
    }
    vm_push_object(result);
}
VM_FUNC_NEW(GetFirstArea, 497)
{
    current_area_index = 0;
    vm_func_push_next_area();
}
VM_FUNC_NEW(GetNextArea, 498)
{
    vm_func_push_next_area();
}

struct MODULE_ENTRY_INFO
{
	char area_resref[16];
	Vector position;
	float orientation_x;
	float orientation_y;
};
MODULE_ENTRY_INFO* (*GetModuleEntryInfo)(CNWSModule*) = (MODULE_ENTRY_INFO* (*)(CNWSModule *))0x081c10d4;
VM_FUNC_NEW(SetStartingLocation, 227)
{
	CScriptLocation location = vm_pop_location();
	CNWSModule* module = get_module();
	CNWSArea* area = GetAreaById(location.loc_area);
	if (module && area)
	{
		MODULE_ENTRY_INFO* entry_info = GetModuleEntryInfo(module);
		strncpy(entry_info->area_resref, area->res_helper.resref.value, 16);
		entry_info->position = location.loc_position;
		entry_info->orientation_x = location.loc_orientation.x;
		entry_info->orientation_y = location.loc_orientation.y;
	}
}

}
