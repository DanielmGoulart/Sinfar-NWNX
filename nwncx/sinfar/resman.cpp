#include "pch.h"

#include "DXTC.h"

#ifdef __linux__
 #include <fcntl.h>
 #include <sys/types.h>
 #include <sys/stat.h>
#endif

namespace nwncx
{
namespace sinfar
{

	bool file_exists(const std::string& file_path)
	{
#ifdef WIN32
		return (GetFileAttributesA(file_path.c_str()) != INVALID_FILE_ATTRIBUTES);
#elif __linux__
		struct stat buffer;   
		return (stat(file_path.c_str(), &buffer) == 0);
#endif
	}

	unordered_map<std::string, CRes*> resources;
	unordered_map<std::string, std::unique_ptr<aurora::CKey>> all_fixed_key_tables;
	unordered_map<std::string, std::unique_ptr<aurora::CERF>> all_encapsulated_resources_files;
	unordered_map<std::string, std::unique_ptr<aurora::CResourcesDirectory>> all_directory_resources;
	unordered_map<std::string, std::unique_ptr<aurora::CResourcesDirectory>> all_dynamic_directory_resources;
	unordered_map<std::string, std::map<RESOURCE_TYPE, std::deque<aurora::CResourceData*>>> best_resources;
	uint32_t resman_total_alloc = 0;
	uint32_t resman_last_demand_id = 0;
	unordered_map<uint32_t, CRes*> resman_to_be_deleted_map;

	void free_chunk()
	{
		while (resman_total_alloc > resman_cache_size)
		{
			auto iter = resman_to_be_deleted_map.begin();
			if (iter != resman_to_be_deleted_map.end())
			{
				CRes* res_to_delete = iter->second;
				res_to_delete->vtable->Destructor(res_to_delete, NWNCX_MEMBER_X_PARAM_VAL 3);
			}
			else
			{
				log_printf("ERROR: no resource to free but %u bytes allocated\n", resman_total_alloc);
				break;
			}
		}
	}

	inline void remove_from_to_be_deleted_map(RESOURCE_ENTRY* res_entry)
	{
		if (resman_to_be_deleted_map.erase(res_entry->to_be_deleted_priority))
		{
			resman_total_alloc -= res_entry->to_be_deleted_alloc_size;
		}
	}

	inline void add_to_to_be_deleted_map(CRes* res)
	{
		free_chunk();
		RESOURCE_ENTRY* res_entry = static_cast<RESOURCE_ENTRY*>(res->entry_info);
		if (res_entry && res_entry->to_be_deleted_priority)
		{
			resman_total_alloc += res_entry->to_be_deleted_alloc_size;
			resman_to_be_deleted_map[res_entry->to_be_deleted_priority] = res;
		}
		else
		{
			log_printf("ERROR: trying to add resource to cache without an ID\n");
		}
	}

	inline void give_priority_id_to_res(RESOURCE_ENTRY* res_entry)
	{
		resman_last_demand_id++;
		res_entry->to_be_deleted_priority = resman_last_demand_id;
	}

	void free_res_data(CRes* res)
	{
		if (res->data)
		{
			res->vtable->OnResourceFreed(res);
			if (res->data)
			{
				if (res->has_low_buffer)
				{
					nwn_free(res->data-6);
				}
				else
				{
					nwn_free(res->data);
				}
				res->data = 0;
			}
			res->size = 0;
			if (res->entry_info)
			{
				RESOURCE_ENTRY* res_entry = (RESOURCE_ENTRY*)res->entry_info;
				res_entry->to_be_deleted_alloc_size = 0;
			}
		}
	}

	void alloc_res_data(CRes* res)
	{
		uint32_t alloc_size = res->size;
		if (res->has_high_buffer) alloc_size += 10;
		if (res->has_low_buffer)
		{
			alloc_size += 6;
			res->data = (char*)nwn_alloc(alloc_size) + 6;
		}
		else
		{
			res->data = (char*)nwn_alloc(alloc_size);
		}
		if (res->entry_info)
		{
			RESOURCE_ENTRY* res_entry = (RESOURCE_ENTRY*)res->entry_info;
			res_entry->to_be_deleted_alloc_size = alloc_size;
		}	
	}


	void export_all_best_resources(const std::string& dest)
	{
		for (auto res_iter=best_resources.begin(); res_iter!=best_resources.end(); res_iter++)
		{
			std::string res_ext = res_iter->first.substr(res_iter->first.length()-4);
			if (/*res_ext != ".ncs" &&
				res_ext != ".nss" &&
				res_ext != ".dlg" &&
				res_ext != ".are" &&
				res_ext != ".git" &&
				res_ext != ".utc" &&
				res_ext != ".utd" &&
				res_ext != ".ute" &&
				res_ext != ".uti" &&
				res_ext != ".utm" &&
				res_ext != ".utp" &&
				res_ext != ".utt" &&
				res_ext != ".uts" &&
				res_ext != ".utw" &&
				res_ext != ".ptt" &&
				res_ext != ".bic" &&
				//res_ext != ".itp" && //toolset and DM client
				//res_ext != ".ltr" && //to generate names
				//res_ext != ".bmp" && //toolset palette
				//res_ext != ".ini" && // char creation?*/
				res_ext != ".txt")
			{
				aurora::CResourceData* res_data = get_resource_data_from_filename(res_iter->first);
				res_data->SaveToFile(dest+"/"+res_iter->first);
			}
		}
	}

