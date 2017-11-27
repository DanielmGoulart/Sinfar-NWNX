#include "nwscript.h"
#include "script_event.h"
#include "server.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <stack>
#include <sys/time.h>

using namespace nwnx::core;

namespace nwnx { namespace nwscript {

	#ifdef LAG_TEST
	struct script_executions_info
	{
		uint32_t run_count;
		timeval run_time;
		script_executions_info()
		{
			run_count = 0;
			memset(&run_time, 0, sizeof(run_time));
		}
	};
	std::unordered_map<std::string, script_executions_info> scripts_executions_info;
	#endif
	unsigned char d_ret_code_shutdown[0x20];
	void (*Shutdown_Org)(CServerExoAppInternal*, int, int) = (void (*)(CServerExoAppInternal*, int, int))&d_ret_code_shutdown;
	void Shutdown_HookProc(CServerExoAppInternal* server_internal, int p2, int p3)
	{
	#ifdef LAG_TEST
		//dump script execution info
		FILE* f = fopen("./logs.0/scripts_exec.txt", "w");
		if (f)
		{
			fprintf(f, "script_name;run_count;run_time\n");
			for (auto iter=scripts_executions_info.begin(); iter!=scripts_executions_info.end(); iter++)
			{
				fprintf(f, "%s;%u;%ld.%06ld\n", iter->first.c_str(), iter->second.run_count, iter->second.run_time.tv_sec, iter->second.run_time.tv_usec);
			}
			fclose(f);
		}
		else
		{
			fprintf(stderr, "failed to open scripts_exec.txt\n");
		}
	#endif

		return Shutdown_Org(server_internal, p2, p3);
	}

	std::stack<std::string> last_script_ran;
	std::string get_last_script_ran()
	{
		return (last_script_ran.size()>0?last_script_ran.top().c_str():"");
	}
	uint32_t last_run_script_caller_id = OBJECT_INVALID;
	#ifdef LAG_TEST
		timeval script_timer_start;
		inline void lag_test_run_script_end(const std::string& script_name, bool pause_only=false, bool script_situation=false);
		inline void lag_test_run_script(const std::string& script_name, bool resume_only=false);
		inline void lag_test_run_script(const std::string& script_name, bool resume_only)
		{
			if (!resume_only && last_script_ran.size() > 0)
			{
				lag_test_run_script_end(last_script_ran.top(), true);
			}
			gettimeofday(&script_timer_start, NULL);
		}
		inline void lag_test_run_script_end(const std::string& script_name, bool pause_only, bool script_situation)
		{
			timeval script_timer_end;
			gettimeofday(&script_timer_end, NULL);
			timersub(&script_timer_end, &script_timer_start, &script_timer_end);
			script_executions_info& script_exec_info = scripts_executions_info[script_name];
			timeradd(&script_exec_info.run_time, &script_timer_end, &script_exec_info.run_time);
			if (!pause_only)
			{
				if (!script_situation) script_exec_info.run_count++;
				if (script_timer_end.tv_sec > 0 || script_timer_end.tv_usec/100000 > 0)
				{
					time_t t=time(NULL);
					char* timestamp = ctime(&t);
					timestamp[strlen(timestamp)-1] = 0;
					char laggy_script_path[255];
					sprintf(laggy_script_path, "%s/logs.0/laggy_scripts.txt", server::get_nwn_folder());
					FILE* f = fopen(laggy_script_path, "a");
					if (f)
					{
						fprintf(f, "[%s] %x|%s = %ld.%06lds\n", timestamp, last_run_script_caller_id, script_name.c_str(), script_timer_end.tv_sec, script_timer_end.tv_usec);
						fclose(f);
					}
					else
					{
						fprintf(stderr, "failed to open laggy_scripts.txt with error:%d\n", errno);
					}
				}
				if (last_script_ran.size() > 0)
				{
					lag_test_run_script(last_script_ran.top(), true);
				}
			}
		}
	#endif
	std::unordered_map<std::string, std::vector<std::string>> registered_events;
	bool has_registered_events(const std::string& script_name)
	{
		return (registered_events.find(script_name) != registered_events.end());
	}
	bool bBypassRegisteredEvent;
	unsigned char d_ret_code_runscript[0x20];
	int (*RunScript_Org)(CVirtualMachine*, CExoString*, uint32_t, int) = (int (*)(CVirtualMachine*, CExoString*, uint32_t, int))&d_ret_code_runscript;
	int RunScript_HookProc(CVirtualMachine* vm, CExoString* script, uint32_t caller_id, int p4)
	{
		if (!script || !(script->text)) return 0;

		auto iter_events = registered_events.find(script->text);
		if (iter_events != registered_events.end())
		{
			bBypassRegisteredEvent = false;

			auto& vec_event = iter_events->second;
			uint32_t script_count = vec_event.size();
			for (uint32_t script_index=script_count-1; script_index<script_count; script_index--)
			{
				CExoString registered_script(vec_event.at(script_index).c_str());
				int script_return_val = RunScript_HookProc(vm, &registered_script, caller_id, 1);

				if (bBypassRegisteredEvent) return script_return_val;
			}
		}

		last_run_script_caller_id = caller_id;
		std::string script_name(script->text); //must save the script name because it can be freed when running the script

	#ifdef LAG_TEST
		lag_test_run_script(script_name);
	#endif

	    last_script_ran.push(script_name);
	    int ret = RunScript_Org(vm, script, caller_id, p4);
		last_script_ran.pop();

	#ifdef LAG_TEST
		lag_test_run_script_end(script_name);
	#endif

		return ret;
	}

