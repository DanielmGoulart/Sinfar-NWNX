#include "GFF.h"
#include <stdexcept>

CFileBuffer::CFileBuffer(uint32_t starting_size) : size(starting_size), pos(0), eof_pos(0)
{
	buffer = size ? (char*)malloc(size) : NULL;
}
CFileBuffer::CFileBuffer() : CFileBuffer(0x1000){}
void CFileBuffer::seek(size_t new_pos)
{
	if (new_pos >= size)
	{
		size = new_pos * 2;
		buffer = buffer ? (char*)realloc(buffer, size) : (char*)malloc(size);
	}
	pos = new_pos;
	if (pos > eof_pos) eof_pos = pos;
}
void CFileBuffer::write(const void* data, size_t data_len)
{
	uint32_t write_pos = pos;
	seek(pos + data_len);
	memcpy(buffer+write_pos, data, data_len);
}
char* CFileBuffer::get_data()
{
	return buffer;
}
char* CFileBuffer::steal_data()
{
	char* stolen_data = buffer;
	buffer = NULL;
	size = 0;
	eof_pos = 0;
	pos = 0;
	return stolen_data;
}
uint32_t CFileBuffer::get_data_size()
{
	return eof_pos;
}
CFileBuffer::~CFileBuffer()
{
	if (buffer) free(buffer);
}

namespace 
{
	
void my_fread(void* ptr, uint32_t size, uint32_t count, FILE* stream)
{
	uint32_t read_count = fread(ptr, size, count, stream);
	if (read_count != count)
	{
		if (read_count != 0 || count*size != 0)
		{
			char error_msg[200];
			sprintf(error_msg, __FILE__":trying to read %u blocks of %u bytes but read only %u blocks", count, size, read_count);
			throw std::logic_error(error_msg);
		}
	}
}

}

double* CGFF::ReadDouble(GFF_HEADER* lp_gff_header, uint32_t nOffset)
{
	double* d = new double;
	*d = 0;

	if (nOffset+8 > lp_gff_header->FieldDataCount)
	{
		sprintf(last_error, "The double value is corrupted: its nOffset (%u) is out of the field data count range (%u).", nOffset, lp_gff_header->FieldDataCount-8);
		return d;
	}

	char* file_data = (char*)lp_gff_header;	
	*d = *(double*)(file_data+lp_gff_header->FieldDataOffset+nOffset);
	return d;
}

CGFFExoLocString* CGFF::ReadExoLocString(GFF_HEADER* lp_gff_header, uint32_t nOffset)
{
	CGFFExoLocString* lpGFFExoLocString = new CGFFExoLocString();

	if (nOffset+12 > lp_gff_header->FieldDataCount)
	{
		sprintf(last_error, "The localized string is corrupted: its nOffset (%u) is out of the field data count range (%u).", nOffset, lp_gff_header->FieldDataCount-12);
		return lpGFFExoLocString;
	}
	
	char* file_data = (char*)lp_gff_header;
	char* exo_loc_str_ptr = file_data+lp_gff_header->FieldDataOffset+nOffset;

	lpGFFExoLocString->nStrRef = *(uint32_t*)(exo_loc_str_ptr+4);
	uint32_t nStringCount = *(uint32_t*)(exo_loc_str_ptr+8);
	
	uint32_t nStringOffset = 12;
	uint32_t nStringId;
	uint32_t nStringSize;
	uint32_t nNewStringSize;
	char* string;
	for (uint32_t nString=0; nString<nStringCount; nString++)
	{
		if (nStringOffset+8 > lp_gff_header->FieldDataCount)
		{
			sprintf(last_error, "The string inside a localized string is corrupted: its offset (%u) is out of the field data count range (%u).", nStringOffset, lp_gff_header->FieldDataCount-8);
			return lpGFFExoLocString;
		}

		nStringId = *(uint32_t*)(exo_loc_str_ptr+nStringOffset);
		nStringSize = *(uint32_t*)(exo_loc_str_ptr+nStringOffset+4);

		if (nStringOffset+8+nStringSize > lp_gff_header->FieldDataCount)
		{
			sprintf(last_error, "The string inside a localized string is corrupted: its lenght (%u) is out of the field data count range (%u).", nStringSize, lp_gff_header->FieldDataCount-nStringOffset-8);
			return lpGFFExoLocString;
		}

		if (nStringSize > 50000) nNewStringSize = 50000; else nNewStringSize = nStringSize;
		string = new char[nNewStringSize+1];
		memcpy(string, (exo_loc_str_ptr+nStringOffset+8), nNewStringSize);
		string[nNewStringSize] = 0;
		lpGFFExoLocString->data[nStringId] = string;

		nStringOffset += 8 + nStringSize;
	}

	return lpGFFExoLocString;
}