	int (*AurPaletteSet)(int, const char*) = (int (*)(int, const char*))
#ifdef WIN32
		0x00782720;
#elif __linux__
		0x08523E5C;
#endif
	class DispatchedColorsPaletteUpdate: public DispatchedUpdate
	{
	public:
		int index;
		std::string name;
		DispatchedColorsPaletteUpdate(int index, const std::string& name) : index(index), name(name) {}
		DispatchedUpdate* clone() {return new DispatchedColorsPaletteUpdate(*this);}
		void perform() 
		{
			//AurPaletteSet(index, name.c_str());
		}
	};

	unordered_map<std::string, std::unique_ptr<DispatchedUpdate>> CreateResourcesUpdateCallbackMap()
	{
		unordered_map<std::string, std::unique_ptr<DispatchedUpdate>> result;
		result["pal_skin01.tga"].reset(new DispatchedColorsPaletteUpdate(0, "pal_skin01"));
		result["pal_hair01.tga"].reset(new DispatchedColorsPaletteUpdate(1, "pal_hair01"));
		result["pal_armor01.tga"].reset(new DispatchedColorsPaletteUpdate(2, "pal_armor01"));
		result["pal_armor02.tga"].reset(new DispatchedColorsPaletteUpdate(3, "pal_armor02"));
		result["pal_cloth01.tga"].reset(new DispatchedColorsPaletteUpdate(4, "pal_cloth01"));
		result["pal_leath01.tga"].reset(new DispatchedColorsPaletteUpdate(5, "pal_leath01"));
		result["pal_tattoo01.tga"].reset(new DispatchedColorsPaletteUpdate(6, "pal_tattoo01"));
		return result;
	}
	unordered_map<std::string, std::unique_ptr<DispatchedUpdate>> resources_update_callback_map = CreateResourcesUpdateCallbackMap();

	void update_resource_data(const std::string& res_filename)
	{
		auto resources_iter = resources.find(res_filename);
		if (resources_iter != resources.end())
		{
			CRes* res = resources_iter->second;
			if (res->owner_counter == 0)
			{
				res->vtable->Destructor(res, NWNCX_MEMBER_X_PARAM_VAL 3);
			}
			else
			{
				free_res_data(resources_iter->second);
			}
		}

		//reload specific resources
		auto iter = resources_update_callback_map.find(res_filename);
		if (iter != resources_update_callback_map.end())
		{
			dispatch_update(std::unique_ptr<DispatchedUpdate>(iter->second->clone()));
		}
	}

	aurora::CResourceData* get_resource_data_from_filename(const std::string res_filename)
	{
		auto all_res_iter = best_resources.find(res_filename);
		if (all_res_iter != best_resources.end())
		{
			auto res_containers_iter = all_res_iter->second.begin();
			if (res_containers_iter != all_res_iter->second.end())
			{
				if (res_containers_iter->second.size() > 0)
				{
					return res_containers_iter->second.front();
				}
			}
		}
		return NULL;
	}

	void add_resources_container_to_best_resources(aurora::CResourcesContainer* resources_container, RESOURCE_TYPE res_type)
	{
		for (auto iter = resources_container->resources.begin(); iter != resources_container->resources.end(); iter++)
		{
			std::string res_filename = iter->first.GetFileName();
			update_resource_data(res_filename);
			best_resources[res_filename][res_type].push_front(iter->second.get());
		}
	}
	void remove_resources_container_from_best_resources(aurora::CResourcesContainer* resources_container, RESOURCE_TYPE res_type)
	{
		for (auto all_res_in_container_iter = resources_container->resources.begin(); all_res_in_container_iter != resources_container->resources.end(); all_res_in_container_iter++)
		{
			std::string res_filename = all_res_in_container_iter->first.GetFileName();
			update_resource_data(res_filename);
			auto best_resource_stacks_iter = best_resources.find(res_filename);
			auto& best_resource_stacks = best_resource_stacks_iter->second;
			auto container_stack_iter = best_resource_stacks.find(res_type);
			if (container_stack_iter != best_resource_stacks.end())
			{
				auto& container_stack = container_stack_iter->second;
				for (auto iter = container_stack.begin(); iter != container_stack.end();)
				{
					if (*iter == all_res_in_container_iter->second.get())
					{
						auto to_erase = iter;
						iter++;
						container_stack.erase(to_erase);
						break;
					}
					else
					{
						iter++;
					}
				}
				if (container_stack.empty())
				{
					best_resource_stacks.erase(container_stack_iter);
					if (best_resource_stacks.empty())
					{
						best_resources.erase(best_resource_stacks_iter);
					}
				}
			}
		}
	}

#ifdef WIN32
	CExoString*(NWNCX_MEMBER_CALL *resolve_filename_org)(void*, NWNCX_MEMBER_X_PARAM CExoString*, CExoString*, uint16_t) =
		(CExoString*(NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM CExoString*, CExoString*, uint16_t))0x005BE220;
#elif __linux__
		CExoString (*ResolveFileName)(CExoAliasList*, CExoString*, uint16_t) = (CExoString (*)(CExoAliasList*, CExoString*, uint16_t))0x085A353C;
#endif
	std::string resolve_filename(const std::string& filename)
	{
		CExoString from(filename.c_str());
#ifdef WIN32
		CExoString result;
		resolve_filename_org((*p_exo_base)->exo_aliases, NWNCX_MEMBER_X_PARAM_VAL &result, &from, -1);
#elif __linux__
		CExoString result = ResolveFileName((*p_exo_base)->exo_aliases, &from, -1);
#endif
		return (result.text ? aurora::strtolower(result.text) : "");
	}

