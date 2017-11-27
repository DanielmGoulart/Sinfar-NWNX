#include "nwscript_funcs.h"

#include <sys/time.h>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <json/writer.h>
#include <json/reader.h>

namespace {

VM_FUNC_REPLACE(GetSubString, 65)
{
	CExoString source = vm_pop_string();
	int index = vm_pop_int();
	int length = vm_pop_int();
	CExoString result;
	if (source.text && *source.text && length)
	{
		int total_len = strlen(source.text);
		if (index < 0) index = total_len+index;
		if (index >= 0 && index < total_len)
		{
			int max_len = total_len-index;
			if (length > max_len) length = max_len;
			else if (length < 0) length = max_len+length;
			if (length > 0)
			{
				result.len = length;
				result.text = (char*)malloc(length+1);
				memcpy(result.text, source.text+index, length);
				result.text[length] = 0;
			}
		}
	}
	vm_push_string(&result);
}

VM_FUNC_NEW(IntToObject, 515)
{
    vm_push_object(vm_pop_int());
}

VM_FUNC_NEW(StringToObject, 516)
{
    std::string value = vm_pop_string();
    vm_push_object(strtoul(value.c_str(), NULL, 16));
}

VM_FUNC_NEW(MemGet, 154)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int offset = vm_pop_int();
	int size = vm_pop_int();
	int value = 0;
	if (o)
	{
		void* object = NULL;
		switch (o->type)
		{
			case OBJECT_TYPE_MODULE: object = get_module(); break;
			case OBJECT_TYPE_AREA: object = o->vtable->AsNWSArea(o); break;
			case OBJECT_TYPE_CREATURE: object = o->vtable->AsNWSCreature(o); break;
			case OBJECT_TYPE_ITEM: object = o->vtable->AsNWSItem(o); break;
			case OBJECT_TYPE_TRIGGER: object = o->vtable->AsNWSTrigger(o); break;
			case OBJECT_TYPE_PLACEABLE: object = o->vtable->AsNWSPlaceable(o); break;
			case OBJECT_TYPE_DOOR: object = o->vtable->AsNWSDoor(o); break;
			case OBJECT_TYPE_AREA_OF_EFFECT: object = o->vtable->AsNWSAreaOfEffectObject(o); break;
			case OBJECT_TYPE_WAYPOINT: object = o->vtable->AsNWSWaypoint(o); break;
			case OBJECT_TYPE_ENCOUNTER: object = o->vtable->AsNWSEncounter(o); break;
			case OBJECT_TYPE_STORE: object = o->vtable->AsNWSStore(o); break;
		}
		if (object)
		{
			void* addr = ((char*)object)+offset;
			memcpy(&value, addr, size);
		}
	}
	vm_push_int(value);
}
VM_FUNC_NEW(MemSet, 155)
{
	CGameObject* o = GetGameObject(vm_pop_object());
	int offset = vm_pop_int();
	int size = vm_pop_int();
	int value = vm_pop_int();
	if (offset < 0x8048000)
	{
		if (o == NULL) return;
		void* object = NULL;
		switch (o->type)
		{
			case OBJECT_TYPE_MODULE: object = get_module(); break;
			case OBJECT_TYPE_AREA: object = o->vtable->AsNWSArea(o); break;
			case OBJECT_TYPE_CREATURE: object = o->vtable->AsNWSCreature(o); break;
			case OBJECT_TYPE_ITEM: object = o->vtable->AsNWSItem(o); break;
			case OBJECT_TYPE_TRIGGER: object = o->vtable->AsNWSTrigger(o); break;
			case OBJECT_TYPE_PLACEABLE: object = o->vtable->AsNWSPlaceable(o); break;
			case OBJECT_TYPE_DOOR: object = o->vtable->AsNWSDoor(o); break;
			case OBJECT_TYPE_AREA_OF_EFFECT: object = o->vtable->AsNWSAreaOfEffectObject(o); break;
			case OBJECT_TYPE_WAYPOINT: object = o->vtable->AsNWSWaypoint(o); break;
			case OBJECT_TYPE_ENCOUNTER: object = o->vtable->AsNWSEncounter(o); break;
			case OBJECT_TYPE_STORE: object = o->vtable->AsNWSStore(o); break;
		}
		if (object == NULL) return;
		offset += (int)object;
	}
	enable_write((unsigned long)offset);
	memcpy((void*)offset, &value, size);
}