	unsigned char d_ret_code_runscriptsituation[0x20];
	int (*RunScriptSituation_Org)(CVirtualMachine*, CVirtualMachineScript*, uint32_t, int) = (int (*)(CVirtualMachine*, CVirtualMachineScript*, uint32_t, int))&d_ret_code_runscriptsituation;
	int RunScriptSituation_HookProc(CVirtualMachine* vm, CVirtualMachineScript* script, uint32_t caller_id, int p4)
	{
		last_run_script_caller_id = caller_id;
		std::string script_name(script->script_name.text);

	#ifdef LAG_TEST
		lag_test_run_script(script_name);
	#endif

		last_script_ran.push(script->script_name.text);
		int ret = RunScriptSituation_Org(vm, script, caller_id, p4);
		last_script_ran.pop();

	#ifdef LAG_TEST
		lag_test_run_script_end(script_name, false, true);
	#endif

		return ret;
	}

	#define VM_STACK_UNDERFLOW 0xFFFFFD81
	#define VM_STACK_OVERFLOW 0xFFFFFD82
	class vm_exception_stack_underflow: public std::exception{};
	class vm_exception_stack_overflow: public std::exception{};

	void CScriptVarValue::delete_value()
	{
		if (value_type == VARTYPE_STRING)
		{
			delete (CExoString*)value;
		}
		else if (value_type >= 10)
		{
			DestroyGameDefinedStructure(NULL, value_type-10, value);
		}
	}
	CScriptVarValue::CScriptVarValue() : CScriptVarValue(0)
	{
	}
	CScriptVarValue::CScriptVarValue(const CScriptVarValue& copy)
	{
		value_type = copy.value_type;
		if (value_type == VARTYPE_STRING)
		{
			value = new CExoString(*(CExoString*)copy.value);
		}
		else if (value_type  >= 10)
		{
			value = CopyGameDefinedStructure(NULL, value_type-10, copy.value);
		}
		else
		{
			value = copy.value;
		}
	}
	CScriptVarValue::CScriptVarValue(CScriptVarValue&& move)
	{
		value_type = move.value_type;
		value = move.value;
		move.value_type = -1;
		move.value = 0;
	}
	CScriptVarValue::CScriptVarValue(int i)
	{
		value_type = VARTYPE_INT;
		value = *(void**)&i;
	}
	CScriptVarValue::CScriptVarValue(float f)
	{
		value_type = VARTYPE_FLOAT;
		value = *(void**)&f;
	}
	CScriptVarValue::CScriptVarValue(uint32_t o)
	{
		value_type = VARTYPE_INT;
		value = *(void**)&o;
	}
	CScriptVarValue::CScriptVarValue(const CExoString& str)
	{
		value_type = VARTYPE_STRING;
		value = new CExoString(str);
	}
	CScriptVarValue::CScriptVarValue(const std::string& str)
	{
		value_type = VARTYPE_STRING;
		value = new CExoString(str);
	}
	CScriptVarValue::CScriptVarValue(const char* str)
	{
		value_type = VARTYPE_STRING;
		value = new CExoString(str);
	}
	CScriptVarValue::CScriptVarValue(const Vector& vector) : CScriptVarValue(vector_as_array(vector))
	{
	}
	CScriptVarValue::CScriptVarValue(int struct_type, const void* data)
	{
		value_type = struct_type+10;
		value = CopyGameDefinedStructure(NULL, struct_type, data);
	}
	CScriptVarValue::CScriptVarValue(CGameEffect* effect) : CScriptVarValue(ENGINE_STRUCTURE_EFFECT, effect)
	{
	}
	CScriptVarValue::CScriptVarValue(const CScriptLocation& location) : CScriptVarValue(location_as_array(location))
	{
	}
	CScriptVarValue::CScriptVarValue(const CScriptArray& array)
	{
		value_type = ENGINE_STRUCTURE_ARRAY+10;
		value = new CScriptArray(new CScriptArrayData(*array));
	}
	CScriptVarValue::CScriptVarValue(const CScriptDictionary& dictionary)
	{
		value_type = ENGINE_STRUCTURE_DICTIONARY+10;
		value = new CScriptDictionary(new CScriptDictionaryData(*dictionary));
	}
	int CScriptVarValue::as_int()const
	{
		if (value_type == VARTYPE_INT) return *(int*)&value;
		else if (value_type == VARTYPE_FLOAT) return *(float*)&value;
		else if (value_type == VARTYPE_STRING && *((CExoString*)value)->text) return atoi(((CExoString*)value)->text);
		return 0;
	}
	float CScriptVarValue::as_float()const
	{
		if (value_type == VARTYPE_INT) return *(int*)&value;
		else if (value_type == VARTYPE_FLOAT) return *(float*)&value;
		return 0;
	}
	uint32_t CScriptVarValue::as_object()const
	{
		if (value_type == VARTYPE_INT) return *(int*)&value;
		return OBJECT_INVALID;
	}
	std::string CScriptVarValue::as_string()const
	{
		switch (value_type)
		{
			case VARTYPE_INT: return boost::lexical_cast<std::string>(*(int*)&value);
			case VARTYPE_FLOAT: return boost::lexical_cast<std::string>(*(float*)&value);
			case VARTYPE_STRING: return ((CExoString*)value)->to_str();
			case ENGINE_STRUCTURE_ARRAY+10: return "[array]";
			case ENGINE_STRUCTURE_DICTIONARY+10: return "[dictionary]";
			case ENGINE_STRUCTURE_EFFECT+10: return "[effect]";
			case ENGINE_STRUCTURE_ITEMPROPERTY+10: return "[itemproperty]";
			default: return "[bad type:" + boost::lexical_cast<std::string>(value_type) + "]";
		}
	}
	Vector CScriptVarValue::as_vector()const
	{
		if (value_type == ENGINE_STRUCTURE_ARRAY+10) return array_as_vector(*(CScriptArray*)value);
		return empty_vector;
	}
	CScriptArray* CScriptVarValue::as_array()const
	{
		if (value_type == ENGINE_STRUCTURE_ARRAY+10) return (CScriptArray*)value;
		return &empty_cscriptarray;
	}
	CScriptDictionary* CScriptVarValue::as_dictionary()const
	{
		if (value_type == ENGINE_STRUCTURE_DICTIONARY+10) return (CScriptDictionary*)value;
		return &empty_cscriptdictionary;
	}
	CGameEffect* CScriptVarValue::as_effect()const
	{
		if (value_type == ENGINE_STRUCTURE_EFFECT+10 || value_type == ENGINE_STRUCTURE_ITEMPROPERTY+10) return (CGameEffect*)value;
		return empty_effect;
	}
	CScriptLocation CScriptVarValue::as_location()const
	{
		if (value_type == ENGINE_STRUCTURE_ARRAY+10) return array_as_location(*(CScriptArray*)value);
		return empty_location;
	}
	CScriptVarValue& CScriptVarValue::operator=(CScriptVarValue&& assign)
	{
		if (&assign != this)
		{
			delete_value();
			value = assign.value;
			value_type = assign.value_type;
			assign.value=0;
			assign.value_type=-1;
		}
		return *this;
	}
	CScriptVarValue::~CScriptVarValue()
	{
		delete_value();
	}
	CScriptVarValue empty_script_value;

