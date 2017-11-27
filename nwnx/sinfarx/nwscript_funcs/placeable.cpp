#include "nwscript_funcs.h"

namespace {

VM_FUNC_NEW(GetPlaceableAppearance, 207)
{
	CNWSPlaceable* placeable = GetPlaceableById(vm_pop_object());
	vm_push_int(placeable ? placeable->plc_appearance : -1);
}

VM_FUNC_NEW(SetPlaceableAppearance, 502)
{
	CNWSPlaceable* placeable = GetPlaceableById(vm_pop_object());
    uint32_t value = vm_pop_int();
    if (placeable)
    {
        placeable->plc_appearance = value;
    }
}

VM_FUNC_NEW(SetPlaceableHasInventory, 523)
{
	CNWSPlaceable* placeable = GetPlaceableById(vm_pop_object());
	int has_inventory = vm_pop_int();
	if (placeable)
	{
		placeable->plc_has_inventory = has_inventory;
	}
}
VM_FUNC_NEW(SetPlaceableStaticFlag, 212)
{
	CNWSPlaceable* placeable = GetPlaceableById(vm_pop_object());
	int static_flag = vm_pop_int();
	if (placeable)
	{
		placeable->plc_static = (static_flag != 0);
	}
}
VM_FUNC_NEW(GetPlaceableStaticFlag, 213)
{
	CNWSPlaceable* placeable = GetPlaceableById(vm_pop_object());
	vm_push_int(placeable ? placeable->plc_static : -1);
}

void (*ClosePlaceable)(CNWSPlaceable*) = (void (*)(CNWSPlaceable*))0x081e1b48;
VM_FUNC_NEW(ClosePlaceable, 136)
{
	CNWSPlaceable* placeable = GetPlaceableById(vm_pop_object());
	if (placeable)
	{
		ClosePlaceable(placeable);
	}
}

}