CGFFList* CGFF::ReadList(GFF_HEADER* lp_gff_header, uint32_t nOffset, uint32_t* lpStructCount)
{
	CGFFList* lpGFFList = new CGFFList();

	if (nOffset >= lp_gff_header->ListIndicesCount)
	{
		sprintf(last_error, "The file is corrupted: Too many lists (MAX %u).", lp_gff_header->ListIndicesCount);
		return lpGFFList;
	}

	char* file_data = (char*)lp_gff_header;
	uint32_t nNbStruct = *(uint32_t*)(file_data+lp_gff_header->ListIndicesOffset+nOffset);

	CGFFStruct* lpGFFStruct;
	uint32_t nStructIndex;
	for (uint32_t nStruct=0; nStruct<nNbStruct; nStruct++)
	{
		nStructIndex = *(uint32_t*)(file_data+lp_gff_header->ListIndicesOffset+nOffset+4+(nStruct*4));
		lpGFFStruct = ReadStruct(lp_gff_header, nStructIndex, lpStructCount);
		if (lpGFFStruct != NULL)
		{
			lpGFFList->data.push_back(lpGFFStruct);
		}
	}

	return lpGFFList;
}

char* CGFF::ReadLabel(GFF_HEADER* lp_gff_header, uint32_t nIndex)
{
	char* label = new char[17];

	if (nIndex >= lp_gff_header->LabelCount)
	{
		sprintf(last_error, "The label is corrupted: its index (%u) is out of the label count range (%u).", nIndex, lp_gff_header->LabelCount);
		memset(label, 0, 17);
		return label;
	}
	
	char* file_data = (char*)lp_gff_header;
	memcpy(label, file_data+lp_gff_header->LabelOffset+(nIndex*sizeof(GFF_LABEL)), 16);
	label[16] = 0;
	return label;
}

char* CGFF::ReadResRef(GFF_HEADER* lp_gff_header, uint32_t nOffset)
{
	char* resref;

	if (nOffset+1 > lp_gff_header->FieldDataCount)
	{
		sprintf(last_error, "The reserf is corrupted: its offset (%u) is out of the field data count range (%u).", nOffset, lp_gff_header->FieldDataCount-1);
		resref = new char[1];
		resref[0] = 0;
		return resref;
	}

	char* file_data = (char*)lp_gff_header;
	char* resref_ptr = file_data+lp_gff_header->FieldDataOffset+nOffset;
	uint8_t nSize = *(uint8_t*)resref_ptr;
	if (nSize > 16) nSize = 16;

	resref = new char[nSize+1];

	if (nOffset+1+nSize > lp_gff_header->FieldDataCount)
	{
		sprintf(last_error, "The resref is corrupted: its lenght (%d) is out of the field data count range (%u).", nSize, lp_gff_header->FieldDataCount-nOffset-1);
		memset(resref, 0, nSize+1);
		return resref;
	}

	memcpy(resref, resref_ptr+1, nSize);
	resref[nSize] = 0;
	return resref;
}