	CScriptDbResultData::CScriptDbResultData(mysql::CMySQLRes& res) : mysql_result(res.mysql_result)
	{

	}
	CScriptDbResultData::~CScriptDbResultData()
	{
		for (auto iter=mysql::mysql_server_results.begin(); iter!=mysql::mysql_server_results.end(); iter++)
		{
			if (iter->mysql_result == mysql_result)
			{
				mysql::mysql_server_results.erase(iter);
				break;
			}
		}
	}

	CExoString empty_cexostring;
	CScriptArray empty_cscriptarray(new CScriptArrayData);
	CScriptDictionary empty_cscriptdictionary(new CScriptDictionaryData);
	CScriptLocation empty_location;
	CGameEffect* empty_effect;
	Vector empty_vector;
	struct INIT_EMPTY_VARIABLES
	{
		INIT_EMPTY_VARIABLES()
		{
			empty_vector.x = 0;
			empty_vector.y = 0;
			empty_vector.z = 0;
			empty_location.loc_position = empty_vector;
			empty_location.loc_area = OBJECT_INVALID;
			empty_location.loc_orientation = empty_vector;
			empty_effect = (CGameEffect*)CopyGameDefinedStructure(NULL, ENGINE_STRUCTURE_EFFECT, NULL);
		}
	};
	INIT_EMPTY_VARIABLES static_init;

