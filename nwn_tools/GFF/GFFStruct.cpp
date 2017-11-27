#include "GFFStruct.h"

void CGFFStruct::SetStructType(uint32_t nStructType)
{
	structType = nStructType;
}

uint32_t CGFFStruct::GetStructType()
{
	return structType;
}

CGFFStruct::FIELDS_ITERATOR CGFFStruct::GetFieldsIterator()
{
	return data.begin();
}
CGFFStruct::FIELDS_ITERATOR CGFFStruct::GetFieldsEndIterator()
{
	return data.end();
}
CGFFField* CGFFStruct::GetFieldAtPos(FIELDS_ITERATOR& iterator)
{
	if (iterator == data.end()) return NULL;
	return iterator->second;
}
CGFFField* CGFFStruct::GetNextField(FIELDS_ITERATOR& iterator)
{
	if (iterator == data.end()) return NULL;
	iterator++;
	if (iterator == data.end()) return NULL;
	return iterator->second;
}

void CGFFStruct::SetField(CGFFField* lpGFFField)
{
	if (lpGFFField == NULL) return;

	CGFFField* lpNewGFFField = lpGFFField->Clone(new CGFFField());

	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(lpGFFField->label);
	if (iter != data.end())
	{
		delete iter->second;
		data.erase(iter);
	}
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::RemoveField(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		delete iter->second;
		data.erase(iter);
	}
}

void CGFFStruct::SetResRef(const char* label, const char* value)
{
	RemoveField(label);
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_CRESREF;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	if (value == NULL)
	{
		lpNewGFFField->data = new char[1];
		lpNewGFFField->data[0] = 0;
	}
	else
	{
		lpNewGFFField->data = new char[strlen(value)+1];
		strcpy(lpNewGFFField->data, value);
	}
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetString(const char* label, const char* value)
{
	RemoveField(label);
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_CEXOSTRING;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	if (value == NULL)
	{
		lpNewGFFField->data = new char[1];
		lpNewGFFField->data[0] = 0;
	}
	else
	{
		lpNewGFFField->data = new char[strlen(value)+1];
		strcpy(lpNewGFFField->data, value);
	}
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetStruct(const char* label, CGFFStruct* value)
{
	RemoveField(label);
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_STRUCT;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	if (value == NULL)
	{
		lpNewGFFField->data = (char*)(new CGFFStruct);
	}
	else
	{
		lpNewGFFField->data = (char*)value->Clone(new CGFFStruct);
	}
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetList(const char* label, CGFFList* value)
{
	RemoveField(label);
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_LIST;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	if (value == NULL)
	{
		lpNewGFFField->data = (char*)(new CGFFList);
	}
	else
	{
		lpNewGFFField->data = (char*)value->Clone(new CGFFList);
	}
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetLocString(const char* label, CGFFExoLocString* value)
{
	RemoveField(label);
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_CEXOLOCSTRING;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	if (value == NULL)
	{
		lpNewGFFField->data = (char*)(new CGFFExoLocString);
	}
	else
	{
		lpNewGFFField->data = (char*)value->Clone(new CGFFExoLocString);
	}
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetDWORD(const char* label, uint32_t value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_DWORD;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(long)value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetWORD(const char* label, uint16_t value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_WORD;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(long)value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetBYTE(const char* label, uint8_t value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_BYTE;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(long)value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetChar(const char* label, char value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_CHAR;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(long)value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;	
}

void CGFFStruct::SetFloat(const char* label, float value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_FLOAT;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(*(long*)&value);

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetDouble(const char* label, double value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_DOUBLE;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	double* new_value = new double;
	*new_value = value;
	lpNewGFFField->data = (char*)new_value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;	
}

void CGFFStruct::SetDWORD64(const char* label, uint64_t value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_DOUBLE;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	uint64_t* new_value = new uint64_t;
	*new_value = value;
	lpNewGFFField->data = (char*)new_value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;		
}

void CGFFStruct::SetINT64(const char* label, int64_t value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_DOUBLE;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	int64_t* new_value = new int64_t;
	*new_value = value;
	lpNewGFFField->data = (char*)new_value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;		
}

void CGFFStruct::SetInt(const char* label, int value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_INT;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(long)value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;
}

void CGFFStruct::SetShort(const char* label, short value)
{
	CGFFField* lpNewGFFField = new CGFFField;
	lpNewGFFField->dataType = GFF_FIELD_TYPE_SHORT;
	lpNewGFFField->label = new char[strlen(label)+1];
	strcpy(lpNewGFFField->label, label);
	lpNewGFFField->data = (char*)(long)value;

	RemoveField(label);
	data[lpNewGFFField->label] = lpNewGFFField;
}

CGFFField* CGFFStruct::GetField(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}

CGFFStruct* CGFFStruct::GetStruct(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsStruct();
	}
	return NULL;
}

CGFFList* CGFFStruct::GetList(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsList();
	}
	return NULL;
}

const char* CGFFStruct::GetResRef(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsResRef();
	}
	return NULL;
}

const char* CGFFStruct::GetString(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsString();
	}
	return NULL;
}

const char* CGFFStruct::GetLocStringStr(const char* label, int nLangId)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		CGFFExoLocString* loc_str = iter->second->AsLocString();
		if (loc_str)
		{
			return loc_str->GetString(nLangId);
		}
	}
	return NULL;
}
void CGFFStruct::SetLocStringStr(const char* label, int nLangId, const char* value)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		CGFFExoLocString* loc_str = iter->second->AsLocString();
		if (loc_str)
		{
			loc_str->SetString(nLangId, value);
		}
	}
}

CGFFExoLocString* CGFFStruct::GetLocString(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsLocString();
	}
	return NULL;
}

int CGFFStruct::GetInt(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsInt();
	}
	return 0;
}

short CGFFStruct::GetShort(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsShort();
	}
	return 0;
}

uint8_t CGFFStruct::GetBYTE(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsBYTE();
	}
	return 0;
}

char CGFFStruct::GetChar(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsChar();
	}
	return 0;	
}

uint16_t CGFFStruct::GetWORD(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsWORD();
	}
	return 0;
}

uint32_t CGFFStruct::GetDWORD(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsDWORD();
	}
	return 0;
}

float CGFFStruct::GetFloat(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsFloat();
	}
	return 0;
}

double CGFFStruct::GetDouble(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsDouble();
	}
	return 0;	
}

uint64_t CGFFStruct::GetDWORD64(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsDWORD64();
	}
	return 0;	
}

int64_t CGFFStruct::GetINT64(const char* label)
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter = data.find(label);
	if (iter != data.end())
	{
		return iter->second->AsINT64();
	}
	return 0;	
}

uint32_t CGFFStruct::GetFieldCount()
{
	return data.size();
}

CGFFStruct* CGFFStruct::Clone(CGFFStruct* lpGFFStruct)
{
	lpGFFStruct->structType = structType;

	CGFFField* lpGFFField;
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter;
	for(iter=data.begin(); iter!=data.end(); ++iter)
	{
		lpGFFField = iter->second->Clone(new CGFFField);
		lpGFFStruct->data[lpGFFField->label] = lpGFFField;
	}

	return lpGFFStruct;
}

CGFFStruct::CGFFStruct(uint32_t nStructType)
{
	this->structType = nStructType;
}

CGFFStruct::CGFFStruct()
{
	structType = 0;
}

CGFFStruct::~CGFFStruct()
{
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter;
	for(iter=data.begin(); iter!=data.end(); ++iter)
	{
		delete iter->second;
    }
}