char* CGFF::ReadExoString(GFF_HEADER* lp_gff_header, uint32_t nOffset)
{
	char* exostring;

	if (nOffset+4 > lp_gff_header->FieldDataCount)
	{
		sprintf(last_error, "The string is corrupted: its offset (%u) is out of the field data count range (%u).", nOffset, lp_gff_header->FieldDataCount-4);
		exostring = new char[1];
		exostring[0] = 0;
		return exostring;
	}

	char* file_data = (char*)lp_gff_header;
	char* exo_str_ptr = file_data+lp_gff_header->FieldDataOffset+nOffset;

	uint32_t nSize = *(uint32_t*)exo_str_ptr;

	exostring = new char[nSize+1];

	if (nOffset+4+nSize > lp_gff_header->FieldDataCount)
	{
		sprintf(last_error, "The string is corrupted: its lenght (%u) is out of the field data count range (%u).", nSize, lp_gff_header->FieldDataCount-nOffset-4);
		memset(exostring, 0, nSize+1);
		return exostring;
	}

	memcpy(exostring, exo_str_ptr+4, nSize);
	exostring[nSize] = 0;
	return exostring;
}

CGFFField* CGFF::ReadField(GFF_HEADER* lp_gff_header, uint32_t nIndex, uint32_t* lpStructCount)
{
	CGFFField* lpGFFField = new CGFFField();

	if (nIndex >= lp_gff_header->FieldCount)
	{
		sprintf(last_error, "The file is corrupted: The index of the field (%u) is out of the field count range (%u).", nIndex, lp_gff_header->FieldCount);
		return lpGFFField;
	}

	char* file_data = (char*)lp_gff_header;
	GFF_FIELD* gff_field = (GFF_FIELD*)(file_data+lp_gff_header->FieldOffset+(nIndex*sizeof(GFF_FIELD)));

	lpGFFField->label = ReadLabel(lp_gff_header, gff_field->LabelIndex);
	lpGFFField->dataType = gff_field->Type;
	if (gff_field->Type == GFF_FIELD_TYPE_LIST)
	{
		lpGFFField->data = (char*)ReadList(lp_gff_header, gff_field->DataOrDataOffset, lpStructCount);
	}
	else if (gff_field->Type == GFF_FIELD_TYPE_CEXOSTRING)
	{
		lpGFFField->data = ReadExoString(lp_gff_header, gff_field->DataOrDataOffset);
	}
	else if (gff_field->Type == GFF_FIELD_TYPE_CRESREF)
	{
		lpGFFField->data = ReadResRef(lp_gff_header, gff_field->DataOrDataOffset);
	}
	else if (gff_field->Type == GFF_FIELD_TYPE_CEXOLOCSTRING)
	{
		lpGFFField->data = (char*)ReadExoLocString(lp_gff_header, gff_field->DataOrDataOffset);
	}
	else if (gff_field->Type == GFF_FIELD_TYPE_STRUCT)
	{
		lpGFFField->data = (char*)ReadStruct(lp_gff_header, gff_field->DataOrDataOffset, lpStructCount);
	}
	else if (gff_field->Type == GFF_FIELD_TYPE_DOUBLE ||
		gff_field->Type == GFF_FIELD_TYPE_INT64 ||
		gff_field->Type == GFF_FIELD_TYPE_DWORD64)
	{
		lpGFFField->data = (char*)ReadDouble(lp_gff_header, gff_field->DataOrDataOffset);
	}
	else
	{
		lpGFFField->data = *(char**)&gff_field->DataOrDataOffset;
	}
	return lpGFFField;
}