	int(NWNCX_MEMBER_CALL *add_encapsulated_resource_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBB80;
	int NWNCX_MEMBER_CALL add_encapsulated_resource_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		if (ini_resman_use_nwndata_folder && strncmp(filename->text, "TEXTUREPACKS:", 13)==0)
		{
			return true;
		}

		std::string resolved_filename = resolve_filename(filename->text);
		std::string found_filename = resolved_filename + ".hak";
		if (!file_exists(found_filename))
		{
			found_filename = resolved_filename + ".erf";
			if (!file_exists(found_filename))
			{
				found_filename = resolved_filename + ".mod";
				if (!file_exists(found_filename))
				{
					found_filename = resolved_filename + ".sav";
					if (!file_exists(found_filename))
					{
						found_filename = resolved_filename + ".nwm";
						if (!file_exists(found_filename))
						{
							log_printf("ERROR: erf not found:%s\n", resolved_filename.c_str());
							return false;
						}
					}
				}
			}
		}
		std::unique_ptr<aurora::CERF> erf(new aurora::CERF(found_filename));
		if (erf->resources.size() > 0)
		{
			if (all_encapsulated_resources_files.find(filename->text) != all_encapsulated_resources_files.end())
			{
				remove_encapsulated_resource_file_hook(resman, NWNCX_MEMBER_X_PARAM_VAL filename);
			}
			add_resources_container_to_best_resources(erf.get(), ENCAPSULATED);
			all_encapsulated_resources_files[filename->text] = std::move(erf);
			return true;
		}
		else
		{
			log_printf("ERROR: failed to load erf:%s\n", resolved_filename.c_str());
			return false;
		}
	}