	bool poping_engine_struct = false;
	unsigned char d_ret_code_stackpopstruct[0x20];
	int (*StackPopEngineStructure_Org)(void*, int, void*) = (int (*)(void*, int, void*))&d_ret_code_stackpopstruct;
	int StackPopEngineStructure_HookProc(void* vm, int struct_type, void* value)
	{
		poping_engine_struct = true;
		int ret = StackPopEngineStructure_Org(vm, struct_type, value);
		poping_engine_struct = false;
		return ret;
	}
	void (*DestroyGameDefinedStructure)(void*, int, void*) = (void (*)(void*, int, void*))0x08231B28;
	void* (*CopyGameDefinedStructure)(void*, int, const void*) = (void* (*)(void*, int, const void*))0x081FB82C;
	unsigned char d_ret_code_copygamedefstruct[0x20];
	void* (*CopyGameDefinedStructure_Org)(void*, int, const void*) = (void* (*)(void*, int, const void*))&d_ret_code_copygamedefstruct;
	CGameEffect* last_copied_effect = NULL;
	void* CopyGameDefinedStructure_HookProc(void* command, int struct_type, const void* copy_value)
	{
		if (struct_type == ENGINE_STRUCTURE_EFFECT || struct_type == ENGINE_STRUCTURE_ITEMPROPERTY)
		{
			last_copied_effect = (CGameEffect*)copy_value;
		}

		if (poping_engine_struct) return (void*)copy_value;

		switch (struct_type)
		{
			case ENGINE_STRUCTURE_ARRAY:
				if (copy_value)
				{
					return new CScriptArray(*(CScriptArray*)copy_value);
				}
				else
				{
					return new CScriptArray(new CScriptArrayData);
				}
				break;
			case ENGINE_STRUCTURE_DBRESULT:
				if (copy_value)
				{
					return new CScriptDbResult(*(CScriptDbResult*)copy_value);
				}
				else
				{
					return new CScriptDbResult();
				}
				break;
			case ENGINE_STRUCTURE_DICTIONARY:
				if (copy_value)
				{
					return new CScriptDictionary(*(CScriptDictionary*)copy_value);
				}
				else
				{
					return new CScriptDictionary(new CScriptDictionaryData);
				}
				break;
			default:
				return CopyGameDefinedStructure_Org(command, struct_type, copy_value);
		}
	}
	unsigned char d_ret_code_destroygamedefstruct[0x20];
	void (*DestroyGameDefinedStructure_Org)(void*, int, void*) = (void (*)(void*, int, void*))&d_ret_code_destroygamedefstruct;
	void DestroyGameDefinedStructure_HookProc(void* command, int struct_type, void* value)
	{
		if (poping_engine_struct) return;

		switch (struct_type)
		{
			case ENGINE_STRUCTURE_ARRAY:
				delete (CScriptArray*)value;
				break;
			case ENGINE_STRUCTURE_DBRESULT:
				delete (CScriptDbResult*)value;
				break;
			case ENGINE_STRUCTURE_DICTIONARY:
				delete (CScriptDictionary*)value;
				break;
			default:
				return DestroyGameDefinedStructure_Org(command, struct_type, value);
		}
	}
	std::string vector_to_string(const Vector& v)
	{
		return boost::str(boost::format("%.2f#%.2f#%.2f")
			% v.x 
			% v.y 
			% v.z);
	}
	std::string location_to_string(const CScriptLocation& loc)
	{
		CNWSArea* area = GetAreaById(loc.loc_area);
		if (!area) return "";
		return boost::str(boost::format("%s#%.2f#%.2f#%.2f#%.2f")
			% area->area_tag.text
			% loc.loc_position.x
			% loc.loc_position.y
			% loc.loc_position.z
			% vector_to_angle(loc.loc_orientation));
	}
	Vector string_to_vector(const std::string& str)
	{
		Vector v;
		if (sscanf(str.c_str(), "%f#%f#%f", &v.x, &v.y, &v.z) == 3)
		{
			return v;
		}
		return empty_vector;
	}
	CScriptLocation string_to_location(const std::string& str)
	{
		CScriptLocation loc;
		char area_tag[33];
		float orientation = 0;
		if (sscanf(str.c_str(), "%32[^#]#%f#%f#%f#%f", area_tag, &loc.loc_position.x, &loc.loc_position.y, &loc.loc_position.z, &orientation))
		{
			loc.loc_area = find_object_by_tag(area_tag, 0);
			loc.loc_orientation = angle_to_vector(orientation);
			return loc;
		}
		return empty_location;
	}
	float vector_to_angle(const Vector& v)
	{
		return atan2(v.y, v.x) * (180.0 / M_PI);
	}
	Vector angle_to_vector(float angle)
	{
		Vector result;
		angle *= (M_PI / 180.0);
		result.x = cos(angle);
		result.y = sin(angle);
		result.z = 0;
		return result;
	}
	Vector array_as_vector(const CScriptArray& array)
	{
		if (array->size() == 3)
		{
			Vector result;
			result.x = array->at(0).as_float();
			result.y = array->at(1).as_float();
			result.z = array->at(2).as_float();
			return result;
		}
		return empty_vector;
	}
	CScriptArray vector_as_array(const Vector& v)
	{
		CScriptArray array(new CScriptArrayData);
		array->push_back(v.x);
		array->push_back(v.y);
		array->push_back(v.z);
		return array;
	}
	CScriptArray location_as_array(const CScriptLocation& l)
	{
		CScriptArray array(new CScriptArrayData);
		CNWSArea* area = GetAreaById(l.loc_area);
		array->push_back(area?area->area_tag:"OBJECT_INVALID");
		array->push_back(vector_as_array(l.loc_position));
		array->push_back(vector_to_angle(l.loc_orientation));
		return array;
	}
	CScriptLocation array_as_location(const CScriptArray& array)
	{
		if (array->size() == 3)
		{
			CScriptLocation result;
			result.loc_area = find_object_by_tag(array->at(0).as_string(), 0);
			result.loc_position = array_as_vector(*array->at(1).as_array());
			result.loc_orientation = angle_to_vector(array->at(2).as_float());
			return result;
		}
		return empty_location;
	}
	bool script_array_equal(CScriptArrayData* array1, CScriptArrayData* array2)
	{
		if (array1 == array2) return true;
		if (array1->size() != array2->size()) return false;
		for (uint32_t i=0; i<array1->size(); i++)
		{
			if (!script_var_value_equal(array1->at(i), array2->at(i))) return false;
		}
		return true;
	}
	bool script_dictionary_equal(CScriptDictionaryData* dict1, CScriptDictionaryData* dict2)
	{
		if (dict1 == dict2) return true;
		if (dict1->size()!=dict2->size()) return false;
		for (auto& pair : *dict1)
		{
			auto iterPair2 = dict2->find(pair.first);
			if (iterPair2 == dict2->end()) return false;
			if (!script_var_value_equal(pair.second, iterPair2->second)) return false;
		}
		return true;
	}
	bool script_var_value_equal(CScriptVarValue& value1, CScriptVarValue& value2)
	{
		if (value1.value_type != value2.value_type) return false;
		switch (value1.value_type)
		{
			case VARTYPE_STRING:return (((CExoString*)value1.value)->to_str()==((CExoString*)value2.value)->to_str());
			case ENGINE_STRUCTURE_ARRAY+10:return script_array_equal(((CScriptArray*)value1.value)->get(), ((CScriptArray*)value2.value)->get());
			case ENGINE_STRUCTURE_DICTIONARY+10:return script_dictionary_equal(((CScriptDictionary*)value1.value)->get(), ((CScriptDictionary*)value2.value)->get());
			default:return value1.value == value2.value;
		}
	}
	unsigned char d_ret_code_gamedefstructequal[0x20];
	int (*GameDefinedStructureEqual_Org)(void*, int, void*, void*) = (int (*)(void*, int, void*, void*))&d_ret_code_gamedefstructequal;
	int GameDefinedStructureEqual_HookProc(void* command, int struct_type, void* value1, void* value2)
	{
		switch (struct_type)
		{
			case ENGINE_STRUCTURE_ARRAY:return script_array_equal(((CScriptArray*)value1)->get(), ((CScriptArray*)value2)->get());
			case ENGINE_STRUCTURE_DBRESULT:
			{
				CScriptDbResultData* dbresult1 = ((CScriptDbResult*)value1)->get();
				CScriptDbResultData* dbresult2 = ((CScriptDbResult*)value2)->get();
				if (dbresult1 == dbresult2) return true;
				if (!dbresult1 || !dbresult2) return false;
				return (dbresult1->mysql_result == dbresult2->mysql_result);
			}
			case ENGINE_STRUCTURE_DICTIONARY:return script_dictionary_equal(((CScriptDictionary*)value1)->get(), ((CScriptDictionary*)value2)->get());
			default:return GameDefinedStructureEqual_Org(command, struct_type, value1, value2);
		}
	}