CGFFStruct* CGFF::ReadStruct(GFF_HEADER* lp_gff_header, uint32_t nIndex, uint32_t* lpStructCount)
{
	CGFFStruct* lpGFFStruct = new CGFFStruct();

	if (nIndex >= lp_gff_header->StructCount) return lpGFFStruct;

	if (*lpStructCount >= lp_gff_header->StructCount)
	{
		sprintf(last_error, "The file is corrupted: Too many structs (MAX %u).", lp_gff_header->StructCount);
		return lpGFFStruct;
	}
	(*lpStructCount)++;

	char* file_data = (char*)lp_gff_header;
	GFF_STRUCT* gff_struct = (GFF_STRUCT*)(file_data+lp_gff_header->StructOffset+(nIndex*sizeof(GFF_STRUCT)));

	lpGFFStruct->structType = gff_struct->Type;
	CGFFField* lpGFFField;
	if (gff_struct->FieldCount == 1)
	{
		lpGFFField = ReadField(lp_gff_header, gff_struct->DataOrDataOffset, lpStructCount);
		if (lpGFFField != NULL)
		{
			lpGFFStruct->data[lpGFFField->label] = lpGFFField;
		}
	}
	else if (gff_struct->FieldCount > 1)
	{
		if (gff_struct->DataOrDataOffset+(4*gff_struct->FieldCount) > lp_gff_header->FieldIndicesCount) return lpGFFStruct;

		uint32_t nFieldIndex;
		for (uint32_t nField=0; nField<gff_struct->FieldCount; nField++)
		{
			nFieldIndex = *(uint32_t*)(file_data+lp_gff_header->FieldIndicesOffset+gff_struct->DataOrDataOffset+(4*nField));
			lpGFFField = ReadField(lp_gff_header, nFieldIndex, lpStructCount);
			if (lpGFFField != NULL)
			{
				lpGFFStruct->data[lpGFFField->label] = lpGFFField;
			}
		}
	}

	return lpGFFStruct;
}

uint32_t CGFF::WriteDouble(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, double* lpDouble)
{
	uint32_t nFieldDataIndex = lp_gff_header->FieldDataCount;
	lp_gff_header -> FieldDataCount += 8;

	file_buffer.seek(lp_gff_header->FieldDataOffset+nFieldDataIndex);
	file_buffer.write(lpDouble, 8);

	return nFieldDataIndex;
}

uint32_t CGFF::WriteResRef(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, char* resref)
{
	uint32_t nFieldDataIndex = lp_gff_header->FieldDataCount;
	uint8_t nResRefSize = strlen(resref);
	lp_gff_header->FieldDataCount += 1 + nResRefSize;

	file_buffer.seek(lp_gff_header->FieldDataOffset+nFieldDataIndex);
	file_buffer.write(&nResRefSize, 1);
	file_buffer.write(resref, nResRefSize);

	return nFieldDataIndex;
}

uint32_t CGFF::WriteExoLocString(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFExoLocString* lpGFFExoLocString)
{
	uint32_t nFieldDataIndex = lp_gff_header->FieldDataCount;
	uint32_t nTotalSize = 8;
	int nStringSize;
	file_buffer.seek(lp_gff_header->FieldDataOffset+nFieldDataIndex+12);
	std::map<int, char*>::iterator iter;
	for(iter=lpGFFExoLocString->data.begin(); iter!=lpGFFExoLocString->data.end(); ++iter)
	{
		nStringSize = strlen(iter->second);
		nTotalSize += 8 + nStringSize;
		file_buffer.write(&(iter->first), 4);
		file_buffer.write(&nStringSize, 4);
		file_buffer.write(iter->second, nStringSize);
	}
	lp_gff_header->FieldDataCount += nTotalSize + 4;

	file_buffer.seek(lp_gff_header->FieldDataOffset+nFieldDataIndex);
	file_buffer.write(&nTotalSize, 4);
	file_buffer.write(&(lpGFFExoLocString->nStrRef), 4);
	uint32_t nStringCount = lpGFFExoLocString->data.size();
	file_buffer.write(&nStringCount, 4);

	return nFieldDataIndex;
}