	int(NWNCX_MEMBER_CALL *add_resource_image_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*, uint8_t*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*, uint8_t*))0x005CBBA0;
	int NWNCX_MEMBER_CALL add_resource_image_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename, uint8_t* p2)
	{
		log_printf("WARNING: Adding resource image file:%s\n", resolve_filename(filename->text).c_str());
		return true;
	}

	int add_single_fixed_key_table(const std::string& filename)
	{
		std::unique_ptr<aurora::CKey> key_table(new aurora::CKey(filename, "."));
		if (key_table->resources.size())
		{
			add_resources_container_to_best_resources(key_table.get(), KEY_TABLE);
			all_fixed_key_tables[filename] = std::move(key_table);
			return true;
		}
		else
		{
			log_printf("ERROR: failed to load key table:%s\n", filename.c_str());
			return false;
		}
	}

	bool fixed_key_table_loaded = false;
	int(NWNCX_MEMBER_CALL *add_fixed_key_table_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBBC0;
	int NWNCX_MEMBER_CALL add_fixed_key_table_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		if (!fixed_key_table_loaded)
		{
			if (ini_resman_use_nwndata_folder)
			{
				add_resource_directory("sinfarx_nwndata", DIRECTORY, false);

				log_printf("Using sinfarx_nwndata folder\n");
			}
			else
			{
				add_single_fixed_key_table("chitin.key");
				add_single_fixed_key_table("xp1.key");
				add_single_fixed_key_table("xp2.key");
				add_single_fixed_key_table("xp3.key");
				add_single_fixed_key_table("xp2patch.key");
			}
			(*p_exo_base)->exp_pack = 1|2;
			fixed_key_table_loaded = true;
		}
		return true;
	}

	class DispatchResDirectoryRefresh : public DispatchedUpdate
	{
	public:
		DispatchResDirectoryRefresh(const std::string& res_dir_path) : res_dir_path(res_dir_path){}
		DispatchedUpdate* clone() {return new DispatchResDirectoryRefresh(*this);}
		void perform()
		{
			debug_log_printf(3, "Change detected in the directory:%s\n", res_dir_path.c_str());

			for (auto iter=all_directory_resources.begin(); iter!=all_directory_resources.end(); iter++)
			{
				if (iter->second->dir_path == res_dir_path)
				{
					remove_resources_container_from_best_resources(iter->second.get(), DYNAMIC_DIRECTORY);
					iter->second->RefreshResources();
					add_resources_container_to_best_resources(iter->second.get(), DYNAMIC_DIRECTORY);
					break;
				}
			}
		}
		std::string res_dir_path;
	};

	//called from another thread
	void on_res_changed(const std::string& res_dir_path)
	{
		aurora::lock_guard lock(dispatched_updates_mutex);

		for (auto iter=dispatched_updates.begin(); iter!=dispatched_updates.end(); iter++)
		{
			DispatchResDirectoryRefresh* dispatch_res_directory_request = dynamic_cast<DispatchResDirectoryRefresh*>(iter->get());
			if (dispatch_res_directory_request && 
				dispatch_res_directory_request->res_dir_path == res_dir_path)
			{
				return;
			}
		}
		dispatch_update(std::unique_ptr<DispatchResDirectoryRefresh>(new DispatchResDirectoryRefresh(res_dir_path)));
	}

	int add_resource_directory(const std::string& dir_path, RESOURCE_TYPE res_type, bool do_resolve_filename)
	{
		std::string resolved_filename = do_resolve_filename ? resolve_filename(dir_path) : dir_path;
		if (file_exists(resolved_filename))
		{
			std::unique_ptr<aurora::CResourcesDirectory> dir_res(new aurora::CResourcesDirectory(resolved_filename, (res_type==DYNAMIC_DIRECTORY?on_res_changed:NULL)));
			if (all_directory_resources.find(dir_path) != all_directory_resources.end())
			{
				remove_resource_directory(dir_path, res_type);
			}
			add_resources_container_to_best_resources(dir_res.get(), res_type);
			all_directory_resources[dir_path] = std::move(dir_res);
		}
		else
		{
			log_printf("ERROR: directory not found:%s\n", resolved_filename.c_str());
			return false;
		}
		return true;
	}

	int(NWNCX_MEMBER_CALL *add_resource_directory_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBBE0;
	int NWNCX_MEMBER_CALL add_resource_directory_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		return add_resource_directory(filename->text, DIRECTORY);
	}

	int(NWNCX_MEMBER_CALL *remove_encapsulated_resource_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) = 
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBC00;
	int NWNCX_MEMBER_CALL remove_encapsulated_resource_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		if (ini_resman_use_nwndata_folder && strncmp(filename->text, "TEXTUREPACKS:", 13)==0)
		{
			return true;
		}

		auto encapsulated_iter = all_encapsulated_resources_files.find(filename->text);
		if (encapsulated_iter != all_encapsulated_resources_files.end())
		{
			remove_resources_container_from_best_resources(encapsulated_iter->second.get(), ENCAPSULATED);
			all_encapsulated_resources_files.erase(encapsulated_iter);
			return true;
		}
		else
		{
			log_printf("ERROR: failed to remove erf:%s\n", filename->text);
			return false;
		}
	}

	int(NWNCX_MEMBER_CALL *remove_resource_image_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBC10;
	int NWNCX_MEMBER_CALL remove_resource_image_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		return true;
	}

	int(NWNCX_MEMBER_CALL *remove_fixed_key_table_file_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBC20;
	int NWNCX_MEMBER_CALL remove_fixed_key_table_file_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		return true;
	}

	void remove_resource_directory(const std::string& dir_path, RESOURCE_TYPE res_type)
	{
		auto res_directory_iter = all_directory_resources.find(dir_path);
		if (res_directory_iter != all_directory_resources.end())
		{
			remove_resources_container_from_best_resources(res_directory_iter->second.get(), res_type);
			all_directory_resources.erase(res_directory_iter);
		}
	}

	int(NWNCX_MEMBER_CALL *remove_resource_directory_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBC30;
	int NWNCX_MEMBER_CALL remove_resource_directory_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		remove_resource_directory(filename->text, DIRECTORY);
		return true;
	}

	int(NWNCX_MEMBER_CALL *update_resource_directory_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CBC40;
	int NWNCX_MEMBER_CALL update_resource_directory_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CExoString* filename)
	{
		remove_resource_directory(filename->text, DIRECTORY);
		add_resource_directory(filename->text, DIRECTORY);
		return true;
	}

	void(NWNCX_MEMBER_CALL *destroy_res_org)(CRes* NWNCX_DESTRUCTOR_P2) = 
		(void (NWNCX_MEMBER_CALL *)(CRes* NWNCX_DESTRUCTOR_P2))0x005CB6A0;
	void NWNCX_MEMBER_CALL destroy_res_hook(CRes* res NWNCX_DESTRUCTOR_P2)
	{
		RESOURCE_ENTRY* res_entry = static_cast<RESOURCE_ENTRY*>(res->entry_info);
		if (res_entry)
		{
			remove_from_to_be_deleted_map(res_entry);
			auto resources_iter = resources.find(res_entry->filename);
			if (resources_iter != resources.end())
			{
				resources.erase(resources_iter);
			}
			delete res_entry;
			res->entry_info = NULL;
		}
		free_res_data(res);
		return destroy_res_org(res NWNCX_DESTRUCTOR_P2_VAL);
	}

	int(NWNCX_MEMBER_CALL *resource_exists_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t, uint32_t*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t, uint32_t*))0x005CC1C0;
	int NWNCX_MEMBER_CALL resource_exists_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CResRef* resref, uint16_t type, uint32_t* table_type)
	{
		CUSTOM_RESOURCE* custom_resource = get_custom_resource(resref);
		if (custom_resource)
		{
			if (custom_resource->get_main_res_type() == aurora::NwnResType_TGA && type == aurora::NwnResType_DDS)
			{
				return false;
			}
			return resource_exists_hook(resman, NWNCX_MEMBER_X_PARAM_VAL &custom_resource->original_resref, type, table_type);
		}

		auto all_res_iter = best_resources.find(aurora::CResource(static_cast<aurora::NwnResType>(type), resref->value).GetFileName());
		if (all_res_iter != best_resources.end())
		{
			auto res_containers_iter = all_res_iter->second.begin();
			if (res_containers_iter != all_res_iter->second.end())
			{
				if (res_containers_iter->second.size() > 0)
				{
					if (table_type) *table_type = (4-static_cast<uint32_t>(res_containers_iter->first));
					return true;
				}
			}
		}
		return false;
	}

	int resource_exists(CResRef& resref, uint16_t res_type)
	{
		return 	resource_exists_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL &resref, res_type, NULL);
	}

	inline aurora::CResourcesContainer* get_resource_container_from_res(CRes* res)
	{
		if (!res->entry_info) return NULL;
		std::string res_filename = static_cast<RESOURCE_ENTRY*>(res->entry_info)->filename;
		auto all_res_iter = best_resources.find(res_filename);
		if (all_res_iter != best_resources.end())
		{
			auto res_containers_iter = all_res_iter->second.begin();
			if (res_containers_iter != all_res_iter->second.end())
			{
				unordered_map<std::string, std::unique_ptr<aurora::CResourcesContainer>>* all_containers_of_resource_type = NULL;
				switch (res_containers_iter->first)
				{
				case ENCAPSULATED: 
					all_containers_of_resource_type = reinterpret_cast<unordered_map<std::string, std::unique_ptr<aurora::CResourcesContainer>>*>(&all_encapsulated_resources_files);
					break;
				case DIRECTORY:
					all_containers_of_resource_type = reinterpret_cast<unordered_map<std::string, std::unique_ptr<aurora::CResourcesContainer>>*>(&all_directory_resources);
					break;
				case KEY_TABLE:
					all_containers_of_resource_type = reinterpret_cast<unordered_map<std::string, std::unique_ptr<aurora::CResourcesContainer>>*>(&all_fixed_key_tables);
					break;
				case DYNAMIC_DIRECTORY:
					all_containers_of_resource_type = reinterpret_cast<unordered_map<std::string, std::unique_ptr<aurora::CResourcesContainer>>*>(&all_dynamic_directory_resources);
					break;
				}
				if (all_containers_of_resource_type)
				{
					for (auto resource_container_iter=all_containers_of_resource_type->begin(); resource_container_iter!=all_containers_of_resource_type->end(); resource_container_iter++)
					{
						auto& resources_in_container = resource_container_iter->second->resources;
						auto resource_in_container_iter = resources_in_container.find(aurora::CResource(res_filename));
						if (resource_in_container_iter != resources_in_container.end())
						{
							return resource_container_iter->second.get();
						}
					}
				}
			}
		}
		return NULL;
	}

	void (NWNCX_MEMBER_CALL *CExoStringList_constructor)(CExoStringList*, NWNCX_MEMBER_X_PARAM int, int, int) =
		(void (NWNCX_MEMBER_CALL *)(CExoStringList*, NWNCX_MEMBER_X_PARAM int, int, int))
