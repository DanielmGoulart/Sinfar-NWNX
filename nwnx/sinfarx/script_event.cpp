#include "script_event.h"
#include "nwscript.h"
#include <stack>

using namespace nwnx::core;
using namespace nwnx::nwscript;

namespace nwnx { namespace script_event {
struct EVENT
{
	EVENT(const nwscript::CScriptArray& params) : params(params){}
	nwscript::CScriptArray params;
	RESULT result;
};
std::stack<EVENT> running_events;
RESULT run(const std::string& script, uint32_t object_id, const nwscript::CScriptArray& params)
{
	running_events.push(EVENT(params));
	nwnx::core::run_script(script, object_id);
	RESULT result = std::move(running_events.top().result);
	running_events.pop();
	return result;
}
RESULT run(const std::string& script, uint32_t object_id, const std::initializer_list<nwscript::CScriptVarValue>& params)
{
	nwscript::CScriptArray params_array(new nwscript::CScriptArrayData);
	for (const nwscript::CScriptVarValue& param: params)
	{
		params_array->push_back(param);
	}
	return run(script, object_id, params_array);
}
void set_result(const nwscript::CScriptVarValue& result)
{
	running_events.top().result.reset(new nwscript::CScriptVarValue(result));
}
nwscript::CScriptArray& get_params()
{
	return running_events.top().params;
}
const nwscript::CScriptVarValue& get_param(uint32_t index)
{
	if (index < running_events.top().params->size())
	{
		return running_events.top().params->at(index);
	}
	return nwscript::empty_script_value;
}
uint32_t get_param_count()
{
	return running_events.top().params->size();
}

void init()
{
	running_events.push(EVENT(nwscript::empty_cscriptarray));
	set_result(-1);
}
REGISTER_INIT(init);
	
VM_FUNC_NEW(GetEventParamInt, 381)
{
	vm_push_int(script_event::get_param(vm_pop_int()).as_int());
}
VM_FUNC_NEW(GetEventParamObject, 382)
{
	vm_push_object(script_event::get_param(vm_pop_int()).as_object());
}
VM_FUNC_NEW(GetEventParamString, 383)
{
	vm_push_string(script_event::get_param(vm_pop_int()).as_string());
}
VM_FUNC_NEW(GetEventParamFloat, 384)
{
	vm_push_float(script_event::get_param(vm_pop_int()).as_float());
}
VM_FUNC_NEW(GetEventParamEffect, 385)
{
	vm_push_effect(script_event::get_param(vm_pop_int()).as_effect());
}
VM_FUNC_NEW(GetEventParamArray, 386)
{
	vm_push_array(script_event::get_param(vm_pop_int()).as_array());
}
VM_FUNC_NEW(GetEventParamDictionary, 387)
{
	vm_push_dictionary(script_event::get_param(vm_pop_int()).as_dictionary());
}
VM_FUNC_NEW(GetEventParamLocation, 388)
{
	vm_push_location(script_event::get_param(vm_pop_int()).as_location());
}
VM_FUNC_NEW(GetEventParamVector, 389)
{
	vm_push_vector(script_event::get_param(vm_pop_int()).as_vector());
}
VM_FUNC_NEW(GetEventParams, 390)
{
	vm_push_array(&script_event::get_params());
}
VM_FUNC_NEW(GetEventParamType, 391)
{
	vm_push_int(script_event::get_param(vm_pop_int()).value_type);
}
VM_FUNC_NEW(GetEventParamCount, 392)
{
	vm_push_int(script_event::get_param_count());
}
VM_FUNC_NEW(BypassEvent, 393)
{
	script_event::set_result(false);
}
VM_FUNC_NEW(SetEventResultInt, 394)
{
	script_event::set_result(vm_pop_int());
}
VM_FUNC_NEW(SetEventResultObject, 395)
{
	script_event::set_result(vm_pop_object());
}
VM_FUNC_NEW(SetEventResultString, 396)
{
	script_event::set_result(vm_pop_string());
}
VM_FUNC_NEW(SetEventResultFloat, 397)
{
	script_event::set_result(vm_pop_float());
}
VM_FUNC_NEW(SetEventResultEffect, 398)
{
	script_event::set_result(vm_pop_effect().get());
}
VM_FUNC_NEW(SetEventResultArray, 399)
{
	script_event::set_result(vm_pop_array());
}
VM_FUNC_NEW(SetEventResultDictionary, 400)
{
	script_event::set_result(vm_pop_dictionary());
}
VM_FUNC_NEW(SetEventResultVector, 401)
{
	script_event::set_result(vm_pop_vector());
}
VM_FUNC_NEW(SetEventResultLocation, 402)
{
	script_event::set_result(vm_pop_location());
}
VM_FUNC_NEW(RunEvent, 403)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::run(script_name, object_id, params);
}
VM_FUNC_NEW(RunEventInt, 404)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_int(result ? result->as_int() : 0);
}
VM_FUNC_NEW(RunEventObject, 405)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_object(result ? result->as_object() : OBJECT_INVALID);
}
VM_FUNC_NEW(RunEventString, 406)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_string(result ? result->as_string() : "");
}
VM_FUNC_NEW(RunEventFloat, 407)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_float(result ? result->as_float() : 0);
}
VM_FUNC_NEW(RunEventEffect, 408)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_effect(result ? result->as_effect() : empty_effect);
}
VM_FUNC_NEW(RunEventArray, 409)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_array(result ? result->as_array() : &empty_cscriptarray);
}
VM_FUNC_NEW(RunEventDictionary, 410)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_dictionary(result ? result->as_dictionary() : &empty_cscriptdictionary);
}
VM_FUNC_NEW(RunEventVector, 411)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_vector(result ? result->as_vector() : empty_vector);
}
VM_FUNC_NEW(RunEventLocation, 412)
{
	std::string script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	CScriptArray params = vm_pop_array();
	script_event::RESULT result = script_event::run(script_name, object_id, params);
	vm_push_location(result ? result->as_location() : empty_location);
}

}
}