	int (*VMPopInteger)(CVirtualMachine*, int*) = (int (*)(CVirtualMachine*, int*))0x82629fc;
	int vm_pop_int()
	{
		int i;
		if (!VMPopInteger(virtual_machine, &i))
		{
			throw vm_exception_stack_underflow();
		}
		return i;
	}
	int (*VMPopString)(CVirtualMachine*, CExoString*) = (int (*)(CVirtualMachine*, CExoString*))0x8262c18;
	CExoString vm_pop_string()
	{
		CExoString exo_str;
		if (!VMPopString(virtual_machine, &exo_str))
		{
			throw vm_exception_stack_underflow();
		}
		return exo_str;
	}
	int (*VMPopFloat)(CVirtualMachine*, float*) = (int (*)(CVirtualMachine*, float*))0x08262ad8;
	float vm_pop_float()
	{
		float f;
		if (!VMPopFloat(virtual_machine, &f))
		{
			throw vm_exception_stack_underflow();
		}
		return f;
	}
	int (*VMPopObject)(void*, uint32_t*) = (int (*)(void*, uint32_t*))0x08262dfc;
	uint32_t vm_pop_object()
	{
		uint32_t object_id;
		if (!VMPopObject(virtual_machine, &object_id))
		{
			throw vm_exception_stack_underflow();
		}
		return object_id;
	}
	int (*VMPopVector)(CVirtualMachine*, Vector*) = (int (*)(CVirtualMachine*, Vector*))0x82643c8;
	Vector vm_pop_vector()
	{
		Vector vector;
		if (!VMPopVector(virtual_machine, &vector))
		{
			throw vm_exception_stack_underflow();
		}
		return vector;
	}
	int (*VMPopEngineStruct)(CVirtualMachine*, int, void**) = (int (*)(CVirtualMachine*, int, void**))0x8262cfc;
	CScriptLocation vm_pop_location()
	{
		CScriptLocation* script_location;
		if (!VMPopEngineStruct(virtual_machine, 2, (void**)&script_location))
		{
			throw vm_exception_stack_underflow();
		}
		CScriptLocation ret = *script_location;
		delete script_location;
		return ret;
	}
	CScriptArray vm_pop_array()
	{
		CScriptArray* script_array;
		if (!VMPopEngineStruct(virtual_machine, ENGINE_STRUCTURE_ARRAY, (void**)&script_array))
		{
			throw vm_exception_stack_underflow();
		}
		CScriptArray ret = *script_array;
		delete script_array;
		return ret;
	}
	std::unique_ptr<CGameEffect> vm_pop_effect()
	{
		CGameEffect* effect;
		int data_type = virtual_machine->vm_stack.stack_data_types[virtual_machine->vm_stack.size-1]-0x10;
		if (data_type != ENGINE_STRUCTURE_EFFECT && data_type != ENGINE_STRUCTURE_ITEMPROPERTY)
		{
			throw vm_exception_stack_underflow();
		}
		else if (!VMPopEngineStruct(virtual_machine, data_type, (void**)&effect))
		{
			throw vm_exception_stack_underflow();
		}
		return std::unique_ptr<CGameEffect>(effect);
	}
	CScriptDictionary vm_pop_dictionary()
	{
		CScriptDictionary* script_dictionary;
		if (!VMPopEngineStruct(virtual_machine, ENGINE_STRUCTURE_DICTIONARY, (void**)&script_dictionary))
		{
			throw vm_exception_stack_underflow();
		}
		CScriptDictionary ret = *script_dictionary;
		delete script_dictionary;
		return ret;
	}
	int (*VMPushEngineStructure)(CVirtualMachine*, int, void*) = (int (*)(CVirtualMachine*, int, void*))0x082644ac;
	void vm_push_array(CScriptArray* array)
	{
		if (!VMPushEngineStructure(virtual_machine, ENGINE_STRUCTURE_ARRAY, array))
		{
			throw vm_exception_stack_overflow();
		}
	}
	void vm_push_dictionary(CScriptDictionary* dictionary)
	{
		if (!VMPushEngineStructure(virtual_machine, ENGINE_STRUCTURE_DICTIONARY, dictionary))
		{
			throw vm_exception_stack_overflow();
		}
	}
	void vm_push_dbresult(CScriptDbResult* dbresult)
	{
		if (!VMPushEngineStructure(virtual_machine, ENGINE_STRUCTURE_DBRESULT, dbresult))
		{
			throw vm_exception_stack_overflow();
		}
	}
	void vm_push_location(CScriptLocation* location)
	{
		if (!VMPushEngineStructure(virtual_machine, ENGINE_STRUCTURE_LOCATION, location))
		{
			throw vm_exception_stack_overflow();
		}
	}
	void vm_push_location(const CScriptLocation& location)
	{
		vm_push_location((CScriptLocation*)&location);
	}
	void vm_push_effect(CGameEffect* effect)
	{
		int result;
		if (effect == NULL)
		{
			std::unique_ptr<CGameEffect> invalid_effect((CGameEffect*)CopyGameDefinedStructure_HookProc(NULL, ENGINE_STRUCTURE_EFFECT, NULL));
			result = VMPushEngineStructure(virtual_machine, ENGINE_STRUCTURE_EFFECT, invalid_effect.get());
		}
		else
		{
			result = VMPushEngineStructure(virtual_machine, ENGINE_STRUCTURE_EFFECT, effect);
		}

		if (!result)
		{
			throw vm_exception_stack_overflow();
		}
	}
	int (*VMPushInteger)(CVirtualMachine*, int) = (int (*)(CVirtualMachine*, int))0x826434c;
	void vm_push_int(int i)
	{
		if (!VMPushInteger(virtual_machine, i))
		{
			throw vm_exception_stack_overflow();
		}
	}
	int (*VMPushString)(CVirtualMachine*, CExoString*) = (int (*)(CVirtualMachine*, CExoString*))0x08264430;
	void vm_push_string(CExoString* str)
	{
		if (!VMPushString(virtual_machine, str))
		{
			throw vm_exception_stack_overflow();
		}
	}
	void vm_push_string(const std::string& str)
	{
		CExoString exo_str(str.c_str());
		vm_push_string(&exo_str);
	}
	int (*VMPushVector)(CVirtualMachine*, Vector) = (int (*)(CVirtualMachine*, Vector))0x08262b98;
	void vm_push_vector(const Vector& v)
	{
		if (!VMPushVector(virtual_machine, v))
		{
			throw vm_exception_stack_overflow();
		}
	}
	void vm_push_vector(Vector* v)
	{
		vm_push_vector(*v);
	}
	int (*VMPushFloat)(CVirtualMachine*, float) = (int (*)(CVirtualMachine*, float))0x08264388;
	void vm_push_float(float f)
	{
		if (!VMPushFloat(virtual_machine, f))
		{
			throw vm_exception_stack_overflow();
		}
	}
	int (*VMPushObject)(CVirtualMachine*, uint32_t) = (int (*)(CVirtualMachine*, uint32_t))0x08264514;
	void vm_push_object(uint32_t object_id)
	{
		if (!VMPushObject(virtual_machine, object_id))
		{
			throw vm_exception_stack_overflow();
		}
	}