VM_FUNC_NEW(GetArraySize, 58)
{
	CScriptArray array = vm_pop_array();
	vm_push_int(array->size());
}
VM_FUNC_NEW(DeleteArrayItem, 59)
{
	CScriptArray array = vm_pop_array();
	uint32_t index = vm_pop_int();
	if (index < array->size())
	{
		array->erase(array->begin()+index);
	}
}
VM_FUNC_NEW(CopyArray, 60)
{
	CScriptArray new_array(new CScriptArrayData(*vm_pop_array()));
	vm_push_array(&new_array);
}
VM_FUNC_NEW(SortArray, 61)
{
	CScriptArray array = vm_pop_array();
	std::sort(array->begin(), array->end(),
		[](const CScriptVarValue& a, const CScriptVarValue& b) -> bool
	{
		if (a.value_type != b.value_type) return a.value_type < b.value_type;
		switch (a.value_type)
		{
			case VARTYPE_STRING:
			{
				CExoString* a_val = (CExoString*)a.value;
				CExoString* b_val = (CExoString*)b.value;
				return (strcmp(a_val->text?a_val->text:"", b_val->text?b_val->text:"") < 0);
			}
			default:
				return (a.value < b.value);
		}
	});
}
inline CScriptVarValue& vm_SetupSetArrayItem()
{
	CScriptArray array = vm_pop_array();
	uint32_t index = vm_pop_int();
	if (index >= array->size())
	{
		if (index - array->size() > 1000)
		{
			fprintf(stderr, "Trying to set an array element at a too high item index:%u\n", index);
			index = array->size();
		}
		array->resize(index+1);
	}
	return array->at(index);
}
CScriptVarValue empty_var_value;
inline CScriptVarValue& vm_SetupGetArrayItem()
{
	CScriptArray array = vm_pop_array();
	uint32_t index = vm_pop_int();
	if (index < array->size())
	{
		return array->at(index);
	}
	return empty_var_value;
}
VM_FUNC_NEW(GetArrayValueType, 330)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_int(item_value.value_type);
}
//int
VM_FUNC_NEW(AddArrayInt, 62)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptVarValue(vm_pop_int()));
}
VM_FUNC_NEW(SetArrayInt, 63)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vm_pop_int());
}
VM_FUNC_NEW(GetArrayInt, 64)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_int(item_value.as_int());
}
//float
VM_FUNC_NEW(AddArrayFloat, 65)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptVarValue(vm_pop_float()));
}
VM_FUNC_NEW(SetArrayFloat, 66)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vm_pop_float());
}
VM_FUNC_NEW(GetArrayFloat, 67)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_float(item_value.as_float());
}
//object
VM_FUNC_NEW(AddArrayObject, 68)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptVarValue(vm_pop_object()));
}
VM_FUNC_NEW(SetArrayObject, 69)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vm_pop_object());
}
VM_FUNC_NEW(GetArrayObject, 70)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_object(item_value.as_object());
}
//string
VM_FUNC_NEW(AddArrayString, 71)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptVarValue(vm_pop_string()));
}
VM_FUNC_NEW(SetArrayString, 72)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vm_pop_string());
}
VM_FUNC_NEW(GetArrayString, 73)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	CExoString result(item_value.as_string().c_str());
	vm_push_string(&result);
}
//vector
VM_FUNC_NEW(AddArrayVector, 74)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptVarValue(vector_as_array(vm_pop_vector())));
}
VM_FUNC_NEW(SetArrayVector, 75)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vector_as_array(vm_pop_vector()));
}
VM_FUNC_NEW(GetArrayVector, 76)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	Vector result = array_as_vector(*item_value.as_array());
	vm_push_vector(&result);
}
//location
VM_FUNC_NEW(AddArrayLocation, 77)
{
	CScriptArray array = vm_pop_array();
	array->push_back(location_as_array(vm_pop_location()));
}
VM_FUNC_NEW(SetArrayLocation, 78)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(location_as_array(vm_pop_location()));
}
VM_FUNC_NEW(GetArrayLocation, 79)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	CScriptLocation result = array_as_location(*item_value.as_array());
	vm_push_location(&result);
}
//array
VM_FUNC_NEW(AddArrayArray, 80)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptArray(vm_pop_array()));
}
VM_FUNC_NEW(SetArrayArray, 81)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vm_pop_array());
}
VM_FUNC_NEW(GetArrayArray, 82)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_array(item_value.as_array());
}
//dictionary
VM_FUNC_NEW(AddArrayDictionary, 83)
{
	CScriptArray array = vm_pop_array();
	array->push_back(CScriptDictionary(vm_pop_dictionary()));
}
VM_FUNC_NEW(SetArrayDictionary, 84)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = CScriptVarValue(vm_pop_dictionary());
}
VM_FUNC_NEW(GetArrayDictionary, 85)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_dictionary(item_value.as_dictionary());
}
VM_FUNC_NEW(ArrayToString, 86)
{
	CScriptArray array = vm_pop_array();
	std::string separator = vm_pop_string();
	std::string result_std;
	for (CScriptVarValue& var_value : *array)
	{
		if (!result_std.empty()) result_std += separator;
		result_std += var_value.as_string();
	}
	CExoString result(result_std.c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(StringToArray, 87)
{
	std::string source = vm_pop_string();
	std::string separator = vm_pop_string();
	CScriptArray result(new CScriptArrayData);
	boost::tokenizer<boost::char_separator<char>> tokens(source, boost::char_separator<char>(separator.c_str()));
	for(boost::tokenizer<boost::char_separator<char>>::iterator iter=tokens.begin(); iter!=tokens.end(); iter++)
	{
		result->push_back(*iter);
	}
	vm_push_array(&result);
}
VM_FUNC_NEW(GetDictionarySize, 88)
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	vm_push_int(dictionary->size());
}
VM_FUNC_NEW(DeleteDictionaryItem, 89)
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CExoString key = vm_pop_string();
	dictionary->erase(key);
}
VM_FUNC_NEW(CopyDictionary, 90)
{
	CScriptDictionary new_dictionary(new CScriptDictionaryData(*vm_pop_dictionary()));
	vm_push_dictionary(&new_dictionary);
}
VM_FUNC_NEW(GetFirstDictionaryPair, 91)
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CScriptArray result(new CScriptArrayData);
	dictionary->nwscript_iter = dictionary->begin();
	if (dictionary->nwscript_iter != dictionary->end())
	{
		result->push_back(dictionary->nwscript_iter->first);
		result->push_back(dictionary->nwscript_iter->second);
	}
	vm_push_array(&result);
}
VM_FUNC_NEW(GetNextDictionaryPair, 92)
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CScriptArray result(new CScriptArrayData);
	if (dictionary->nwscript_iter != dictionary->end())
	{
		dictionary->nwscript_iter++;
		if (dictionary->nwscript_iter != dictionary->end())
		{
			result->push_back(dictionary->nwscript_iter->first);
			result->push_back(dictionary->nwscript_iter->second);
		}
	}
	vm_push_array(&result);
}
VM_FUNC_NEW(GetDictionaryKeys, 93)
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CScriptArray result(new CScriptArrayData);
	for (auto& iter : *dictionary)
	{
		result->push_back(iter.first);
	}
	vm_push_array(&result);
}
VM_FUNC_NEW(GetDictionaryValues, 94)
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CScriptArray result(new CScriptArrayData);
	for (auto& iter : *dictionary)
	{
		result->push_back(iter.second);
	}
	vm_push_array(&result);
}
inline CScriptVarValue& vm_SetupSetDictionaryItem()
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CExoString key = vm_pop_string();
	return (*dictionary)[key];
}
inline CScriptVarValue& vm_SetupGetDictionaryItem()
{
	CScriptDictionary dictionary = vm_pop_dictionary();
	CExoString key = vm_pop_string();
	auto iter = dictionary->find(key);
	if (iter != dictionary->end())
	{
		return iter->second;
	}
	return empty_var_value;
}
VM_FUNC_NEW(GetDictionaryValueType, 331)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_int(item_value.value_type);
}
//int
VM_FUNC_NEW(SetDictionaryInt, 95)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(vm_pop_int());
}
VM_FUNC_NEW(GetDictionaryInt, 96)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_int(item_value.as_int());
}
//float
VM_FUNC_NEW(SetDictionaryFloat, 97)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(vm_pop_float());
}
VM_FUNC_NEW(GetDictionaryFloat, 98)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_float(item_value.as_float());
}
//object
VM_FUNC_NEW(SetDictionaryObject, 99)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(vm_pop_object());
}
VM_FUNC_NEW(GetDictionaryObject, 100)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_object(item_value.as_object());
}
//string
VM_FUNC_NEW(SetDictionaryString, 101)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(vm_pop_string());
}
VM_FUNC_NEW(GetDictionaryString, 102)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	CExoString result(item_value.as_string().c_str());
	vm_push_string(&result);
}
//vector
VM_FUNC_NEW(SetDictionaryVector, 103)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(vector_as_array(vm_pop_vector()));
}
VM_FUNC_NEW(GetDictionaryVector, 104)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	Vector result = array_as_vector(*item_value.as_array());
	vm_push_vector(&result);
}
VM_FUNC_NEW(SetDictionaryLocation, 105)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(location_as_array(vm_pop_location()));
}
VM_FUNC_NEW(GetDictionaryLocation, 106)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	CScriptLocation result = array_as_location(*item_value.as_array());
	vm_push_location(&result);
}
//array
VM_FUNC_NEW(SetDictionaryArray, 107)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = CScriptVarValue(vm_pop_array());
}
VM_FUNC_NEW(GetDictionaryArray, 108)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_array(item_value.as_array());
}
//dictionary
VM_FUNC_NEW(SetDictionaryDictionary, 109)
{
	//manual setup because the if dictionary to insert is the owner,
	//then the new array need to be created before calling the [] operator
	//because it would add a empty element to the inserted array
	CScriptDictionary dictionary = vm_pop_dictionary();
	CExoString key = vm_pop_string();
	CScriptVarValue dictionary_to_insert(vm_pop_dictionary());
	(*dictionary)[key] = std::move(dictionary_to_insert);
}
VM_FUNC_NEW(GetDictionaryDictionary, 110)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_dictionary(item_value.as_dictionary());
}

