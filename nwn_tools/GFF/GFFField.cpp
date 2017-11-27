#include "GFFField.h"
#include <math.h>

CGFFField::CGFFField(void)
{
	label = NULL;
	data = NULL;
	dataType = 0;
}

CGFFField::CGFFField(const char* label)
{
	data = NULL;
	dataType = 0;
	this->label = new char[strlen(label)+1];
	strcpy(this->label, label);
}

CGFFField::CGFFField(const char* label, CGFFList* list)
{
	this->label = new char[strlen(label)+1];
	strcpy(this->label, label);
	this->data = (char*)list->Clone(new CGFFList);
	this->dataType = GFF_FIELD_TYPE_LIST;
}

void CGFFField::DeleteData()
{
	switch (dataType)
	{
	case GFF_FIELD_TYPE_CEXOLOCSTRING:
		delete (CGFFExoLocString*)data;
		break;
	case GFF_FIELD_TYPE_CEXOSTRING:
	case GFF_FIELD_TYPE_CRESREF:
	//case GFF_FIELD_TYPE_VOID: (TODO)
		delete[] data;
		break;
	case GFF_FIELD_TYPE_DOUBLE:
	case GFF_FIELD_TYPE_INT64:
	case GFF_FIELD_TYPE_DWORD64:
		delete (double*)data;
		break;
	case GFF_FIELD_TYPE_LIST:
		delete (CGFFList*)data;
		break;
	case GFF_FIELD_TYPE_STRUCT:
		delete (CGFFStruct*)data;
		break;
	}
}

CGFFField::~CGFFField(void)
{
	delete[] label;
	DeleteData();
}

CGFFField* CGFFField::Clone(CGFFField* lpGFFField)
{
	lpGFFField->dataType = dataType;
	lpGFFField->label = new char[strlen(label)+1];
	strcpy(lpGFFField->label, label);
	switch (dataType)
	{
	case GFF_FIELD_TYPE_CEXOLOCSTRING:
		lpGFFField->data = (char*)((CGFFExoLocString*)data)->Clone(new CGFFExoLocString());
		break;
	case GFF_FIELD_TYPE_CEXOSTRING:
	case GFF_FIELD_TYPE_CRESREF:
		lpGFFField->data = new char[strlen(data)+1];
		strcpy(lpGFFField->data, data);
		break;
	case GFF_FIELD_TYPE_DOUBLE:
	case GFF_FIELD_TYPE_INT64:
	case GFF_FIELD_TYPE_DWORD64:
		lpGFFField->data = (char*)new double;
		*(double*)lpGFFField->data = *(double*)data;
		break;
	case GFF_FIELD_TYPE_LIST:
		lpGFFField->data = (char*)((CGFFList*)data)->Clone(new CGFFList());
		break;
	case GFF_FIELD_TYPE_STRUCT:
		lpGFFField->data = (char*)((CGFFStruct*)data)->Clone(new CGFFStruct());
		break;
	default:
		lpGFFField->data = data;
	}
	return lpGFFField;
}

void CGFFField::SetResRef(const char* value)
{
	DeleteData();
	dataType = GFF_FIELD_TYPE_CRESREF;
	data = new char[strlen(value)+1];
	strcpy(data, value);
}

const char* CGFFField::GetLabel()
{
	return label;
}

uint32_t CGFFField::GetDataType()
{
	return dataType;
}

CGFFStruct* CGFFField::AsStruct()
{
	if (dataType ==  GFF_FIELD_TYPE_STRUCT)
	{
		return (CGFFStruct*)data;
	}
	return NULL;
}
CGFFList* CGFFField::AsList()
{
	if (dataType ==  GFF_FIELD_TYPE_LIST)
	{
		return (CGFFList*)data;
	}
	return NULL;
}
const char* CGFFField::AsResRef()
{
	if (dataType ==  GFF_FIELD_TYPE_CRESREF)
	{
		return data;
	}
	return NULL;
}
const char* CGFFField::AsString()
{
	if (dataType ==  GFF_FIELD_TYPE_CEXOSTRING)
	{
		return data;
	}
	return NULL;
}
CGFFExoLocString* CGFFField::AsLocString()
{
	if (dataType ==  GFF_FIELD_TYPE_CEXOLOCSTRING)
	{
		return (CGFFExoLocString*)data;
	}
	return NULL;
}
int CGFFField::AsInt()
{
	if (dataType ==  GFF_FIELD_TYPE_INT)
	{
		return *(int*)&(data);
	}
	return 0;
}
short CGFFField::AsShort()
{
	if (dataType ==  GFF_FIELD_TYPE_SHORT)
	{
		return *(short*)&(data);
	}
	return 0.0;
}
char CGFFField::AsChar()
{
	if (dataType ==  GFF_FIELD_TYPE_CHAR)
	{
		return *(char*)&(data);
	}
	return 0;
}
uint8_t CGFFField::AsBYTE()
{
	if (dataType ==  GFF_FIELD_TYPE_BYTE)
	{
		return *(uint8_t*)&(data);
	}
	return 0;
}
uint16_t CGFFField::AsWORD()
{
	if (dataType == GFF_FIELD_TYPE_WORD)
	{
		return *(uint16_t*)&(data);
	}
	return 0;
}
uint32_t CGFFField::AsDWORD()
{
	if (dataType == GFF_FIELD_TYPE_DWORD)
	{
		return *(uint32_t*)&(data);
	}
	return 0;
}
float CGFFField::AsFloat()
{
	if (dataType == GFF_FIELD_TYPE_FLOAT)
	{
		return *(float*)&(data);
	}
	return 0.0;
}
namespace
{
    inline double round_to_digits(double value, int digits)
    {
        if (value == 0.0) return 0.0;
        double factor = pow(10.0, digits - ceil(log10(fabs(value))));
        return round(value * factor) / factor;   
    }
}
double CGFFField::AsSignificantFloat()
{
    return round_to_digits(AsFloat(), 6);
}
double CGFFField::AsDouble()
{
	if (dataType == GFF_FIELD_TYPE_DOUBLE)
	{
		return *(double*)data;
	}
	return 0.0;
}
uint64_t CGFFField::AsDWORD64()
{
	if (dataType == GFF_FIELD_TYPE_DWORD64)
	{
		return *(uint64_t*)data;
	}
	return 0;
}
int64_t CGFFField::AsINT64()
{
	if (dataType == GFF_FIELD_TYPE_INT64)
	{
		return *(int64_t*)data;
	}
	return 0;
}