	#define NEW_VM_CMD_INDEX 0x0350
	struct VM_COMMAND_PTR
	{
		VM_COMMAND_PTR()
		{
			flags = 0xFFFF0000;
			func = NULL;
		}
		int flags;
		void* func;
	};
	VM_COMMAND_PTR* cmd_array = NULL;
	std::unordered_map<uint32_t, void*>& get_new_vm_commands_vec()
	{
		static std::unordered_map<uint32_t, void*> new_vm_commands_vec;
		return new_vm_commands_vec;
	}
	std::unordered_map<uint32_t, std::string>& get_new_vm_commands_name_vec()
	{
		static std::unordered_map<uint32_t, std::string> new_vm_commands_name_vec;
		return new_vm_commands_name_vec;
	}
	std::unordered_set<uint16_t>& get_overriden_vm_commands()
	{
		static std::unordered_set<uint16_t> overriden_vm_commands;
		return overriden_vm_commands;
	}
	bool vm_func_reg_replace(const std::string& func_name, void* func, int num_args, ...)
	{
		va_list args;
		va_start(args, num_args);
		while (num_args--)
		{
			uint32_t vm_func_index = va_arg(args, uint32_t);
			get_new_vm_commands_vec()[vm_func_index] = func;
			get_new_vm_commands_name_vec()[vm_func_index] = func_name;
			get_overriden_vm_commands().insert(vm_func_index);
		}
		va_end(args);
		return true;
	}
	bool vm_func_reg_new(const std::string& func_name, void* func, int num_args, ...)
	{
		va_list args;
		va_start(args, num_args);
		while (num_args--)
		{
			uint32_t vm_func_index = va_arg(args, uint32_t);
			get_new_vm_commands_vec()[vm_func_index+NEW_VM_CMD_INDEX-1] = func;
			get_new_vm_commands_name_vec()[vm_func_index+NEW_VM_CMD_INDEX-1] = func_name;
		}
		va_end(args);
		return true;
	}
	uint32_t last_vm_command = -1;
	int new_vm_cmd_router(int p1, int p2)
	{
		last_vm_command = p2;
		try
		{
			void (*func)() = (void (*)())get_new_vm_commands_vec()[p2];
			if (func)
			{
				func();
				return 0;
			}
			else
			{
				return 0xfffffd7e;
			}
		}
		catch (vm_exception_stack_underflow& e)
		{
			fprintf(stderr, "vm_exception_stack_underflow: %s(%d)\n", get_new_vm_commands_name_vec()[p2].c_str(), p2);
			return VM_STACK_UNDERFLOW;
		}
		catch (vm_exception_stack_overflow& e)
		{
			fprintf(stderr, "vm_exception_stack_overflow: %s(%d)\n", get_new_vm_commands_name_vec()[p2].c_str(), p2);
			return VM_STACK_OVERFLOW;
		}
	}
	void** orginal_cmd_array = NULL;
	int old_vm_cmd_router(int p1, int p2)
	{
		last_vm_command = p2;
		return ((int (*)(int, int))orginal_cmd_array[p2])(p1, p2);
	}