uint32_t CGFF::WriteExoString(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, char* string)
{
	uint32_t nFieldDataIndex = lp_gff_header->FieldDataCount;
	uint32_t nStringSize = strlen(string);
	lp_gff_header->FieldDataCount += 4 + nStringSize;

	file_buffer.seek(lp_gff_header->FieldDataOffset+nFieldDataIndex);
	file_buffer.write(&nStringSize, 4);
	file_buffer.write(string, nStringSize);

	return nFieldDataIndex;
}

uint32_t CGFF::WriteList(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFList* lpGFFList, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map)
{
	uint32_t nListIndex = lp_gff_header->ListIndicesCount;
	uint32_t nListSize = lpGFFList->data.size();
	lp_gff_header->ListIndicesCount += 4 + 4*nListSize;
	file_buffer.seek(lp_gff_header->ListIndicesOffset+nListIndex);
	file_buffer.write(&nListSize, 4);

	uint32_t nStructIndex;
	uint32_t nListIndiceIndex = 0;
	std::vector<CGFFStruct*>::iterator iter;
	for(iter=lpGFFList->data.begin(); iter!=lpGFFList->data.end(); ++iter)
	{
		nStructIndex = WriteStruct(file_buffer, lp_gff_header, *iter, lp_labels_map);
		file_buffer.seek(lp_gff_header->ListIndicesOffset+nListIndex+4+4*nListIndiceIndex);
		file_buffer.write(&nStructIndex, 4);

		nListIndiceIndex++;
	}

	return nListIndex;
}

uint32_t CGFF::WriteField(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFField* lpGFFField, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map)
{
	GFF_FIELD gff_field;

	uint32_t nFieldIndex = lp_gff_header->FieldCount;
	lp_gff_header->FieldCount++;

	gff_field.LabelIndex = (*lp_labels_map)[lpGFFField->label]-1;
	gff_field.Type = lpGFFField->dataType;

	switch (gff_field.Type)
	{
	case GFF_FIELD_TYPE_STRUCT:
		gff_field.DataOrDataOffset = WriteStruct(file_buffer, lp_gff_header, (CGFFStruct*)lpGFFField->data, lp_labels_map);
		break;
	case GFF_FIELD_TYPE_LIST:
		gff_field.DataOrDataOffset = WriteList(file_buffer, lp_gff_header, (CGFFList*)lpGFFField->data, lp_labels_map);
		break;
	case GFF_FIELD_TYPE_CEXOSTRING:
		gff_field.DataOrDataOffset = WriteExoString(file_buffer, lp_gff_header, lpGFFField->data);
		break;
	case GFF_FIELD_TYPE_DOUBLE:
	case GFF_FIELD_TYPE_DWORD64:
	case GFF_FIELD_TYPE_INT64:
		gff_field.DataOrDataOffset = WriteDouble(file_buffer, lp_gff_header, (double*)lpGFFField->data);
		break;
	case GFF_FIELD_TYPE_CRESREF:
		gff_field.DataOrDataOffset = WriteResRef(file_buffer, lp_gff_header, (char*)lpGFFField->data);
		break;
	case GFF_FIELD_TYPE_CEXOLOCSTRING:
		gff_field.DataOrDataOffset = WriteExoLocString(file_buffer, lp_gff_header, (CGFFExoLocString*)lpGFFField->data);
		break;
	case GFF_FIELD_TYPE_VOID:
		gff_field.DataOrDataOffset = 0; //TODO
	default:
		gff_field.DataOrDataOffset = *(int*)&(lpGFFField->data);
	}

	file_buffer.seek(lp_gff_header->FieldOffset+nFieldIndex*sizeof(GFF_FIELD));
	file_buffer.write(&gff_field, sizeof(GFF_FIELD));

	return nFieldIndex;
}

