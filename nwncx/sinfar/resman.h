#pragma once

#include "../../nwn_tools/CKey/CKey.h"
#include "../../nwn_tools/CERF/CERF.h"
#include "../nwncx.h"
#include <deque>

namespace nwncx
{
namespace sinfar
{
	bool file_exists(const std::string& file_path);

	extern std::deque<std::string> last_demand_res_filename;

	struct RESOURCE_ENTRY
	{
		RESOURCE_ENTRY(const std::string& filename) : 
			filename(filename),
			to_be_deleted_priority(0),
			to_be_deleted_alloc_size(0)
		{
		}
		std::string filename;
		uint32_t to_be_deleted_priority;
		uint32_t to_be_deleted_alloc_size;
	};
	enum RESOURCE_TYPE
	{
		DYNAMIC_DIRECTORY = 0,
		ENCAPSULATED = 1,
		DIRECTORY = 2,
		KEY_TABLE = 3
	};

#pragma pack(push, 1)
	struct TGA_HEADER
	{
		uint8_t id;
		uint8_t map_type;
		uint8_t img_type;
		uint16_t map_index;
		uint16_t map_length;
		uint8_t map_entry_size;
		uint16_t origin_x;
		uint16_t origin_y;
		uint16_t width;
		uint16_t height;
		uint8_t pixels_depth;
		uint8_t img_descriptor;
		TGA_HEADER(uint16_t width, uint16_t height) :
			id(0), 
			map_type(0), 
			img_type(2),
			map_index(0),
			map_length(0),
			map_entry_size(0),
			origin_x(0),
			origin_y(0),
			width(width),
			height(height),
			pixels_depth(32),
			img_descriptor(40)
		{
		}
	};
	struct DDS_DATA
	{
		uint32_t width;
		uint32_t height;
		uint32_t color_channels;
		uint8_t unk_0xC[8];
		uint8_t data[1];
	};
#pragma pack(pop)

	extern unordered_map<std::string, CRes*> resources;
	extern unordered_map<std::string, std::unique_ptr<aurora::CKey>> all_fixed_key_tables;
	extern unordered_map<std::string, std::unique_ptr<aurora::CERF>> all_encapsulated_resources_files;
	extern unordered_map<std::string, std::unique_ptr<aurora::CResourcesDirectory>> all_directory_resources;
	extern unordered_map<std::string, std::unique_ptr<aurora::CResourcesDirectory>> all_dynamic_directory_resources;
	extern unordered_map<std::string, std::map<RESOURCE_TYPE, std::deque<aurora::CResourceData*>>> best_resources;
	extern uint32_t resman_total_alloc;
	extern uint32_t resman_last_demand_id;
	extern unordered_map<uint32_t, CRes*> resman_to_be_deleted_map;

	void export_all_best_resources(const std::string& dest);

#ifdef WIN32
	extern CExoString*(NWNCX_MEMBER_CALL *resolve_filename_org)(void*, NWNCX_MEMBER_X_PARAM CExoString*, CExoString*, uint16_t);
#endif
	std::string resolve_filename(const std::string& filename);

	bool service_resource(CRes* res);
	void free_res_data(CRes* res);
	void alloc_res_data(CRes* res);
	void update_resource_data(const std::string& res_filename);
	aurora::CResourceData* get_resource_data_from_filename(const std::string res_filename);
	void remove_resources_container_from_best_resources(aurora::CResourcesContainer* resources_container, RESOURCE_TYPE res_type);
	void add_resources_container_to_best_resources(aurora::CResourcesContainer* resources_container, RESOURCE_TYPE res_type);

	int add_resource_directory(const std::string& dir_path, RESOURCE_TYPE res_type, bool do_resolve_filename=true);
	void remove_resource_directory(const std::string& dir_path, RESOURCE_TYPE res_type);

	extern int(NWNCX_MEMBER_CALL *add_encapsulated_resource_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL add_encapsulated_resource_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *add_resource_image_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*, uint8_t*);
	int NWNCX_MEMBER_CALL add_resource_image_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename, uint8_t* p2);

	int add_single_fixed_key_table(const std::string& filename);
	extern int(NWNCX_MEMBER_CALL *add_fixed_key_table_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL add_fixed_key_table_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *add_resource_directory_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL add_resource_directory_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *remove_encapsulated_resource_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL remove_encapsulated_resource_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *remove_resource_image_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL remove_resource_image_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *remove_fixed_key_table_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL remove_fixed_key_table_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *remove_resource_directory_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL remove_resource_directory_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern int(NWNCX_MEMBER_CALL *update_resource_directory_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*);
	int NWNCX_MEMBER_CALL update_resource_directory_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename);

	extern CExoStringList*(NWNCX_MEMBER_CALL *get_res_of_type_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM uint16_t, CRes*);
	CExoStringList* NWNCX_MEMBER_CALL get_res_of_type_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM uint16_t type, CRes* res);

	extern void(NWNCX_MEMBER_CALL *destroy_res_org)(CRes* NWNCX_DESTRUCTOR_P2);
	void NWNCX_MEMBER_CALL destroy_res_hook(CRes* res NWNCX_DESTRUCTOR_P2);

	int resource_exists(CResRef& resref, uint16_t res_type);

	extern int(NWNCX_MEMBER_CALL *resource_exists_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t, uint32_t*);
	int NWNCX_MEMBER_CALL resource_exists_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CResRef* resref, uint16_t type, uint32_t* table_type);

	extern uint32_t(NWNCX_MEMBER_CALL *get_table_count_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*, int);
	uint32_t NWNCX_MEMBER_CALL get_table_count_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res, int p2);

	extern int(NWNCX_MEMBER_CALL *release_res_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*);
	int NWNCX_MEMBER_CALL release_res_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res);

	int NWNCX_MEMBER_CALL on_release_res_before_destroying_it(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res);
	extern int(NWNCX_MEMBER_CALL *release_res_object_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*);
	int NWNCX_MEMBER_CALL release_res_object_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res);

	extern int(NWNCX_MEMBER_CALL *free_res_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*);
	int NWNCX_MEMBER_CALL free_res_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res);

	extern void(NWNCX_MEMBER_CALL *free_res_data_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*);
	void NWNCX_MEMBER_CALL free_res_data_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res);

	extern int(NWNCX_MEMBER_CALL *free_chunk_org)(CExoResMan*);
	int NWNCX_MEMBER_CALL free_chunk_hook(CExoResMan* resman);

	extern int(NWNCX_MEMBER_CALL *request_res_org)(CRes*);
	int NWNCX_MEMBER_CALL request_res_hook(CRes* res);

	extern int(NWNCX_MEMBER_CALL *cancel_request_res_org)(CRes*);
	int NWNCX_MEMBER_CALL cancel_request_res_hook(CRes* res);

	extern void*(NWNCX_MEMBER_CALL *demand_res_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*);
	void* NWNCX_MEMBER_CALL demand_res_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res);

	extern CRes*(NWNCX_MEMBER_CALL *get_res_object_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t);
	CRes* NWNCX_MEMBER_CALL get_res_object_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CResRef* resref, uint16_t type);

	extern void(NWNCX_MEMBER_CALL *set_res_object_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t, CRes*);
	void NWNCX_MEMBER_CALL set_res_object_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CResRef* resref, uint16_t type, CRes* res);

	extern void(NWNCX_MEMBER_CALL *resman_update_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM uint32_t);
	void NWNCX_MEMBER_CALL resman_update_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM uint32_t p2);

	CRes* get_loaded_res(CResRef& resref, uint16_t type);
}
}