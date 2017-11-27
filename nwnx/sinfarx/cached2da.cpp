#include "cached2da.h"
#include "resman.h"
#include "nwscript.h"

#include <algorithm>
#include <unordered_map>
#include <memory>

using namespace nwnx::core;
using namespace nwnx::resman;
using namespace nwnx::nwscript;

namespace nwnx { namespace cached2da {

std::string tables_dir = "./override";
unordered_map<string, std::unique_ptr<C2da>> cached_tables;
C2da* get_cached_2da(std::string table_name)
{
	std::transform(table_name.begin(), table_name.end(), table_name.begin(), ::tolower);
	auto iter = cached_tables.find(table_name);
	if (iter == cached_tables.end())
	{
		C2da* table = new C2da(get_resource_path(aurora::NwnResType_2DA, table_name.c_str()));
		cached_tables[table_name] = std::unique_ptr<C2da>(table);
		return table;
	}
	else
	{
		return iter->second.get();
	}
}

void remove_2da_from_cache(std::string table_name)
{
	std::transform(table_name.begin(), table_name.end(), table_name.begin(), ::tolower);
	cached_tables.erase(table_name);
}

void clear_cached_2da()
{
	cached_tables.clear();
}

VM_FUNC_REPLACE(Get2DAString, 710)
{
	CExoString table_name = vm_pop_string();
	CExoString col_name = vm_pop_string();
	int row_index = vm_pop_int();
	CExoString result;
	C2da* table = get_cached_2da(table_name);
	if (table)
	{
		result = table->GetString(col_name, row_index).c_str();
	}
	vm_push_string(&result);
}

VM_FUNC_NEW(Get2DARowCount, 51)
{
	CExoString table_name = vm_pop_string();
	int result = 0;
	C2da* table = get_cached_2da(table_name);
	if (table)
	{
		result = table->GetRowCount();
	}
	vm_push_int(result);
}
VM_FUNC_NEW(Get2DAColCount, 52)
{
	CExoString table_name = vm_pop_string();
	int result = 0;
	C2da* table = get_cached_2da(table_name);
	if (table)
	{
		result = table->GetColCount();
	}
	vm_push_int(result);
}
VM_FUNC_NEW(Get2DAColumnNameByIndex, 53)
{
	CExoString table_name = vm_pop_string();
	int column_index = vm_pop_int();
	CExoString result;
	C2da* table = get_cached_2da(table_name);
	if (table)
	{
		result = table->GetColumnNameByIndex(column_index).c_str();
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(Get2DAStringByColumnIndex, 54)
{
	CExoString table_name = vm_pop_string();
	int column_index = vm_pop_int();
	int row_index = vm_pop_int();
	CExoString result;
	C2da* table = get_cached_2da(table_name);
	if (table)
	{
		result = table->GetString(column_index, row_index).c_str();
	}
	vm_push_string(&result);
}
inline bool starts_with(const char* string, const char* prefix)
{
    while(*prefix)
    {
        if(*prefix++ != *string++)
            return false;
    }
    return true;
}
VM_FUNC_NEW(GetNext2DARowWithPrefix, 55)
{
	#define VM_FUNC_FIND_NEXT_2DA_ROW_COMPARATOR (starts_with(table->GetString(col_index, row_index).c_str(), prefix))
	#include "vm_func_get_next_2da_row.h"
	#undef VM_FUNC_FIND_NEXT_2DA_ROW_COMPARATOR
}
VM_FUNC_NEW(GetNext2DARowWithValue, 57)
{
	#define VM_FUNC_FIND_NEXT_2DA_ROW_COMPARATOR (table->GetString(col_index, row_index) == prefix)
	#include "vm_func_get_next_2da_row.h"
	#undef VM_FUNC_FIND_NEXT_2DA_ROW_COMPARATOR
}

VM_FUNC_NEW(ReloadCached2DA, 56)
{
	CExoString table_name = vm_pop_string();
	remove_2da_from_cache(table_name);
}
void (*ClearCached2DAs)(CTwoDimArrays*) = (void (*)(CTwoDimArrays*))0x080b98a4;
VM_FUNC_NEW(ReloadCached2das, 525)
{
	ClearCached2DAs(nwn_rules->ru_2das);
	clear_cached_2da();	
}
int (*Unload2DAs)(CTwoDimArrays*) = (int (*)(CTwoDimArrays*))0x080b7728;
int (*ReloadAllRules)(CNWRules*) = (int (*)(CNWRules*))0x080C6AA0;
VM_FUNC_NEW(ReloadAll2das, 526)
{
	ClearCached2DAs(nwn_rules->ru_2das);
	clear_cached_2da();	
	Unload2DAs(nwn_rules->ru_2das);
	ReloadAllRules(nwn_rules);
}

}
}