	uint32_t last_new_vm_command_index = 0;
	void* OnInitializeVMCommands_AllocArray(uint32_t size)
	{
		uint32_t total_num_vm_commands = NEW_VM_CMD_INDEX+last_new_vm_command_index;
		cmd_array = new VM_COMMAND_PTR[total_num_vm_commands];
		for (uint32_t i=0; i<total_num_vm_commands; i++)
		{
			cmd_array[i].func = (void*)new_vm_cmd_router;
		}
		return cmd_array;
	}

	unsigned char d_ret_code_scripterror[0x20];
	int (*ReportScriptError_Org)(CNWVirtualMachineCommands*, CExoString*, int) = (int (*)(CNWVirtualMachineCommands*, CExoString*, int))&d_ret_code_scripterror;
	int ReportScriptError_HookProc(CNWVirtualMachineCommands* vm, CExoString* error_script, int error_code)
	{
		time_t t=time(NULL);
		char* timestamp = ctime(&t);
		timestamp[strlen(timestamp)-1] = 0;
		if (error_code == 632) //TMI
		{
			fprintf(stderr, "[%s]:TOO MANY INSTRUCTIONS ALERT!!!:%s(%d)\n", timestamp, error_script->text, last_vm_command);
			return ReportScriptError_Org(vm, error_script, error_code);
		}
		else if (error_code == 638) // stack overflow
		{
			fprintf(stderr, "[%s]:STACK OVERFLOW!!!:%s(%d)\n", timestamp, error_script->text, last_vm_command);
			return ReportScriptError_Org(vm, error_script, error_code);
		}
		else
		{
			fprintf(stderr, "[%s]:SCRIPT ERROR:%s(%d)(%d)\n", timestamp, error_script->text, error_code, last_vm_command);
			script_event::run("inc_scripterror", vm->cmd_self_id, {error_script->to_str(), error_code});
			return 0;
		}
	}