VM_FUNC_NEW(TrimString, 111)
{
	std::string source = vm_pop_string();
	boost::trim(source);
	CExoString result(source.c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(ReplaceSubString, 112)
{
	std::string source = vm_pop_string();
	std::string substr = vm_pop_string();
	std::string replace = vm_pop_string();
	boost::replace_all(source, substr, replace);
	CExoString result(source.c_str());
	vm_push_string(&result);
}
inline boost::regex create_script_regex(const std::string& regex_str, int nwscript_flags)
{
	int flags = boost::regex_constants::extended;
	if (nwscript_flags & 1) flags |= boost::regex_constants::icase;
	boost::regex result;
	try
	{
		return boost::regex(regex_str, flags);
	}
	catch (const boost::regex_error& error)
	{
		log::logf(log::error, "Bad Regex: %s", error.what());
		return boost::regex("", flags);
	}
}
VM_FUNC_NEW(RegexTest, 113)
{
	std::string source = vm_pop_string();
	std::string regex_str = vm_pop_string();
	int flags = vm_pop_int();
	boost::regex regex = create_script_regex(regex_str, flags);
	vm_push_int(boost::regex_match(source, regex, boost::regex_constants::match_any));
}
VM_FUNC_NEW(RegexMatch, 114)
{
	std::string source = vm_pop_string();
	std::string regex_str = vm_pop_string();
	int flags = vm_pop_int();
	boost::regex regex = create_script_regex(regex_str, flags);
	CScriptArray result(new CScriptArrayData);
	boost::smatch match;
	if (boost::regex_match(source, match, regex, boost::regex_constants::match_any))
	{
		for (size_t match_index=0; match_index<match.size(); match_index++)
		{
			result->push_back(match[match_index].str());
		}
	}
	vm_push_array(&result);
}
VM_FUNC_NEW(RegexReplace, 115)
{
	std::string source = vm_pop_string();
	std::string regex_str = vm_pop_string();
	std::string replacement = vm_pop_string();
	int flags = vm_pop_int();
	boost::regex regex = create_script_regex(regex_str, flags);
	std::string result_std = boost::regex_replace(source, regex, replacement, boost::regex_constants::match_any);
	CExoString result(result_std.c_str());
	vm_push_string(&result);
}

Json::FastWriter json_writer;
Json::Reader json_reader;
Json::Value json_null;
inline float json_value_as_float(Json::Value& json_value)
{
	if (json_value.isNumeric())
	{
		return json_value.asFloat();
	}
	return 0;
}
inline int json_value_as_int(Json::Value& json_value)
{
	if (json_value.isNumeric())
	{
		return json_value.asInt();
	}
	return 0;
}
inline std::string json_value_as_string(Json::Value& json_value)
{
	if (json_value.isString())
	{
		return json_value.asString();
	}
	return "";
}
inline Json::Value script_var_as_json_value(CScriptVarValue& script_var);
inline Json::Value array_as_json_value(CScriptArray array)
{
	Json::Value result_json(Json::ValueType::arrayValue);
	for (CScriptVarValue& script_var : *array)
	{
		result_json.append(script_var_as_json_value(script_var));
	}
	return result_json;
}
inline Json::Value dictionary_as_json_value(CScriptDictionary dictionary)
{
	Json::Value result_json(Json::ValueType::objectValue);
	for (auto& pair : *dictionary)
	{
		result_json[pair.first] = script_var_as_json_value(pair.second);
	}
	return result_json;
}
inline CScriptVarValue json_value_as_script_var(Json::Value& json_value);
inline CScriptArray json_value_as_array(Json::Value& json_value)
{
	CScriptArray array(new CScriptArrayData);
	if (json_value.isArray())
	{
		for (Json::Value& array_value : json_value)
		{
			array->push_back(json_value_as_script_var(array_value));
		}
	}
	return array;
}
inline CScriptDictionary json_value_as_dictionary(Json::Value& json_value)
{
	CScriptDictionary dictionary(new CScriptDictionaryData);
	if (json_value.isObject())
	{
		for (Json::Value::iterator iter=json_value.begin(); iter!=json_value.end(); iter++)
		{
			(*dictionary)[iter.name()] = json_value_as_script_var(*iter);
		}
	}
	return dictionary;
}
inline CScriptVarValue json_value_as_script_var(Json::Value& json_value)
{
	if (json_value.isInt())
	{
		return CScriptVarValue(json_value_as_int(json_value));
	}
	else if (json_value.isDouble())
	{
		return CScriptVarValue(json_value_as_float(json_value));
	}
	else if (json_value.isString())
	{
		return CScriptVarValue(json_value_as_string(json_value));
	}
	else if (json_value.isObject())
	{
		return CScriptVarValue(json_value_as_dictionary(json_value));
	}
	else if (json_value.isArray())
	{
		return CScriptVarValue(json_value_as_array(json_value));
	}
	return CScriptVarValue();
}
inline Json::Value script_var_as_json_value(CScriptVarValue& script_var)
{
	switch (script_var.value_type)
	{
		case VARTYPE_INT:return Json::Value(*(int*)&script_var.value);
		case VARTYPE_FLOAT:return Json::Value(*(float*)&script_var.value);
		case VARTYPE_STRING:return Json::Value(((CExoString*)script_var.value)->to_str());
		case ENGINE_STRUCTURE_ARRAY+10: return array_as_json_value(*(CScriptArray*)script_var.value);
		case ENGINE_STRUCTURE_DICTIONARY+10:return dictionary_as_json_value(*(CScriptDictionary*)script_var.value);
		default:return json_null;
	}
}
VM_FUNC_NEW(StringToJSON, 116)
{
	Json::Value result_json(vm_pop_string());
	CExoString result(json_writer.write(result_json).c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(StringFromJSON, 117)
{
	std::string json_input = vm_pop_string();
	Json::Value result_json;
	json_reader.parse(json_input, result_json);
	CExoString result(json_value_as_string(result_json).c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(VectorToJSON, 118)
{
	Vector v = vm_pop_vector();
	Json::Value result_json = array_as_json_value(vector_as_array(v));
	CExoString result(json_writer.write(result_json).c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(VectorFromJSON, 119)
{
	std::string json_input = vm_pop_string();
	Json::Value result_json;
	json_reader.parse(json_input, result_json);
	Vector result = array_as_vector(json_value_as_array(result_json));
	vm_push_vector(&result);
}
VM_FUNC_NEW(LocationToJSON, 120)
{
	CScriptLocation l = vm_pop_location();
	Json::Value result_json = array_as_json_value(location_as_array(l));
	CExoString result(json_writer.write(result_json).c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(LocationFromJSON, 121)
{
	std::string json_input = vm_pop_string();
	Json::Value result_json;
	json_reader.parse(json_input, result_json);
	CScriptLocation result = array_as_location(json_value_as_array(result_json));
	vm_push_location(&result);
}
VM_FUNC_NEW(ArrayToJSON, 122)
{
	CScriptArray array = vm_pop_array();
	Json::Value result_json = array_as_json_value(array);
	CExoString result(json_writer.write(result_json).c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(ArrayFromJSON, 123)
{
	std::string json_input = vm_pop_string();
	Json::Value result_json;
	json_reader.parse(json_input, result_json);
	CScriptArray result = json_value_as_array(result_json);
	vm_push_array(&result);
}
VM_FUNC_NEW(DictionaryToJSON, 124)
{
	CScriptDictionary array = vm_pop_dictionary();
	Json::Value result_json = dictionary_as_json_value(array);
	CExoString result(json_writer.write(result_json).c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(DictionaryFromJSON, 125)
{
	std::string json_input = vm_pop_string();
	Json::Value result_json;
	json_reader.parse(json_input, result_json);
	CScriptDictionary result = json_value_as_dictionary(result_json);
	vm_push_dictionary(&result);
}
VM_FUNC_NEW(AddArrayEffect, 413)
{
	CScriptArray array = vm_pop_array();
	array->push_back(vm_pop_effect().get());
}
VM_FUNC_NEW(SetArrayEffect, 414)
{
	CScriptVarValue& item_value = vm_SetupSetArrayItem();
	item_value = vm_pop_effect().get();
}
VM_FUNC_NEW(GetArrayEffect, 415)
{
	CScriptVarValue& item_value = vm_SetupGetArrayItem();
	vm_push_effect(item_value.as_effect());
}
VM_FUNC_NEW(SetDictionaryEffect, 416)
{
	CScriptVarValue& item_value = vm_SetupSetDictionaryItem();
	item_value = vm_pop_effect().get();
}
VM_FUNC_NEW(GetDictionaryEffect, 417)
{
	CScriptVarValue& item_value = vm_SetupGetDictionaryItem();
	vm_push_effect(item_value.as_effect());
}

int (*GetRunScriptReturnValue)(CVirtualMachine*, int*, int*) = (int (*)(CVirtualMachine*, int*, int*))0x08264324;
VM_FUNC_NEW(ExecuteConditionalScript, 169)
{
	CExoString script_name = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	int result = 0;
	if (run_script(&script_name, object_id, 1))
	{
		int type;
		int value;
		if (GetRunScriptReturnValue(virtual_machine, &type, &value))
		{
			result = value;
		}
	}
	vm_push_int(result);
}

typedef std::map<std::string, std::string> map_string;
std::map<std::string, map_string> script_maps;
VM_FUNC_NEW(SetMapString, 269)
{
	std::string map_name = vm_pop_string();
	std::string map_key = vm_pop_string();
	std::string map_string = vm_pop_string();
	script_maps[map_name][map_key] = map_string;
}
VM_FUNC_NEW(GetMapString, 270)
{
	std::string map_name = vm_pop_string();
	std::string map_key = vm_pop_string();
	CExoString result;
	map_string& script_map = script_maps[map_name];
	map_string::iterator map_string_iter = script_map.find(map_key);
	if (map_string_iter != script_map.end())
	{
		result = CExoString(map_string_iter->second);
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(DeleteMapString, 271)
{
	std::string map_name = vm_pop_string();
	std::string map_key = vm_pop_string();
	script_maps[map_name].erase(map_key);
}
typedef std::unordered_set<uint32_t> set_objid;
std::map<std::string, set_objid> script_reg_objects;
VM_FUNC_NEW(RegisterObject, 272)
{
	std::string set_name = vm_pop_string();
	uint32_t obj_id = vm_pop_object();
	script_reg_objects[set_name].insert(obj_id);
}
VM_FUNC_NEW(UnregisterObject, 273)
{
	std::string set_name = vm_pop_string();
	uint32_t obj_id = vm_pop_object();
	script_reg_objects[set_name].erase(obj_id);
}
VM_FUNC_NEW(UnregisterAllObjects, 274)
{
	std::string set_name = vm_pop_string();
	script_reg_objects[set_name].clear();
}
VM_FUNC_NEW(RunScriptOnAllRegisteredObjects, 275)
{
	std::string set_name = vm_pop_string();
	std::string script_name = vm_pop_string();
	set_objid& script_set = script_reg_objects[set_name];
	set_objid::iterator set_objid_iter;
	for (set_objid_iter=script_set.begin(); set_objid_iter!=script_set.end(); set_objid_iter++)
	{
		run_script(script_name.c_str(), *set_objid_iter);
	}
}
set_objid::iterator getnext_objid_set_iter;
set_objid* getnext_objid_set = NULL;
inline uint32_t get_next_valid_registered_object()
{
	while (true)
	{
		if (getnext_objid_set_iter == getnext_objid_set->end())
		{
			return OBJECT_INVALID;
		}
		else
		{
			cgoa_iter = cgoa.find(*getnext_objid_set_iter);
			if (cgoa_iter != cgoa.end())
			{
				return *getnext_objid_set_iter;
			}
			else
			{
				getnext_objid_set->erase(getnext_objid_set_iter++);
			}
		}
	}
	return OBJECT_INVALID;
}
VM_FUNC_NEW(GetFirstRegisteredObject, 293)
{
	std::string set_name = vm_pop_string();
	getnext_objid_set = &script_reg_objects[set_name];
	getnext_objid_set_iter = getnext_objid_set->begin();
	vm_push_object(get_next_valid_registered_object());
}
VM_FUNC_NEW(GetNextRegisteredObject, 294)
{
	uint32_t result = OBJECT_INVALID;
	if (getnext_objid_set_iter != getnext_objid_set->end())
	{
		getnext_objid_set_iter++;
		result = get_next_valid_registered_object();
	}
	vm_push_object(result);
}

timeval timer_start;
VM_FUNC_NEW(StartTimer, 146)
{
	gettimeofday(&timer_start, NULL);
}
VM_FUNC_NEW(EndTimer, 147)
{
	timeval timer_end;
	gettimeofday(&timer_end, NULL);
	timersub(&timer_end, &timer_start, &timer_end);
	char result_c_str[50];
	sprintf(result_c_str, "%ld.%06ld", timer_end.tv_sec, timer_end.tv_usec);
	CExoString result(result_c_str);
	vm_push_string(&result);
}

}
