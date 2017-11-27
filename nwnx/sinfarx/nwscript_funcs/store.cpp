#include "nwscript_funcs.h"

namespace
{
	
VM_FUNC_NEW(SetStoreSellMarkUp, 198)
{
	CNWSStore* store = GetStoreById(vm_pop_object());
	int value = vm_pop_int();
	if (store)
	{
		store->st_sell_mark_up = value;
	}
}
VM_FUNC_NEW(GetStoreSellMarkUp, 199)
{
	CNWSStore* store = GetStoreById(vm_pop_object());
	vm_push_int(store ? store->st_sell_mark_up : -1);
}
VM_FUNC_NEW(SetStoreBuyMarkDown, 200)
{
	CNWSStore* store = GetStoreById(vm_pop_object());
	int value = vm_pop_int();
	if (store)
	{
		store->st_buy_mark_down = value;
	}
}
VM_FUNC_NEW(SetStoreBuyStolenGoods, 201)
{
	CNWSStore* store = GetStoreById(vm_pop_object());
	int value = vm_pop_int();
	if (store)
	{
		store->st_buy_stolen = value;
	}
}

}