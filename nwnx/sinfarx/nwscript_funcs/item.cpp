#include "nwscript_funcs.h"

namespace {

VM_FUNC_NEW(SetBaseItemType, 499)
{
    CNWSItem* item = GetItemById(vm_pop_object());
    uint32_t value = vm_pop_int();
    if (item)
    {
        item->it_baseitem = value;
    }
}

VM_FUNC_NEW(SetGoldPieceValue, 500)
{
    CNWSItem* item = GetItemById(vm_pop_object());
    uint32_t value = vm_pop_int();
    if (item)
    {
        item->it_cost_ided = value;
    }
}

VM_FUNC_NEW(SetItemWeight, 501)
{
    CNWSItem* item = GetItemById(vm_pop_object());
    uint32_t value = vm_pop_int();
    if (item)
    {
        item->it_weight = value;
    }
}

#define ITEM_APPR_TYPE_SIMPLE_MODEL	0
#define ITEM_APPR_TYPE_WEAPON_COLOR	1
#define ITEM_APPR_TYPE_WEAPON_MODEL	2
#define ITEM_APPR_TYPE_ARMOR_MODEL	3
#define ITEM_APPR_TYPE_ARMOR_COLOR	4
VM_FUNC_NEW(ModifyItem, 208)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	uint32_t type = vm_pop_int();
	uint32_t index = vm_pop_int();
	uint32_t new_value = vm_pop_int();
	if (item)
	{
		switch (type)
		{
			case ITEM_APPR_TYPE_SIMPLE_MODEL:
				item->it_model[0] = new_value;
				break;
			case ITEM_APPR_TYPE_WEAPON_COLOR:
				if (index < 3)
				{
					item->it_model[index] = item->it_model[index] / 10 * 10 + new_value;
				}
				break;
			case ITEM_APPR_TYPE_WEAPON_MODEL:
				if (index < 3)
				{
					item->it_model[index] = item->it_model[index] % 10 + new_value * 10;
				}
				break;
			case ITEM_APPR_TYPE_ARMOR_MODEL:
				if (index < 19)
				{
					item->it_model[3+index] = new_value;
				}
				break;
			case ITEM_APPR_TYPE_ARMOR_COLOR:
				if (index < 6)
				{
					item->it_color[index] = new_value;
				}
				break;
		}
	}
}

int (*GetItemAC)(CNWSItem*) = (int (*)(CNWSItem*))0x081a2d58;
void (*ComputeWeight)(CNWSItem*) = (void (*)(CNWSItem*))0x081a137c;
VM_FUNC_NEW(GetItemAC, 202)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	vm_push_int(item ? GetItemAC(item) : 0);
}
void (*ComputeItemCost)(CNWSItem*) = (void (*)(CNWSItem*))0x081a5794;
inline void update_item_ac(CNWSItem* item)
{
	item->it_ac = GetItemAC(item);
	ComputeWeight(item);
	ComputeItemCost(item);
}
VM_FUNC_NEW(SetItemAC, 203)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	int value = vm_pop_int();
	if (item)
	{
		set_local_int(&item->obj.obj_vartable, "XAC", value);
		update_item_ac(item);
	}
}
VM_FUNC_NEW(ResetItemAC, 204)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	if (item)
	{
		delete_local_int(&item->obj.obj_vartable, "XAC");
		update_item_ac(item);
	}
}

VM_FUNC_NEW(ForceItemStackSize, 229)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	uint32_t value = vm_pop_int();
	if (item)
	{
		item->it_stack_size = value;
	}
}

VM_FUNC_NEW(SetAdditionalCost, 218)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	uint32_t add_cost = vm_pop_int();
	if (item)
	{
		item->it_cost_add = add_cost;
	}
}
VM_FUNC_NEW(GetAdditionalCost, 219)
{
	CNWSItem* item = GetItemById(vm_pop_object());
	vm_push_int(item ? item->it_cost_add : 0);
}

}
