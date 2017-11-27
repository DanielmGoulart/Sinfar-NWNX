#include "quick_read_gff.h"
#include <sstream>

void my_fread(void * ptr, unsigned int size, unsigned int count, FILE * fp)
{
	unsigned int read_count = fread(ptr, size, count, fp);
	if (read_count != count)
	{
		if (read_count != 0 || count*size != 0)
		{
			fprintf(stderr, "error:trying to read %u blocks of %u bytes but read only %u blocks", count, size, read_count);
		}
	}
}

std::vector<char> read_file_bytes(FILE* f, uint32_t offset, uint32_t num_bytes)
{
	fseek(f, offset, 0);
	std::vector<char> buffer(num_bytes);
	my_fread(&buffer[0], num_bytes, 1, f);
	return buffer;
}

std::string read_gff_cexostring(FILE* f, uint32_t offset)
{
	fseek(f, offset, 0);
	uint32_t size;
	my_fread(&size, sizeof(size), 1, f);
	if (size > 0)
	{
		std::vector<char> buffer(size);
		my_fread(&buffer[0], size, 1, f);
		return std::string(&buffer[0], size);
	}
	else
	{
		return "";
	}
}

std::string read_gff_cexolocstring(FILE* f, uint32_t offset)
{
	fseek(f, offset+8, 0);
	uint32_t strings_count;
	my_fread(&strings_count, sizeof(strings_count), 1, f);	
	if (strings_count)
	{
		return read_gff_cexostring(f, offset+16);
	}
	else
	{
		return "";
	}
}

std::string quick_read_gff(const std::string& filename, const std::string& field_name)
{
	FILE* f = fopen(filename.c_str(), "rb");
	if (f==NULL) return "";
	const char* field_name_c_str = field_name.c_str();
	GFF_HEADER header;
	my_fread(&header, sizeof(GFF_HEADER), 1, f);
	GFF_STRUCT main_struct;
	my_fread(&main_struct, sizeof(GFF_STRUCT), 1, f);
    if (main_struct.FieldCount > 1000) return "";
	for (uint32_t struct_field_index=0; struct_field_index<main_struct.FieldCount; struct_field_index++)
	{
		fseek(f, header.FieldIndicesOffset+main_struct.DataOrDataOffset+(struct_field_index*4), 0);
		uint32_t field_index;
		my_fread(&field_index, sizeof(field_index), 1, f);
		fseek(f, header.FieldOffset+(field_index*sizeof(GFF_FIELD)), 0);
		GFF_FIELD field;
		my_fread(&field, sizeof(GFF_FIELD), 1, f); 
		fseek(f, header.LabelOffset+(field.LabelIndex*sizeof(GFF_LABEL)), 0);
		GFF_LABEL label;
		my_fread(&label, sizeof(GFF_LABEL), 1, f);
		if (strncmp(label.Name, field_name_c_str, 16)==0)
		{
			std::ostringstream result;
			GFF_RESREF resref;
			std::string resref_str;
			switch(field.Type)
			{
			case GFF_FIELD_TYPE_BYTE:
			case GFF_FIELD_TYPE_WORD:
			case GFF_FIELD_TYPE_DWORD:
				result << (uint32_t)field.DataOrDataOffset;
				break;
			case GFF_FIELD_TYPE_CHAR:
			case GFF_FIELD_TYPE_SHORT:
			case GFF_FIELD_TYPE_INT:
				result << (int32_t)field.DataOrDataOffset;
				break;
			case GFF_FIELD_TYPE_CEXOLOCSTRING:
				result << read_gff_cexolocstring(f, header.FieldDataOffset+field.DataOrDataOffset);
				break;
			case GFF_FIELD_TYPE_CEXOSTRING:
				result << read_gff_cexostring(f, header.FieldDataOffset+field.DataOrDataOffset);
				break;
			case GFF_FIELD_TYPE_CRESREF:
				fseek(f, header.FieldDataOffset+field.DataOrDataOffset, 0);
				my_fread(&resref, sizeof(resref), 1, f);
				resref_str.assign(resref.ResRef, resref.size);
				result << resref_str;
				break;
			case GFF_FIELD_TYPE_VOID:
				break;
			case GFF_FIELD_TYPE_DOUBLE:
				result << *(double*)&(read_file_bytes(f, header.FieldDataOffset+field.DataOrDataOffset, sizeof(double))[0]);
				break;
			case GFF_FIELD_TYPE_INT64:
				result << *(int64_t*)&(read_file_bytes(f, header.FieldDataOffset+field.DataOrDataOffset, sizeof(int64_t))[0]);
				break;
			case GFF_FIELD_TYPE_DWORD64:
				result << *(uint64_t*)&(read_file_bytes(f, header.FieldDataOffset+field.DataOrDataOffset, sizeof(uint64_t))[0]);
				break;
			case GFF_FIELD_TYPE_LIST:
				break;
			case GFF_FIELD_TYPE_STRUCT:
				break;
			}
			return result.str();
		}
	}
	return "";
}