uint32_t CGFF::WriteStruct(CFileBuffer& file_buffer, GFF_HEADER* lp_gff_header, CGFFStruct* lpGFFStruct, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map)
{
	uint32_t nStructIndex = lp_gff_header->StructCount;
	lp_gff_header->StructCount++;

	GFF_STRUCT gff_struct;
	gff_struct.FieldCount = lpGFFStruct->data.size();
	gff_struct.Type = lpGFFStruct->structType;
	if (gff_struct.FieldCount == 1)
	{
		gff_struct.DataOrDataOffset = WriteField(file_buffer, lp_gff_header, lpGFFStruct->data.begin()->second, lp_labels_map);
	}
	else if (gff_struct.FieldCount > 1)
	{
		uint32_t nFieldIndicesIndex = lp_gff_header->FieldIndicesCount;
		lp_gff_header->FieldIndicesCount += 4*gff_struct.FieldCount;
		gff_struct.DataOrDataOffset = nFieldIndicesIndex;
		uint32_t nFieldIndex;
		uint32_t nFieldIndiceCount = 0;
		std::map<const char*, CGFFField*, map_strcmp>::iterator iter;
		for(iter=lpGFFStruct->data.begin(); iter!=lpGFFStruct->data.end(); ++iter)
		{
			nFieldIndex = WriteField(file_buffer, lp_gff_header, iter->second, lp_labels_map);
			file_buffer.seek(lp_gff_header->FieldIndicesOffset+nFieldIndicesIndex+nFieldIndiceCount*4);
			file_buffer.write(&nFieldIndex, 4);
			nFieldIndiceCount++;
		}
	}

	file_buffer.seek(lp_gff_header->StructOffset+nStructIndex*sizeof(GFF_STRUCT));
	file_buffer.write(&gff_struct, sizeof(GFF_STRUCT));

	return nStructIndex;
}

void CGFF::GetListDataCount(GFF_HEADER* lp_gff_header, CGFFList* lpGFFList, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map)
{
	std::vector<CGFFStruct*>::iterator iter;
	for (iter=lpGFFList->data.begin(); iter!=lpGFFList->data.end(); ++iter)
	{
		lp_gff_header->StructCount++;
		GetStructDataCount(lp_gff_header, *iter, lp_labels_map);
	}
}

uint32_t CGFF::GetCExoLocStringTotalSize(CGFFExoLocString* lpGFFExoLocString)
{
	uint32_t nSize = 12;
	std::map<int, char*>::iterator iter;
	for (iter=lpGFFExoLocString->data.begin(); iter!=lpGFFExoLocString->data.end(); ++iter)
	{
		nSize += 8 + strlen(iter->second);
	}
	return nSize;
}

void CGFF::GetStructDataCount(GFF_HEADER* lp_gff_header, CGFFStruct* lpGFFStruct, std::map<const char*, uint32_t, map_strcmp>* lp_labels_map)
{
	if (lpGFFStruct->data.size() > 1)
	{
		lp_gff_header->FieldIndicesCount += 4*lpGFFStruct->data.size();
	}
	std::map<const char*, CGFFField*, map_strcmp>::iterator iter;
	for(iter=lpGFFStruct->data.begin(); iter!=lpGFFStruct->data.end(); ++iter)
	{
		if (!(*lp_labels_map)[iter->first])
		{
			lp_gff_header->LabelCount++;
			(*lp_labels_map)[iter->first] = lp_gff_header->LabelCount;
		}
		lp_gff_header->FieldCount++;
		switch (iter->second->dataType)
		{
		case GFF_FIELD_TYPE_STRUCT:
			lp_gff_header->StructCount++;
			GetStructDataCount(lp_gff_header, (CGFFStruct*)iter->second->data, lp_labels_map);
			break;
		case GFF_FIELD_TYPE_LIST:
			lp_gff_header->ListIndicesCount += 4 + (((CGFFList*)iter->second->data)->data.size() * 4);
			GetListDataCount(lp_gff_header, (CGFFList*)iter->second->data, lp_labels_map);
			break;
		case GFF_FIELD_TYPE_CEXOSTRING:
			lp_gff_header->FieldDataCount += 4 + strlen(iter->second->data);
			break;
		case GFF_FIELD_TYPE_DOUBLE:
		case GFF_FIELD_TYPE_DWORD64:
		case GFF_FIELD_TYPE_INT64:
			lp_gff_header->FieldDataCount += 8;
			break;
		case GFF_FIELD_TYPE_CRESREF:
			lp_gff_header->FieldDataCount += 1 + strlen(iter->second->data);
			break;
		case GFF_FIELD_TYPE_CEXOLOCSTRING:
			lp_gff_header->FieldDataCount += GetCExoLocStringTotalSize((CGFFExoLocString*)iter->second->data);
			break;
		}
    }
}

