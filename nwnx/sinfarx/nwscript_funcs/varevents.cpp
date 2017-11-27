#include "nwscript_funcs.h"
#include <boost/lexical_cast.hpp>

namespace {

std::string current_event;
uint32_t current_event_object = OBJECT_INVALID;
bool bypass_current_event = false;

void set_no_bypass_value(CNWSScriptVarTable* var_table, const std::string& event, int no_bypass)
{
    if (no_bypass)
        set_local_int(var_table, event+"_NOBP", 1);
    else
        delete_local_int(var_table, event+"_NOBP");
}

VM_FUNC_NEW(RunVariableEvent, 533)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	std::string event = vm_pop_string();
	if (!o)
	{
		vm_push_int(false);
		return;	
	}
	CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
	if (!var_table)
	{
		vm_push_int(false);
		return;
	}
    std::string script = get_local_string(var_table, event);
    if (script != "")
    {
        uint32_t registered_count = get_local_int(var_table, event+"_COUNT");
		current_event_object = o->id;
		current_event = event;
		bypass_current_event = false;

        run_script(script, virtual_machine->vm_cmd->cmd_self_id);
        if (bypass_current_event)
        {
            for (uint32_t script_index=0; script_index<registered_count; script_index++)
            {
                if (get_local_int(var_table, event+boost::lexical_cast<std::string>(script_index)+"_NOBP"))
                {
                    run_script(get_local_string(var_table, event+boost::lexical_cast<std::string>(script_index)), virtual_machine->vm_cmd->cmd_self_id);
                }
            }
			vm_push_int(true);
			return;
        }
        else
        {
            for (uint32_t script_index=0; script_index<registered_count; script_index++)
            {
                run_script(get_local_string(var_table, event+boost::lexical_cast<std::string>(script_index)), virtual_machine->vm_cmd->cmd_self_id);
                if (bypass_current_event)
                {
					for (++script_index; script_index<registered_count; script_index++)
                    {
                        if (get_local_int(var_table, event+boost::lexical_cast<std::string>(script_index)+"_NOBP"))
                        {
                            run_script(get_local_string(var_table, event+boost::lexical_cast<std::string>(script_index)), virtual_machine->vm_cmd->cmd_self_id);
                        }
                    }
                    vm_push_int(true);
					return;
                }
            }
        }
    }
	vm_push_int(false);
}

VM_FUNC_NEW(GetCurrentVariableEvent, 534)
{
    vm_push_string(current_event);
}

VM_FUNC_NEW(GetCurrentVariableEventTarget, 535)
{
    vm_push_object(current_event_object);
}

VM_FUNC_NEW(BypassVariableEvent, 536)
{
    bypass_current_event = true;
}

VM_FUNC_NEW(UnregisterVariableEvent, 537)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	std::string event = vm_pop_string();
	std::string script = vm_pop_string();	
	if (!o || event.empty() || script.empty()) return;
	CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
	if (!var_table) return;
	
	std::string registered_count_var = event+"_COUNT";
	uint32_t registered_count = get_local_int(var_table, registered_count_var);
    std::string current_script = get_local_string(var_table, event);
    if (current_script == script)
    {
        if (registered_count == 0) //first but also the last
        {
            delete_local_string(var_table, event);
            delete_local_int(var_table, event+"_NOBP");
        }
        else
        {
            registered_count--;

            set_local_string(var_table, event, get_local_string(var_table, event+boost::lexical_cast<std::string>(registered_count)));
            set_no_bypass_value(var_table, event, get_local_int(var_table, event+boost::lexical_cast<std::string>(registered_count)+"_NOBP"));

            if (registered_count)
            {
                set_local_int(var_table, registered_count_var, registered_count);
            }
            else
            {
                delete_local_int(var_table, registered_count_var);
            }
        }
    }
    else
    {
        for (uint32_t script_index=0; script_index<registered_count; script_index++)
        {
            std::string event_script_var = event+boost::lexical_cast<std::string>(script_index);
            current_script = get_local_string(var_table, event_script_var);
            if (current_script == script)
            {
                registered_count--;

                if (script_index == registered_count)//last script
                {
                    delete_local_string(var_table, event_script_var);
                    delete_local_int(var_table, event_script_var+"_NOBP");
                }
                else
                {
                    set_local_string(var_table, event_script_var, get_local_string(var_table, event+boost::lexical_cast<std::string>(registered_count)));
                    set_no_bypass_value(var_table, event_script_var,  get_local_int(var_table, event+boost::lexical_cast<std::string>(registered_count)+"_NOBP"));
                }

                if (registered_count)
                {
                    set_local_int(var_table, registered_count_var, registered_count);
                }
                else
                {
                    delete_local_int(var_table, registered_count_var);
                }

                break;
            }
        }
    }
}

VM_FUNC_NEW(RegisterVariableEvent, 538)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	std::string event = vm_pop_string();
	std::string script = vm_pop_string();
	int no_bypass = vm_pop_int();
	if (!o || event.empty() || script.empty()) return;
	CNWSScriptVarTable* var_table = GetGameObjectVarTable(o);
	if (!var_table) return;

    std::string current_script = get_local_string(var_table, event);
    if (current_script == "")
    {
        set_local_string(var_table, event, script);
        set_no_bypass_value(var_table, event, no_bypass);
    }
    else if (current_script == script)
    {
        set_no_bypass_value(var_table, event, no_bypass);
    }
    else
    {
        std::string registered_count_var = event+"_COUNT";
        uint32_t registered_count = get_local_int(var_table, registered_count_var);
        for (uint32_t script_index=0; script_index<registered_count; script_index++)
        {
            current_script = get_local_string(var_table, event+boost::lexical_cast<std::string>(script_index));
            if (current_script == script)
            {
                set_no_bypass_value(var_table, event+boost::lexical_cast<std::string>(script_index), no_bypass);
                return;
            }
        }
        set_local_string(var_table, event+boost::lexical_cast<std::string>(registered_count), script);
        set_no_bypass_value(var_table, event+boost::lexical_cast<std::string>(registered_count), no_bypass);
        set_local_int(var_table, registered_count_var, ++registered_count);
    }
}

}