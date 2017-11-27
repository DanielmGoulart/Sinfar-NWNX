#pragma once

#include "core.h"
#include "mysql.h"
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace nwnx { namespace nwscript {

std::string get_last_script_ran();
bool has_registered_events(const std::string& script_name);

int vm_pop_int();
CExoString vm_pop_string();
float vm_pop_float();
uint32_t vm_pop_object();
Vector vm_pop_vector();
CScriptLocation vm_pop_location();
class CScriptVarValue;
typedef std::vector<CScriptVarValue> CScriptArrayData;
typedef std::shared_ptr<CScriptArrayData> CScriptArray;
class CScriptDictionaryData: public std::map<std::string, CScriptVarValue>
{
public:
	iterator nwscript_iter;
	CScriptDictionaryData(): std::map<std::string, CScriptVarValue>(), nwscript_iter(begin())
	{
	}
};
typedef std::shared_ptr<CScriptDictionaryData> CScriptDictionary;
extern CExoString empty_cexostring;
extern CScriptArray empty_cscriptarray;
extern CScriptDictionary empty_cscriptdictionary;
extern CScriptLocation empty_location;
extern CGameEffect* empty_effect;
extern Vector empty_vector;
extern void (*DestroyGameDefinedStructure)(void*, int, void*);
extern void* (*CopyGameDefinedStructure)(void*, int, const void*);
extern uint32_t last_vm_command;
std::string vector_to_string(const Vector& v);
std::string location_to_string(const CScriptLocation& loc);
Vector string_to_vector(const std::string& str);
CScriptLocation string_to_location(const std::string& str);
class CScriptVarValue
{
private:
	void delete_value();
public:
	int value_type;
	void* value;
	CScriptVarValue();
	CScriptVarValue(const CScriptVarValue& copy);
	CScriptVarValue(CScriptVarValue&& move);
	CScriptVarValue(int i);
	CScriptVarValue(float f);
	CScriptVarValue(uint32_t o);
	CScriptVarValue(const CExoString& str);
	CScriptVarValue(const std::string& str);
	CScriptVarValue(const char* str);
	CScriptVarValue(const Vector& vector);
	CScriptVarValue(int struct_type, const void* data);
	CScriptVarValue(CGameEffect* effect);
	CScriptVarValue(const CScriptLocation& location);
	CScriptVarValue(const CScriptArray& array);
	CScriptVarValue(const CScriptDictionary& dictionary);
	int as_int()const;
	float as_float()const;
	uint32_t as_object()const;
	std::string as_string()const;
	CScriptArray* as_array()const;
	CScriptDictionary* as_dictionary()const;
	CGameEffect* as_effect()const;
	CScriptLocation as_location()const;
	Vector as_vector()const;
	CScriptVarValue& operator=(CScriptVarValue&& assign);
	~CScriptVarValue();
};
extern CScriptVarValue empty_script_value;
class CScriptDbResultData
{
public:
	CScriptDbResultData() = delete;
	CScriptDbResultData(mysql::CMySQLRes& res);
	~CScriptDbResultData();
	MYSQL_RES* mysql_result;
};
typedef std::shared_ptr<CScriptDbResultData> CScriptDbResult;
extern CGameEffect* last_copied_effect;
float vector_to_angle(const Vector& v);
Vector angle_to_vector(float angle);
Vector array_as_vector(const CScriptArray& array);
CScriptArray vector_as_array(const Vector& v);
CScriptArray location_as_array(const CScriptLocation& l);
CScriptLocation array_as_location(const CScriptArray& array);
bool script_array_equal(CScriptArrayData* array1, CScriptArrayData* array2);
bool script_dictionary_equal(CScriptDictionaryData* dict1, CScriptDictionaryData* dict2);
bool script_var_value_equal(CScriptVarValue& value1, CScriptVarValue& value2);
CScriptArray vm_pop_array();
std::unique_ptr<CGameEffect> vm_pop_effect();
CScriptDictionary vm_pop_dictionary();
void vm_push_array(CScriptArray* array);
void vm_push_dictionary(CScriptDictionary* dictionary);
void vm_push_dbresult(CScriptDbResult* dbresult);
void vm_push_location(const CScriptLocation& location);
void vm_push_location(CScriptLocation* location);
void vm_push_effect(CGameEffect* effect);
void vm_push_int(int i);
void vm_push_string(CExoString* str);
void vm_push_string(const std::string& str);
void vm_push_vector(const Vector& v);
void vm_push_vector(Vector* v);
void vm_push_float(float f);
void vm_push_object(uint32_t object_id);
int new_vm_cmd_router(int p1, int p2);
bool vm_func_reg_replace(const std::string& func_name, void* func, int num_args, ...);
bool vm_func_reg_new(const std::string& func_name, void* func, int num_args, ...);
struct VM_FUNC_REG_HOLDER{VM_FUNC_REG_HOLDER(bool b){}};
#define VM_FUNC_NEW(func_name, ...) \
	void vm_func_##func_name(); \
	VM_FUNC_REG_HOLDER vm_func_reg_##func_name(vm_func_reg_new(#func_name, (void*)vm_func_##func_name, (sizeof((int[]){__VA_ARGS__})/sizeof(int)), __VA_ARGS__)); \
	void vm_func_##func_name()
#define VM_FUNC_REPLACE(func_name, ...) \
	void vm_func_##func_name(); \
	VM_FUNC_REG_HOLDER vm_func_reg_##func_name(vm_func_reg_replace(#func_name, (void*)vm_func_##func_name, (sizeof((int[]){__VA_ARGS__})/sizeof(int)), __VA_ARGS__)); \
	void vm_func_##func_name()

}
}