void CGFF::ToFileBuffer(CFileBuffer& file_buffer)
{
	if (mainStruct == NULL) return;

	GFF_HEADER gff_header;

	strncpy((char*)&(gff_header.FileVersion), (char*)&(fileVersion), 4);
	strncpy((char*)&(gff_header.FileType), (char*)&(fileType), 4);

	gff_header.StructCount = 1;
	gff_header.FieldCount = 0;
	gff_header.LabelCount = 0;
	gff_header.FieldDataCount = 0;
	gff_header.FieldIndicesCount = 0;
	gff_header.ListIndicesCount = 0;

	std::map<const char*, uint32_t, map_strcmp> labels_map;

	GetStructDataCount(&gff_header, mainStruct, &labels_map);

	gff_header.StructOffset = sizeof(GFF_HEADER);
	gff_header.FieldOffset = gff_header.StructOffset + (gff_header.StructCount*sizeof(GFF_STRUCT));
	gff_header.LabelOffset = gff_header.FieldOffset + (gff_header.FieldCount*sizeof(GFF_FIELD));
	gff_header.FieldDataOffset = gff_header.LabelOffset + (gff_header.LabelCount*sizeof(GFF_LABEL));
	gff_header.FieldIndicesOffset = gff_header.FieldDataOffset + gff_header.FieldDataCount;
	gff_header.ListIndicesOffset = gff_header.FieldIndicesOffset + gff_header.FieldIndicesCount;

	gff_header.StructCount = 0;
	gff_header.FieldCount = 0;
	gff_header.FieldDataCount = 0;
	gff_header.FieldIndicesCount = 0;
	gff_header.ListIndicesCount = 0;
	
	GFF_LABEL gff_label;
	std::map<const char*, uint32_t, map_strcmp>::iterator iter;
	for (iter=labels_map.begin(); iter!=labels_map.end(); ++iter)
	{
		strncpy((char*)&gff_label, iter->first, 16);
		file_buffer.seek(gff_header.LabelOffset+(iter->second-1)*sizeof(GFF_LABEL));
		file_buffer.write(&gff_label, sizeof(GFF_LABEL));
	}
	WriteStruct(file_buffer, &gff_header, mainStruct, &labels_map);
	file_buffer.seek(0);
	file_buffer.write(&gff_header, sizeof(GFF_HEADER));	
}

void CGFF::SaveToFile(const char* filename)
{
	CFileBuffer file_buffer;
	
	ToFileBuffer(file_buffer);
	
	FILE* f = fopen(filename, "wb");
	fwrite(file_buffer.get_data(), file_buffer.get_data_size(), 1, f);
	fclose(f);

	return;
}

