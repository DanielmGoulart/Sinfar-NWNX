#include "CERF.h"
#include <algorithm>

using namespace aurora;

CERF::CERF(const std::string& filepath) : filepath(filepath)
{
	CFile erf_file(fopen(filepath.c_str(), "rb"));
	if (!erf_file)
	{
		log_printf("Failed to open ERF:%s\n", filepath.c_str());
		return;
	}
	size_t read_count = fread(&header, sizeof(header), 1, erf_file);
	fseek(erf_file, 0, SEEK_END);
	long erf_file_size = ftell(erf_file);
	if (read_count != 1 || 
		header.EntryCount == 0 || header.EntryCount > 10*1000*1000 ||
		header.OffsetToLocalizedString != sizeof(header) ||
		header.OffsetToKeyList != sizeof(header)+header.LocalizedStringSize ||
		header.OffsetToResourceList < sizeof(header)+header.LocalizedStringSize+sizeof(ERF_KEY)*header.EntryCount)
	{
		log_printf("Invalid ERF:%s\n", filepath.c_str());
		return;	
	}
	std::vector<ERF_KEY> erf_keys(header.EntryCount);
	fseek(erf_file, header.OffsetToKeyList, SEEK_SET);
	fread(&erf_keys.at(0), sizeof(ERF_KEY)*header.EntryCount, 1, erf_file);
	std::vector<ERF_RESOURCE> erf_resources_data(header.EntryCount);
	fseek(erf_file, header.OffsetToResourceList, SEEK_SET);
	fread(&erf_resources_data.at(0), sizeof(ERF_RESOURCE)*header.EntryCount, 1, erf_file);
	long expected_pos = ftell(erf_file);
	for (uint32_t i = 0; i < header.EntryCount; i++)
	{
		auto erf_resource = erf_resources_data.at(i);
		if (erf_resource.OffsetToResource != expected_pos)
		{
			log_printf("Invalid ERF resource:%d(%s)\n", i, filepath.c_str());
			return;
		}
		expected_pos += erf_resource.ResourceSize;
	}
	for (uint32_t i = 0; i < header.EntryCount; i++)
	{
		ERF_KEY& erf_key = erf_keys.at(i);
		normalize_resref(erf_key.ResRef);
		std::unique_ptr<CERFResource> erf_res(new CERFResource(*this, erf_resources_data.at(i)));
		this->resources[CResource(static_cast<NwnResType>(erf_key.ResType), erf_key.ResRef)] = std::move(erf_res);
	}
}

void CERFResource::ReadData(char* buffer)
{
	CFile erf_file(fopen(erf.filepath.c_str(), "rb"));
	if (!erf_file) return;
	fseek(erf_file, res_info.OffsetToResource, SEEK_SET);
	fread(buffer, res_info.ResourceSize, 1, erf_file);
}