#ifdef WIN32
0x005BF550;
#elif __linux__
0x085A4C48;
#endif
	void (NWNCX_MEMBER_CALL *CExoStringList_add)(CExoStringList*, NWNCX_MEMBER_X_PARAM CExoString*) =
		(void (NWNCX_MEMBER_CALL *)(CExoStringList*, NWNCX_MEMBER_X_PARAM CExoString*))
#ifdef WIN32
0x005BF5E0;
#elif __linux__
0x085A4678;
#endif
	CExoStringList*(NWNCX_MEMBER_CALL *get_res_of_type_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM uint16_t, CRes*) = 
		(CExoStringList* (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM uint16_t, CRes*))0x005CC600;
	CExoStringList* NWNCX_MEMBER_CALL get_res_of_type_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM uint16_t type, CRes* res)
	{
		aurora::CResourcesContainer* res_container = get_resource_container_from_res(res);
		if (res_container)
		{
			CExoStringList* res_of_type = (CExoStringList*)nwn_alloc(sizeof(CExoStringList));
			CExoStringList_constructor(res_of_type, NWNCX_MEMBER_X_PARAM_VAL 0, 1, 5);
			for (auto res_iter=res_container->resources.begin(); res_iter!=res_container->resources.end(); res_iter++)
			{
				if (res_iter->first.type == type)
				{
					char res_resref_str[17];
					strncpy(res_resref_str, res_iter->first.resref.value, 16);
					res_resref_str[16] = 0;
					CExoString* res_resref_exo_str = new(nwn_alloc(sizeof(CExoString))) CExoString(res_resref_str);
					CExoStringList_add(res_of_type, NWNCX_MEMBER_X_PARAM_VAL res_resref_exo_str);
				}
			}
			return res_of_type;
		}
		log_printf("ERROR: get_res_of_type failed\n");
		return NULL;
	}

	uint32_t(NWNCX_MEMBER_CALL *get_table_count_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*, int) = 
		(uint32_t (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*, int))0x005CCA20;
	uint32_t NWNCX_MEMBER_CALL get_table_count_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res, int p2)
	{
		aurora::CResourcesContainer* res_container = get_resource_container_from_res(res);
		if (res_container)
		{
			return res_container->resources.size();
		}
		log_printf("ERROR: get_table_count failed\n");
		return 0;
	}

	int(NWNCX_MEMBER_CALL *release_res_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*))0x005CCBA0;
	int NWNCX_MEMBER_CALL release_res_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res)
	{
		//res->demands--;
		return true;
	}

	int(NWNCX_MEMBER_CALL *release_res_object_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*))0x005CCB40;
	int NWNCX_MEMBER_CALL release_res_object_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res)
	{
		if (res->owner_counter <= 1)
		{
			res->owner_counter = 0;
			if (res->data && 
				res->entry_info && 
				static_cast<RESOURCE_ENTRY*>(res->entry_info)->filename.length() > 0 &&
				static_cast<RESOURCE_ENTRY*>(res->entry_info)->filename.at(0)!='¬')
			{
				add_to_to_be_deleted_map(res);
			}
			else
			{
				res->vtable->Destructor(res, NWNCX_MEMBER_X_PARAM_VAL 3);
			}
		}
		else
		{
			res->owner_counter--;
		}
		return true;
	}

	int NWNCX_MEMBER_CALL on_release_res_before_destroying_it(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res)
	{
		res->owner_counter = 0;
		/*//add it to the cache ... to be safe... but normally it should destroy itself automatically
		RESOURCE_ENTRY* res_entry = static_cast<RESOURCE_ENTRY*>(res->entry_info);
		if (res_entry)
		{
			if (!res_entry->to_be_deleted_priority)
			{
				give_priority_id_to_res(res_entry);
			}
			add_to_to_be_deleted_map(res);
		}*/
		return true;
	}

	int(NWNCX_MEMBER_CALL *free_res_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*) =
		(int (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*))0x005CC210;
	int NWNCX_MEMBER_CALL free_res_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res)
	{
		return true;
	}

	void(NWNCX_MEMBER_CALL *free_res_data_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*) =
		(void (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*))0x005CC2F0;
	void NWNCX_MEMBER_CALL free_res_data_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res)
	{
		//it will be freed later...
	}

	int(NWNCX_MEMBER_CALL *free_chunk_org)(CExoResMan*) = (int (NWNCX_MEMBER_CALL *)(CExoResMan*))0x005CC350;
	int NWNCX_MEMBER_CALL free_chunk_hook(CExoResMan* resman)
	{
		free_chunk();
		return true;
	}

	int(NWNCX_MEMBER_CALL *request_res_org)(CRes*) = (int(NWNCX_MEMBER_CALL *)(CRes*))0x005CB890;
	int NWNCX_MEMBER_CALL request_res_hook(CRes* res)
	{
		//res->requests++;
		return true;
	}

	int(NWNCX_MEMBER_CALL *cancel_request_res_org)(CRes*) = (int(NWNCX_MEMBER_CALL *)(CRes*))0x005CB780;
	int NWNCX_MEMBER_CALL cancel_request_res_hook(CRes* res)
	{
		//res->requests--;
		return true;
	}

	inline uint32_t get_res_type_size(uint16_t type) {
		switch (type) 
		{
		case aurora::NwnResType_PLT: return 0x48;
		case aurora::NwnResType_TXI: return 0x38;
		case aurora::NwnResType_TGA: return 0x50;
		case aurora::NwnResType_DDS: return 0x3C;
		}
		return 0;
	}
	inline long get_res_constructor_ptr(uint16_t type) {
		switch (type) 
		{
#ifdef WIN32
		case aurora::NwnResType_PLT: return 0x007FA0B0;
		case aurora::NwnResType_TXI: return 0x007F9FF0;
		case aurora::NwnResType_TGA: return 0x00859C30;
		case aurora::NwnResType_DDS: return 0x007FA2C0;
#elif __linux__
		case aurora::NwnResType_PLT: return 0x085537F0;
		case aurora::NwnResType_TXI: return 0x08551368;
		case aurora::NwnResType_TGA: return 0x08594328;
		case aurora::NwnResType_DDS: return 0x08552C68;
#endif
		}
		return 0;
	}
	CRes* get_loaded_res(CResRef& resref, uint16_t type)  {
		if (type != aurora::NwnResType_PLT &&
			type != aurora::NwnResType_TXI &&
			type != aurora::NwnResType_TGA && 
			type != aurora::NwnResType_DDS) 
		{
			log_printf("ERROR: Unsupported custom resource type:%s\n", aurora::CResource(static_cast<aurora::NwnResType>(type), resref.value).GetFileName().c_str());
			return NULL;
		}
		CRes* res = get_res_object_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL &resref, type);
		if (!res)
		{
			res = (CRes*)nwn_alloc(get_res_type_size(type));
			((int (NWNCX_MEMBER_CALL *)(CRes*))get_res_constructor_ptr(type))(res);
			set_res_object_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL &resref, type, res);
		}
		demand_res_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL res);
		release_res_object_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL res);
		if (res->data == NULL) log_printf("ERROR: Failed to load custom resource:%s\n", aurora::CResource(static_cast<aurora::NwnResType>(type), resref.value).GetFileName().c_str());
		return res;
	}

	bool dds_data_to_tga_data(DDS_DATA* dds_data, char* tga_data, uint32_t tga_data_size)
	{
		DTXTC::compressionType type;
		if (dds_data->color_channels == 3)
			type = DTXTC::DXT1;
		else if (dds_data->color_channels  == 4)
			type = DTXTC::DXT5;
		else {
			log_printf("DDS is using an unknown number of channels:%d\n", dds_data->color_channels);
			return false;
		}

		DTXTC::Decompress(dds_data->data, (DTXTC::COLOR_ALPHA*)(tga_data+sizeof(TGA_HEADER)), dds_data->width, dds_data->height, type);
		new(tga_data) TGA_HEADER(dds_data->width, dds_data->height);

		return true;
	}

	std::deque<std::string> last_demand_res_filename;
	void*(NWNCX_MEMBER_CALL *demand_res_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*) =
		(void* (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CRes*))0x005CC010;
	void* NWNCX_MEMBER_CALL demand_res_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CRes* res)
	{
		if (!res)
		{
			//log_printf("ERROR: Resource is NULL\n");
			return NULL;
		}

		RESOURCE_ENTRY* res_entry = static_cast<RESOURCE_ENTRY*>(res->entry_info);
		if (res_entry->filename.length() <= 4)
		{
			//log_printf("ERROR: invalid resref:%s\n", last_demand_res_filename.c_str());
			return NULL;
		}
		last_demand_res_filename.push_back(res_entry->filename);
		if (last_demand_res_filename.size() > 30)
		{
			last_demand_res_filename.pop_front();
		}

		if (ini_resman_log_requested_resources)
		{
			log_printf("RR:%s\n", res_entry->filename.c_str());
		}
		
		if (res->data == NULL)
		{
			aurora::NwnResType res_type = aurora::NwnGetResTypeFromExtension(res_entry->filename.c_str() + res_entry->filename.length()-3);
			aurora::CResourceData* res_data = NULL;
			bool load_tga_from_dds_data = false;
			CUSTOM_RESOURCE* custom_resource = get_custom_resource(res_entry->filename);
			if (custom_resource)
			{
				char new_filename[21];
				strncpy(new_filename, custom_resource->original_resref.value, 16);
				if (ini_resman_log_requested_resources)
				{
					log_printf("CR:%s\n", new_filename);
				}
				new_filename[16] = 0;
				strcat(new_filename, res_entry->filename.c_str()+res_entry->filename.length()-4);
				res_data = get_resource_data_from_filename(new_filename);
				if (!res_data)
				{
					if (res_type == aurora::NwnResType_TGA)
					{
						memcpy(new_filename+strlen(new_filename)-3, "dds", 3);
						res_data = get_resource_data_from_filename(new_filename);
						load_tga_from_dds_data = true;
					}
				}
			}
			else
			{
				res_data = get_resource_data_from_filename(res_entry->filename);
			}
			if (res_data)
			{	
				res->size = res_data->GetDataSize();
				if (res->size > 0)
				{
					uint32_t alloc_size = res->size;
					if (custom_resource)
					{
						if (load_tga_from_dds_data)
						{
							CRes* org_res = get_loaded_res(custom_resource->original_resref, aurora::NwnResType_DDS);
							if (org_res)
							{
								DDS_DATA* dds_data = (DDS_DATA*)org_res->data;
								res->size = sizeof(TGA_HEADER)+(dds_data->width*dds_data->height*4);
								alloc_res_data(res);
								if (!dds_data_to_tga_data(dds_data, res->data, res->size))
								{
									return NULL;
								}
							}
							else
							{
								log_printf("Failed to load DDS for custom TGA resource\n");
								return NULL;
							}
						}
						else
						{
							CRes* org_res = get_loaded_res(custom_resource->original_resref, res_type);
							if (res_type == aurora::NwnResType_TGA)
							{
								CResTGA* res_tga = (CResTGA*)org_res;
								uint32_t num_bits = res_tga->header->num_bits;
								uint32_t width = res_tga->header->width;
								uint32_t height = res_tga->header->height;
								uint32_t num_pixels = width*height;
								uint32_t pixels_data_size = num_pixels*sizeof(TGAPixel_32);
								res->size = sizeof(TGA_HEADER)+pixels_data_size;
								alloc_res_data(res);
								new(res->data) TGA_HEADER(width, height);
								((TGA_HEADER*)res->data)->img_descriptor = ((TGA_HEADER*)(org_res->data))->img_descriptor;
								TGAPixel_32* tga_pixels_data = (TGAPixel_32*)(res->data+sizeof(TGA_HEADER));
								if (num_bits == 32)
								{
									memcpy(tga_pixels_data, res_tga->tga_data, pixels_data_size);
								}
								else if (num_bits == 24)
								{
									TGAPixel_24* org_tga_pixels_data = (TGAPixel_24*)res_tga->tga_data;
									for (uint32_t pixel=0; pixel<num_pixels; pixel++)
									{
										tga_pixels_data[pixel].alpha = 0xFF;
										tga_pixels_data[pixel].red = org_tga_pixels_data[pixel].red;
										tga_pixels_data[pixel].green = org_tga_pixels_data[pixel].green;
										tga_pixels_data[pixel].blue = org_tga_pixels_data[pixel].blue;
									}
								}
								else
								{
									log_printf("Unsupported TGA number of bits:%d\n", num_bits);
									return NULL;
								}
							}
							else
							{
								alloc_res_data(res);
								memcpy(res->data, org_res->data, res->size);
							}
						}
						custom_resource->pre_modify_original_res(res, res_type);
					}
					else
					{
						alloc_res_data(res);
						res_data->ReadData((char*)res->data);
					}

					//fix plt using an invalid palette
					if (res_type == aurora::NwnResType_PLT)
					{
						bool found_invalid_pixel = false;
						PLT_Header* plt_header = reinterpret_cast<PLT_Header*>(res->data);
						uint32_t num_pixels = plt_header->ulWidth*plt_header->ulHeight;
						for (uint32_t i = 0; i < num_pixels; i++)
						{
							PLT_Pixel* pixel = reinterpret_cast<PLT_Pixel*>(res->data + sizeof(PLT_Header) + (i * sizeof(PLT_Pixel)));
							if (pixel->ucPalette > 9)
							{
								pixel->ucPalette = 0;
								found_invalid_pixel = true;
							}
						}
						if (found_invalid_pixel)
						{
							debug_log_printf(3, "ERROR: %s is using an invalid palette\n", res_entry->filename.c_str());
						}
					}

					if (!res->vtable->OnResourceServiced(res))
					{
						log_printf("ERROR: Invalid resource:%s\n", res_entry->filename.c_str());
						free_res_data(res);
					}
					else
					{
						if (custom_resource)
						{
							custom_resource->post_modify_original_res(res, res_type);
						}
					}
				}
				else
				{
					log_printf("ERROR: Resource is empty:%s\n", res_entry->filename.c_str());	
				}
			}
			else if (ini_resman_log_missing_resources)
			{
				if (res_type != aurora::NwnResType_TXI &&
					res_type != aurora::NwnResType_PWK)
				{
					log_printf("[Missing Resource]: %s\n", res_entry->filename.c_str());
				}
			}
		}

		//res->demands++;

		if (res->data)
		{
			give_priority_id_to_res(res_entry);
		}

		return res->data;
	}

	CRes*(NWNCX_MEMBER_CALL *get_res_object_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t) =
		(CRes* (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t))0x005CC630;
	CRes* NWNCX_MEMBER_CALL get_res_object_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CResRef* resref, uint16_t type)
	{
		aurora::CResource aurora_res(static_cast<aurora::NwnResType>(type), resref->value);
		auto resources_iter = resources.find(aurora_res.GetFileName());
		if (resources_iter != resources.end())
		{
			CRes* res = resources_iter->second;
			if (res->owner_counter == 0 && res->entry_info)
			{
				remove_from_to_be_deleted_map(static_cast<RESOURCE_ENTRY*>(res->entry_info));
			}
			res->owner_counter++;
			return res;
		}
		return NULL;
	}

	void(NWNCX_MEMBER_CALL *set_res_object_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t, CRes*) =
		(void (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM CResRef*, uint16_t, CRes*))0x005CC690;
	void NWNCX_MEMBER_CALL set_res_object_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM CResRef* resref, uint16_t type, CRes* res)
	{
		aurora::CResource aurora_res(static_cast<aurora::NwnResType>(type), resref->value);
		std::string res_filename = aurora_res.GetFileName();
		auto resources_iter = resources.find(res_filename);
		if (resources_iter != resources.end())
		{
			log_printf("ERROR: requesting an already stored resource\n");
			CRes* old_res = resources_iter->second;
			old_res->vtable->Destructor(old_res, NWNCX_MEMBER_X_PARAM_VAL 3);
		}
		if (res->entry_info != NULL)
		{
			log_printf("ERROR: resource entry already initialized\n");
		}
		RESOURCE_ENTRY* res_entry = new RESOURCE_ENTRY(res_filename);
		res->entry_info = res_entry;
		resources[res_filename] = res;
		res->owner_counter = 1;

		//if (resources.size() % 10 == 0) log_printf("res count:%u\n", resources.size());
	}

	void(NWNCX_MEMBER_CALL *resman_update_org)(CExoResMan*, NWNCX_MEMBER_X_PARAM uint32_t) = 
		(void (NWNCX_MEMBER_CALL *)(CExoResMan*, NWNCX_MEMBER_X_PARAM uint32_t))0x005CD410;
	void NWNCX_MEMBER_CALL resman_update_hook(CExoResMan* resman, NWNCX_MEMBER_X_PARAM uint32_t p2)
	{
		return;
	}

}
}