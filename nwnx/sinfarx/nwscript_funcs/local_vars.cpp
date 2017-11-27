#include "nwscript_funcs.h"

namespace {

void (*CopyScriptVars)(CNWSObject*, CNWSScriptVarTable*) = (void (*)(CNWSObject*, CNWSScriptVarTable*))0x081c9638;
VM_FUNC_NEW(CopyVariables, 205)
{
	CNWSObject* from_object = GetObjectById(vm_pop_object());
	CNWSObject* to_object = GetObjectById(vm_pop_object());
	if (from_object && to_object)
	{
		CopyScriptVars(to_object, &(from_object->obj_vartable));
	}
}

CScriptArray VarToScriptArray(CScriptVariable* var)
{
	CScriptArray result(new CScriptArrayData);
	result->push_back(var->var_name);
	result->push_back(var->var_type);
	switch (var->var_type)
	{
		case VARTYPE_INT:
			result->push_back((int)(var->var_value));
			break;
		case VARTYPE_FLOAT:
			result->push_back(*(float*)&(var->var_value));
			break;
		case VARTYPE_STRING:
			result->push_back(*(CExoString*)(var->var_value));
			break;
		case VARTYPE_OBJECT:
			result->push_back((uint32_t)(var->var_value));
			break;
		case VARTYPE_LOCATION:
			result->push_back(location_as_array(*(CScriptLocation*)(var->var_value)));
			break;
	}
	return result;
}
VM_FUNC_NEW(GetLocalVariablesOfTypeWithPrefix, 344)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	uint32_t var_type = vm_pop_int();
	std::string var_prefix = vm_pop_string();
	const char* c_var_prefix = var_prefix.c_str();
	uint32_t var_prefix_len = var_prefix.length();
	CScriptArray result(new CScriptArrayData);
	if (o)
	{
		CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table)
		{
			for (uint32_t var_index=0; var_index<var_table->vt_len; var_index++)
			{
				CScriptVariable* var = &(var_table->vt_list[var_index]);
				if (var->var_type == var_type)
				{
					if (var->var_name.text && strncmp(var->var_name.text, c_var_prefix, var_prefix_len) == 0)
					{
						result->push_back(VarToScriptArray(var));
					}
				}
			}
		}
	}
	vm_push_array(&result);
}
VM_FUNC_NEW(DeleteLocalVariablesOfTypeWithPrefix, 345)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	uint32_t var_type = vm_pop_int();
	std::string var_prefix = vm_pop_string();
	const char* c_var_prefix = var_prefix.c_str();
	uint32_t var_prefix_len = var_prefix.length();
	int deleted_count = 0;
	if (o)
	{
		CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table)
		{
			for (uint32_t var_index=0; var_index<var_table->vt_len; var_index++)
			{
				CScriptVariable* var = &(var_table->vt_list[var_index]);
				if (var->var_type == var_type)
				{
					if (var->var_name.text && strncmp(var->var_name.text, c_var_prefix, var_prefix_len) == 0)
					{
						delete_local_var(var_table, var_index);
						var_index--;
						deleted_count++;
					}
				}
			}
		}
	}
	vm_push_int(deleted_count);
}
VM_FUNC_NEW(GetLocalVariableCount, 346)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	uint32_t result = 0;
	if (o)
	{
		CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table)
		{
			result = var_table->vt_len;
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(GetLocalVariableName, 495)
{
    CGameObject* o = GetGameObject(vm_pop_object());
	uint32_t var_index = vm_pop_int();
    std::string result;
	if (o)
	{
        CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table && var_index < var_table->vt_len)
		{
			result = var_table->vt_list[var_index].var_name;
		}
    }
    vm_push_string(result);
}
VM_FUNC_NEW(GetLocalVariableType, 496)
{
    CGameObject* o = GetGameObject(vm_pop_object());
	uint32_t var_index = vm_pop_int();
    int result = -1;
	if (o)
	{
        CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table && var_index < var_table->vt_len)
		{
			result = var_table->vt_list[var_index].var_type;
		}
    }
    vm_push_int(result);
}
VM_FUNC_NEW(GetLocalVariableAtIndex, 347)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	uint32_t var_index = vm_pop_int();
	if (o)
	{
		CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table)
		{
			if (var_index < var_table->vt_len)
			{
				CScriptArray result = VarToScriptArray(&(var_table->vt_list[var_index]));
				vm_push_array(&result);
				return;
			}
		}
	}
	CScriptArray result(new CScriptArrayData);
	vm_push_array(&result);
}
void vm_func_has_local_variable_of_type(uint32_t var_type)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	CExoString var_name = vm_pop_string();
	bool result = false;
	if (o)
	{
		CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
		if (var_table)
		{
			uint32_t var_count = var_table->vt_len;
			for (uint32_t var_index=0; var_index<var_count; var_index++)
			{
				CScriptVariable* var = &(var_table->vt_list[var_index]);
				if (var->var_type == var_type)//int
				{
					if (var->var_name == var_name)
					{
						result = true;
						break;
					}
				}
			}
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(HasLocalInt, 348)
{
	vm_func_has_local_variable_of_type(VARTYPE_INT);
}
VM_FUNC_NEW(HasLocalString, 349)
{
	vm_func_has_local_variable_of_type(VARTYPE_STRING);
}
VM_FUNC_NEW(HasLocalFloat, 350)
{
	vm_func_has_local_variable_of_type(VARTYPE_FLOAT);
}
VM_FUNC_NEW(HasLocalObject, 351)
{
	vm_func_has_local_variable_of_type(VARTYPE_OBJECT);
}
VM_FUNC_NEW(HasLocalLocation, 352)
{
	vm_func_has_local_variable_of_type(VARTYPE_LOCATION);
}

}