	unsigned char d_ret_code_readscriptfile[0x20];
	int (*ReadScriptFile_Org)(CVirtualMachine*, CExoString*) = (int (*)(CVirtualMachine*, CExoString*))&d_ret_code_readscriptfile;
	int ReadScriptFile_HookProc(CVirtualMachine* vm, CExoString* script)
	{
		int result = ReadScriptFile_Org(vm, script);
		if (result && result != -634/*res not found*/)
		{
			fprintf(stderr, "failed to read script %s : %x (%d|%x)\n", script->text, result, vm->vm_execute_script_stack_size, last_run_script_caller_id);
			if (result == -635)
			{
				vm->vm_execute_script_stack_size--;
			}
		}
		return result;
	}
	
	void module_loaded_init()
	{
		orginal_cmd_array = new void*[NEW_VM_CMD_INDEX];
		for (uint32_t cmd_index=0; cmd_index<NEW_VM_CMD_INDEX; cmd_index++)
		{
			orginal_cmd_array[cmd_index] = cmd_array[cmd_index].func;
			cmd_array[cmd_index].func = (void*)old_vm_cmd_router;
		}
		for (uint16_t overriden_vm_command : get_overriden_vm_commands())
		{
			cmd_array[overriden_vm_command].flags = 0xFFFF0000;
			cmd_array[overriden_vm_command].func = (void*)new_vm_cmd_router;
		}
	}

	void init()
	{
		hook_function(0x080a4fd0, (unsigned long)Shutdown_HookProc, d_ret_code_shutdown, 12);

		hook_function(0x08261f94, (unsigned long)RunScript_HookProc, d_ret_code_runscript, 12);
		hook_function(0x08262534, (unsigned long)RunScriptSituation_HookProc, d_ret_code_runscriptsituation, 12);
		hook_function(0x08261c2c, (unsigned long)ReadScriptFile_HookProc, d_ret_code_readscriptfile, 12);

		hook_function(0x08262cfc, (long)StackPopEngineStructure_HookProc, d_ret_code_stackpopstruct, 12);
		hook_function(0x081fb82c, (long)CopyGameDefinedStructure_HookProc, d_ret_code_copygamedefstruct, 10);
		hook_function(0x08231b28, (long)DestroyGameDefinedStructure_HookProc, d_ret_code_destroygamedefstruct, 10);
		hook_function(0x081fbb7c, (long)GameDefinedStructureEqual_HookProc, d_ret_code_gamedefstructequal, 12);
		hook_function(0x081fb68c, (long)ReportScriptError_HookProc, d_ret_code_scripterror, 9);

		for (auto vm_command : get_new_vm_commands_vec())
		{
			if (vm_command.first > last_new_vm_command_index) last_new_vm_command_index = vm_command.first;
		}
		enable_write(0x08231A3B+2);
		*(uint32_t*)(0x08231A3B+2) = NEW_VM_CMD_INDEX + last_new_vm_command_index - 1;
		hook_call(0x081F5CD5, (uint32_t)OnInitializeVMCommands_AllocArray);
		
		register_hook(hook::module_loading, []{
			module_loaded_init();
		});
	}
	REGISTER_INIT(init);

	void unregister_event(const std::string& event, const std::string& script)
	{
		auto& vec_event = registered_events[event];
		for (uint32_t script_index=0; script_index<vec_event.size(); script_index++)
		{
			if (vec_event.at(script_index) == script)
			{
				vec_event.erase(vec_event.begin()+script_index);
			}
		}
		if (vec_event.size() == 0) registered_events.erase(event);
	}
	VM_FUNC_NEW(RegisterEvent, 241)
	{
		std::string event = vm_pop_string();
		std::string script = vm_pop_string();
		if (!event.empty() && !script.empty())
		{
			unregister_event(event, script);
			registered_events[event].push_back(script);
		}
	}
	VM_FUNC_NEW(UnregisterEvent, 242)
	{
		std::string event = vm_pop_string();
		std::string script = vm_pop_string();
		unregister_event(event, script);
	}
	VM_FUNC_NEW(ByPassRegisteredEvent, 243)
	{
		bBypassRegisteredEvent = true;
	}
	VM_FUNC_NEW(GetCurrentScriptName, 145)
	{
		CExoString result(get_last_script_ran().c_str());
		vm_push_string(&result);
	}

}
}