bool CGFF::GetIsGFFHeaderValid(GFF_HEADER* lp_gff_header, uint32_t file_size)
{
	uint32_t nFilePos = lp_gff_header->StructOffset;
	if (nFilePos != sizeof(GFF_HEADER)) return false;
	nFilePos += lp_gff_header->StructCount*sizeof(GFF_STRUCT);
	if (nFilePos != lp_gff_header->FieldOffset) return false;
	nFilePos += lp_gff_header->FieldCount*sizeof(GFF_FIELD);
	if (nFilePos != lp_gff_header->LabelOffset) return false;
	nFilePos += lp_gff_header->LabelCount*sizeof(GFF_LABEL);
	if (nFilePos != lp_gff_header->FieldDataOffset) return false;
	nFilePos += lp_gff_header->FieldDataCount;
	if (nFilePos !=  lp_gff_header->FieldIndicesOffset) return false;
	nFilePos += lp_gff_header->FieldIndicesCount;
	if (nFilePos != lp_gff_header->ListIndicesOffset) return false;
	nFilePos += lp_gff_header->ListIndicesCount;
	if (nFilePos != file_size) return false;

	return true;
}

void CGFF::init(const char* file_data, uint32_t file_size, bool bIgnoreMinorErros)
{
	GFF_HEADER* gff_header = (GFF_HEADER*)file_data;
	if (bIgnoreMinorErros || GetIsGFFHeaderValid(gff_header, file_size))
	{
		strncpy((char*)&(fileVersion), (char*)&(gff_header->FileVersion), 4);
		strncpy((char*)&(fileType), (char*)&(gff_header->FileType), 4);
		uint32_t nStructCount=0;
		mainStruct = ReadStruct(gff_header, 0, &nStructCount);
	}
	else
	{
		sprintf(last_error, "The GFF header is invalid.");
	}		
}

CGFF::CGFF(const char* file_data, uint32_t file_size, bool bIgnoreMinorErros=false)
{
	init(file_data, file_size, bIgnoreMinorErros);
}

CGFF::CGFF(const char* filename, bool bIgnoreMinorErros)
{
	mainStruct = NULL;
	FILE* f = fopen(filename, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		uint32_t file_size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* file_data = new char[file_size];
		my_fread(file_data, file_size, 1, f);
		if (!ferror(f))
		{
			init(file_data, file_size, bIgnoreMinorErros);
		}
		else
		{
			sprintf(last_error, "An error occurred while attempting to read: \"%s\".", filename);
		}
		delete[] file_data;
		fclose(f);
	}
	else
	{
		sprintf(last_error, "The file: \"%s\" couln't be open.", filename);
	}
	if (mainStruct == NULL) mainStruct = new CGFFStruct((uint32_t)-1);
}

CGFF::CGFF(const char* fileType, const char* fileVersion)
{
	strcpy(last_error, "no error");
	mainStruct = new CGFFStruct((uint32_t)-1);
	strncpy(this->fileType, fileType, 4);
	strncpy(this->fileVersion, fileVersion, 4);
	return;
}

CGFF::CGFF(const char* fileType, const char* fileVersion, CGFFStruct* mainStruct)
{
	strcpy(last_error, "no error");
	this->mainStruct = mainStruct->Clone(new CGFFStruct);
	this->mainStruct->structType = (uint32_t)-1;
	strncpy(this->fileType, fileType, 4);
	strncpy(this->fileVersion, fileVersion, 4);
	return;
}

CGFF::CGFF(CGFF* copyme)
{
	strcpy(last_error, "no error");
	mainStruct = copyme->mainStruct->Clone(new CGFFStruct);
	strncpy(this->fileType, copyme->fileType, 4);
	strncpy(this->fileVersion, copyme->fileVersion, 4);
}

CGFF::~CGFF(void)
{
	if (mainStruct) delete mainStruct;
}

CGFFStruct* CGFF::GetMainStruct()
{
	return mainStruct;
}

std::string CGFF::GetFileType()
{
	return std::string(fileType, 4);
}

std::string CGFF::GetFileVersion()
{
	return std::string(fileVersion, 4);	
}

void CGFF::SetMainStruct(CGFFStruct* main_struct)
{
	if (mainStruct) delete mainStruct;
	this->mainStruct = main_struct->Clone(new CGFFStruct);
	this->mainStruct->structType = (uint32_t)-1;
}

char* CGFF::GetLastErrorMessage()
{
	return last_error;
}
