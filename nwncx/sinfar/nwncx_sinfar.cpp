#include "pch.h"

namespace nwncx
{
namespace sinfar
{
	int NWNCX_SINFAR_VERSION = 404;

	void dummy_printstacktrace(const std::string& info)
	{
		log_printf("PLACEHOLDER: %s\n", info);
	}
	void(*print_stacktrace)(const std::string&) = dummy_printstacktrace;

	class CUSTOM_PLT : public PC_PART_XDATA, public CUSTOM_RESOURCE
	{
	public:
		uint16_t get_main_res_type() 
		{
			return aurora::NwnResType_PLT;
		}
		void pre_modify_original_res(CRes* res, uint16_t res_type) 
		{
			if (res_type == aurora::NwnResType_PLT)
			{
				PLT_Header* plt_header = reinterpret_cast<PLT_Header*>(res->data);
				uint32_t num_pixels = plt_header->ulWidth*plt_header->ulHeight;
				for (uint32_t i = 0; i < num_pixels; i++)
				{
					PLT_Pixel* pixel = reinterpret_cast<PLT_Pixel*>(res->data+sizeof(PLT_Header)+(i*sizeof(PLT_Pixel)));
					uint8_t palette = pixel->ucPalette;
					if (palette > 9) palette = 0;
					int8_t channel_lightness_mod = channels_lightness_mod[palette];
					if (channel_lightness_mod)
					{
						int new_uc_index = channel_lightness_mod + pixel->ucIndex;
						if (new_uc_index < 0) pixel->ucIndex = 0;
						else if (new_uc_index > 255) pixel->ucIndex = 255;
						else pixel->ucIndex = new_uc_index;
					}
				}
			}
		}
	};

	class CUSTOM_TGA : public CUSTOM_RESOURCE
	{
	private:
		inline void mod_color_value(uint8_t& color_value, int8_t& color_mod)
		{
			color_value = truncate_color(color_value + (color_mod * 2));
		}
		inline uint8_t truncate_color(int value)
		{
			return (value < 0 ? 0 : (value > 255 ? 255 : value ));
		}
		RGBA_MOD rgba_mod;
		CUSTOM_TGA();
	public:
		CUSTOM_TGA(RGBA_MOD rgba_mod) : CUSTOM_RESOURCE()
		{
			memcpy(this->rgba_mod, rgba_mod, sizeof(RGBA_MOD));
		}
		uint16_t get_main_res_type() 
		{
			return aurora::NwnResType_TGA;
		}
		uint16_t get_second_res_type() 
		{ 
			return aurora::NwnResType_DDS; 
		}
		void post_modify_original_res(CRes* res, uint16_t res_type)
		{
			if (res_type == aurora::NwnResType_TGA)
			{
				TGA_HEADER* tga_header = (TGA_HEADER*)res->data;
				if (tga_header->pixels_depth == 32)
				{
					TGAPixel_32* tga_pixels = (TGAPixel_32*)(res->data + sizeof(TGA_HEADER));
					uint32_t num_pixels = tga_header->width*tga_header->height;
					if (rgba_mod[0] || rgba_mod[1] || rgba_mod[2] || rgba_mod[3])
					{
						for (uint32_t i = 0; i<num_pixels; i++)
						{
							mod_color_value(tga_pixels[i].red, rgba_mod[0]);
							mod_color_value(tga_pixels[i].green, rgba_mod[1]);
							mod_color_value(tga_pixels[i].blue, rgba_mod[2]);
							mod_color_value(tga_pixels[i].alpha, rgba_mod[3]);
						}
					}

					CXImage ximage(tga_pixels, tga_header->width, tga_header->height);
					if (rgba_mod[4])
					{
						ximage.Noise((uint8_t)rgba_mod[4]);
					}
					if (rgba_mod[5])
					{
						ximage.Jitter(((uint8_t)rgba_mod[5]) * 5);
					}
					if (rgba_mod[6])
					{
						ximage.Light(rgba_mod[6] * 2);
					}
					if (rgba_mod[7] || rgba_mod[8] || rgba_mod[9])
					{
						ximage.Colorize((uint8_t)rgba_mod[7], (uint8_t)rgba_mod[8], (float)(1.0 / 255.0)*(255 - (uint8_t)rgba_mod[9]));
					}
					if (rgba_mod[10])
					{
						ximage.Solarize(255 - (uint8_t)rgba_mod[10]);
					}
					if (rgba_mod[13])
					{
						ximage.Gamma((float)(5.0 / 255.0)*((uint8_t)rgba_mod[13]));
					}

					/*
					if (rgba_mod[11])
					{
						//blur
						ximage.Repair((float)((30.0/255.0)*((uint8_t)rgba_mod[11])));
					}
					if (rgba_mod[12])
					{
						ximage.Rotate((float)(360.0/255.0)*((uint8_t)rgba_mod[12]));
					}*/
				}
			}
		}
	};
	class PC_PARTS_CUSTOM_PLT: public PC_PARTS<CUSTOM_PLT>
	{
	public:
		int update(PC_PARTS_CUSTOM_PLT& update_parts)
		{
			int result = 0;
			for (int i=0; i<get_num_parts(); i++)
			{
				PC_PART_XDATA& current_part = parts[i];
				PC_PART_XDATA& update_part = update_parts[i];
				if (memcmp(&current_part.channels_mappping, &update_part.channels_mappping, sizeof(current_part.channels_mappping))!=0 ||
					memcmp(&current_part.channels_lightness_mod, &update_part.channels_lightness_mod, sizeof(current_part.channels_lightness_mod))!=0 ||
					memcmp(&current_part.channels_color, &update_part.channels_color, sizeof(current_part.channels_color))!=0 ||
					current_part.channels_color_override != update_part.channels_color_override ||
					current_part.scaling != update_part.scaling ||
					current_part.extra_x != update_part.extra_x ||
					current_part.extra_y != update_part.extra_y ||
					current_part.extra_z != update_part.extra_z ||
					current_part.extra_rotate_x != update_part.extra_rotate_x ||
					current_part.extra_rotate_y != update_part.extra_rotate_y ||
					current_part.extra_rotate_z != update_part.extra_rotate_z)
				{
					current_part = update_part;
					parts[i].set_custom_resource_id(0);
					result |= (1<<i);
				}
			}
			return result;
		}
	};

#pragma pack(push, 1)
	class GobExtra
	{
	private:
		CUSTOM_PLT* p_custom_plt;
		PC_PART_XDATA* p_xdata;
		std::vector<CUSTOM_TGA*>* p_custom_tgas;
	public:
		inline void set_custom_plt(CUSTOM_PLT* new_custom_plt)
		{
			if (p_custom_plt) delete p_custom_plt;
			p_custom_plt = new_custom_plt;
		}
		inline void set_xdata(PC_PART_XDATA& xdata)
		{
			if (p_xdata) delete p_xdata;
			p_xdata = new PC_PART_XDATA(xdata);
		}
		inline PC_PART_XDATA* get_xdata()
		{
			return p_xdata;
		}
		inline CUSTOM_PLT* get_custom_plt()
		{
			return p_custom_plt;
		}
		inline std::vector<CUSTOM_TGA*>* get_custom_tgas()
		{
			return p_custom_tgas;
		}
		inline std::vector<CUSTOM_TGA*>* get_allocated_custom_tgas()
		{
			if (!p_custom_tgas)
			{
				p_custom_tgas = new std::vector<CUSTOM_TGA*>;
			}
			return p_custom_tgas;
		}

		GobExtra() : p_custom_plt(NULL), p_xdata(NULL), p_custom_tgas(NULL) {}
		~GobExtra()
		{
			if (p_custom_plt) delete p_custom_plt;
			if (p_custom_tgas)
			{
				for (auto iter=p_custom_tgas->begin(); iter!=p_custom_tgas->end(); iter++)
				{
					delete *iter;
				}
				delete p_custom_tgas;
			}
			if (p_xdata) delete p_xdata;
		}
	};
	class ExtendedGob
	{
	public:
		Gob base;
		GobExtra extra;
	};
#pragma pack(pop)

	struct CREATURE_EXTRA
	{
		uint16_t chest_model;
		uint16_t pelvis_model;
		uint32_t attached_by;
		CREATURE_EXTRA_ATTACHED* attached_to;
		int flags;
		PC_PARTS_CUSTOM_PLT custom_pc_parts_plt;
		PC_PART_XDATA body_xdata;
		ITEM_PARTS_XDATA left_hand_xdata;
		ITEM_PARTS_XDATA right_hand_xdata;
		CREATURE_EXTRA():	chest_model(0),
							pelvis_model(0),
							attached_by(OBJECT_INVALID),
							attached_to(NULL),
							flags(0)
		{
			//constructor
		}
		inline void DetachAttachedTo()
		{
			if (attached_to)
			{
				get_creature_extra(attached_to->target_id)->attached_by = OBJECT_INVALID;

				delete attached_to;
				attached_to = NULL;
			}
		}
		inline void DetachAttachedBy()
		{
			if (attached_by != OBJECT_INVALID)
			{
				CREATURE_EXTRA* creature_extra_attached_by = get_creature_extra(attached_by);
				if (creature_extra_attached_by->attached_to)
				{
					delete creature_extra_attached_by->attached_to;
					creature_extra_attached_by->attached_to = NULL;
				}
				attached_by = OBJECT_INVALID;
			}
		}
		~CREATURE_EXTRA()
		{
			DetachAttachedTo();
			DetachAttachedBy();
		}
	};

	const uint8_t color_channels_texture_index[NUM_PLT_COLOR_CHANNELS] = {0,1,2,3,4,4,5,5,6,6};

	char log_filename[] = "./nwncx_sinfar.txt";
	char ini_filename[] = "./sinfarx.ini";
	const char* INI_SECTION = "NWNCXSinfar";
	const char* INI_KEY_DM_PARTY = "ShowDMParty";
	const char* INI_KEY_JOINED_MSG = "ShowJoinedMessage";
	const char* INI_KEY_PLAYER_LIST = "ShowAllPlayersInPlayerList";
	const char* INI_KEY_PLAY_INNACTIVE_ANIMS = "PlayInnactiveAnimations";
	const char* INI_KEY_DISABLE_ATI_SHADERS = "DisableAtiShaders";
	const char* INI_LOG_LEVEL = "LogLevel";
	const char* INI_RESMAN_CACHE_SIZE = "ResourcesManagerCacheSize";
	const char* INI_RESMAN_LOG_REQUESTED_RESOURCES = "LogRequestedResources";
	const char* INI_RESMAN_LOG_MISSING_RESOURCES = "LogMissingResources";
	const char* INI_RESMAN_USE_NWNDATA_FOLDER = "UseSinfarXNWNDataFolder";
	int ini_dm_party = 1;
	int ini_show_joined_msg = 1;
	int ini_show_all_players_in_player_list = 1;
	int ini_play_innactive_anim = 1;
	int ini_log_level = 1;
	int ini_resman_cache_size = 300;
	int ini_resman_log_requested_resources = 0;
	int ini_resman_log_missing_resources = 0;
	int ini_resman_use_nwndata_folder = 0;
	int ini_disable_ati_shaders = 0;
	uint32_t resman_cache_size = ini_resman_cache_size*1024*1024;

	//camera settings
	int ini_enable_camera_hack = true;
	float ini_cam_min_zoom = 0.5;
	float ini_cam_max_zoom = 35.0;
	float ini_cam_min_pitch = 1.0;
	float ini_cam_max_pitch = 180;
	float ini_cam_default_min_zoom = 1.0;
	float ini_cam_default_max_zoom = 25.0;

	std::string format_float_for_ini(float f)
	{
		char val[30];
		sprintf(val, "%.2f", f);
		return val;
	}

	void nwncx_sinfar_init(void (*load_extra_ini_settings)(CIniFileA&))
	{
		aurora::log_printf = nwncx::sinfar::log_printf;
		aurora::print_stacktrace = print_stacktrace;

		CIniFileA ini;
		ini.Load(ini_filename);
		CIniSectionA* ini_section = ini.GetSection(INI_SECTION);
		if (ini_section)
		{
			ini_dm_party = atoi(ini_section->GetKeyValue(INI_KEY_DM_PARTY, "1").c_str());
			ini_show_joined_msg = atoi(ini_section->GetKeyValue(INI_KEY_JOINED_MSG, "1").c_str());
			ini_show_all_players_in_player_list = atoi(ini_section->GetKeyValue(INI_KEY_PLAYER_LIST, "1").c_str());
			ini_play_innactive_anim = atoi(ini_section->GetKeyValue(INI_KEY_PLAY_INNACTIVE_ANIMS, "1").c_str());
			ini_disable_ati_shaders = atoi(ini_section->GetKeyValue(INI_KEY_DISABLE_ATI_SHADERS, "0").c_str());
			ini_log_level = atoi(ini_section->GetKeyValue(INI_LOG_LEVEL, "1").c_str());
			ini_resman_cache_size = atoi(ini_section->GetKeyValue(INI_RESMAN_CACHE_SIZE, "300").c_str());
			ini_resman_log_requested_resources = atoi(ini_section->GetKeyValue(INI_RESMAN_LOG_REQUESTED_RESOURCES, "0").c_str());
			ini_resman_log_missing_resources = atoi(ini_section->GetKeyValue(INI_RESMAN_LOG_MISSING_RESOURCES, "0").c_str());
			ini_resman_use_nwndata_folder = atoi(ini_section->GetKeyValue(INI_RESMAN_USE_NWNDATA_FOLDER, "0").c_str());
		}
		resman_cache_size = ini_resman_cache_size*1024*1024;
		ini.SetKeyValue(INI_SECTION, INI_KEY_DM_PARTY, std::to_string(int64_t(ini_dm_party)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_KEY_JOINED_MSG, std::to_string(int64_t(ini_show_joined_msg)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_KEY_PLAYER_LIST, std::to_string(int64_t(ini_show_all_players_in_player_list)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_KEY_PLAY_INNACTIVE_ANIMS, std::to_string(int64_t(ini_play_innactive_anim)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_KEY_DISABLE_ATI_SHADERS, std::to_string(int64_t(ini_disable_ati_shaders)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_LOG_LEVEL, std::to_string(int64_t(ini_log_level)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_RESMAN_CACHE_SIZE, std::to_string(int64_t(ini_resman_cache_size)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_RESMAN_LOG_REQUESTED_RESOURCES, std::to_string(int64_t(ini_resman_log_requested_resources)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_RESMAN_LOG_MISSING_RESOURCES, std::to_string(int64_t(ini_resman_log_missing_resources)).c_str());
		ini.SetKeyValue(INI_SECTION, INI_RESMAN_USE_NWNDATA_FOLDER, std::to_string(int64_t(ini_resman_use_nwndata_folder)).c_str());

		//camera settings
		const char* INI_SECTION_CAMERA = "Camera";
		const char* INI_KEY_CAM_ENABLE_CAMERA_HACK = "Enable";
		const char* INI_KEY_CAM_MIN_ZOOM = "MinZoom";
		const char* INI_KEY_CAM_MAX_ZOOM = "MaxZoom";
		const char* INI_KEY_CAM_MIN_PITCH = "MinPitch";
		const char* INI_KEY_CAM_MAX_PITCH = "MaxPitch";
		const char* INI_KEY_CAM_DEFAULT_MIN_ZOOM = "DefaultMinZoom";
		const char* INI_KEY_CAM_DEFAULT_MAX_ZOOM = "DefaultMaxZoom";
		CIniSectionA* camera_ini_section = ini.GetSection(INI_SECTION_CAMERA);
		if (camera_ini_section)
		{
			ini_enable_camera_hack = atoi(camera_ini_section->GetKeyValue(INI_KEY_CAM_ENABLE_CAMERA_HACK, "1").c_str());
			ini_cam_default_min_zoom = (float)atof(camera_ini_section->GetKeyValue(INI_KEY_CAM_DEFAULT_MIN_ZOOM, "1.0").c_str());
			ini_cam_default_max_zoom = (float)atof(camera_ini_section->GetKeyValue(INI_KEY_CAM_DEFAULT_MAX_ZOOM, "25.0").c_str());
			ini_cam_min_zoom = (float)atof(camera_ini_section->GetKeyValue(INI_KEY_CAM_MIN_ZOOM, "0.5").c_str());
			ini_cam_max_zoom = (float)atof(camera_ini_section->GetKeyValue(INI_KEY_CAM_MAX_ZOOM, "35.0").c_str());
			ini_cam_min_pitch = (float)atof(camera_ini_section->GetKeyValue(INI_KEY_CAM_MIN_PITCH, "1.0").c_str());
			ini_cam_max_pitch = (float)atof(camera_ini_section->GetKeyValue(INI_KEY_CAM_MAX_PITCH, "180.0").c_str());
		}
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_ENABLE_CAMERA_HACK, std::to_string(int64_t(ini_enable_camera_hack)).c_str());
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_DEFAULT_MIN_ZOOM, format_float_for_ini(ini_cam_default_min_zoom).c_str());
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_DEFAULT_MAX_ZOOM, format_float_for_ini(ini_cam_default_max_zoom).c_str());
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_MIN_ZOOM, format_float_for_ini(ini_cam_min_zoom).c_str());
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_MAX_ZOOM, format_float_for_ini(ini_cam_max_zoom).c_str());
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_MIN_PITCH, format_float_for_ini(ini_cam_min_pitch).c_str());
		ini.SetKeyValue(INI_SECTION_CAMERA, INI_KEY_CAM_MAX_PITCH, format_float_for_ini(ini_cam_max_pitch).c_str());

		if (load_extra_ini_settings) load_extra_ini_settings(ini);

		ini.Save(ini_filename);
	}
	
	INLINE void log_printf(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		FILE* log_file = fopen(log_filename, "a");
		if (log_file)
		{
			vfprintf(log_file, format, args);
			fclose(log_file);
		}
		va_end(args);
	}

	INLINE void debug_log_printf(int min_log_level, const char* format, ...)
	{
		if (ini_log_level < min_log_level) return;

		va_list args;
		va_start(args, format);
		FILE* log_file = fopen(log_filename, "a");
		if (log_file)
		{
			vfprintf(log_file, format, args);
			fclose(log_file);
		}
		va_end(args);
	}
	
	int is_on_sinfar = 0;
	uint32_t server_ip = 0xFFFFFFFF;
	bool testing = false;	

#ifdef TESTING
	bool NWNCXSinfar_DebugMe_called = false;
	const char* NWNCXSinfar_DebugMe(const char* info){ NWNCXSinfar_DebugMe_called = true; return info; }
#endif

	uint8_t crypt_key[2] = {0xFE,0xAB};
	uint8_t crypt_key2[3] = {0x98,0x23,0xB7};
	char my_decrypt_buffer[10000];
	inline const char* my_decrypt(const uint8_t to_decrypt[], size_t to_decrypt_len, const uint8_t crypt_key[], size_t crypt_key_len)
	{
		size_t crypt_key_index = 0;
		for (size_t to_decrypt_index=0; to_decrypt_index<to_decrypt_len; to_decrypt_index++)
		{
			my_decrypt_buffer[to_decrypt_index] = to_decrypt[to_decrypt_index] ^ crypt_key[crypt_key_index];
			crypt_key_index++; if (crypt_key_index >= crypt_key_len) crypt_key_index = 0;
		}
		my_decrypt_buffer[to_decrypt_len] = 0;
		return my_decrypt_buffer;
	}

	bool extra_servers_loaded = false;
	std::set<uint32_t> extra_servers_ip;
	inline void add_server_to_extra_servers_ip(const char* host_name)
	{
		hostent* host = gethostbyname(host_name);
		if (host)
		{
			extra_servers_ip.insert(*(uint32_t*)(host->h_addr_list[0]));
		}
	}
	int is_sinfar_server(uint32_t ip_data)
	{
		//log_printf("connecting to :%x\n", ip_data);
		if (ip_data == 0xC6450F40) 
			return 3;
		if (ip_data == 0x0100007F /*localhost*/ || ip_data == 0x1337d30a /*ubundu desktop vm*/)
			return 2;
		else
		{
			if (!extra_servers_loaded)
			{
#ifdef WIN32
				WSADATA wsaData;
				WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
				uint8_t Susurros_Hirientes[] = {0xF9,0x51,0xC3,0xF0,0x46,0xD9,0xF9,0x0D,0xC7,0xEA,0x4C,0xDD,0xFD,0x40,0xC3,0xFC,0x46,0xDA,0xF1,0x56,0xC5,0xFF,0x46,0x99,0xF6,0x46,0xC3};
				add_server_to_extra_servers_ip(MY_DECRYPT(Susurros_Hirientes, crypt_key2));
				uint8_t SilmPW[] = { 0xEB, 0x4A, 0xDB, 0xF5, 0x0D, 0xC7, 0xEF };
				add_server_to_extra_servers_ip(MY_DECRYPT(SilmPW, crypt_key2));
				uint8_t SilmNWNPW[] = {0xF6,0x54,0xD9,0xB6,0x50,0xDE,0xF4,0x4E,0x99,0xE8,0x54};
				add_server_to_extra_servers_ip(MY_DECRYPT(SilmNWNPW, crypt_key2));
				extra_servers_loaded = true;
#ifdef WIN32
				WSACleanup();
#endif
			}
			return (extra_servers_ip.find(ip_data) != extra_servers_ip.end() ? 1 : 0);
		}
	}
	
	CREATURES_EXTRA creatures_extra;
	CREATURE_EXTRA* get_creature_extra(uint32_t creature_id)
	{
		unordered_map<uint32_t, CREATURE_EXTRA*>::iterator iter = creatures_extra.find(creature_id);
		if (iter == creatures_extra.end())
		{
			CREATURE_EXTRA* creature_extra = new CREATURE_EXTRA;
			creatures_extra[creature_id] = creature_extra;
			return creature_extra;
		}
		return iter->second;
	}
	
	void update_creature_extra(CNWCCreature* creature)
	{
		Gob* creature_gob = get_creature_gob(creature);
		if (creature_gob)
		{
			set_gob_scale(creature_gob, get_creature_extra(creature->cre_id)->body_xdata.scaling);
		}
	}
	
	void on_destroy_creature(CNWCCreature* creature)
	{
		unordered_map<uint32_t, CREATURE_EXTRA*>::iterator iter = creatures_extra.find(creature->cre_id);
		if (iter != creatures_extra.end())
		{
			delete iter->second;
			creatures_extra.erase(iter);
		}
	}

	PLACEABLES_EXTRA placeables_extra;
	PLACEABLE_EXTRA* get_placeable_extra(uint32_t plc_id)
	{
		unordered_map<uint32_t, PLACEABLE_EXTRA*>::iterator iter = placeables_extra.find(plc_id);
		if (iter == placeables_extra.end())
		{
			PLACEABLE_EXTRA* placeable_extra = new PLACEABLE_EXTRA;
			placeables_extra[plc_id] = placeable_extra;
			return placeable_extra;
		}
		return iter->second;
	}

	void (NWNCX_MEMBER_CALL *destroy_placeable_org)(CNWCPlaceable*) = (void (NWNCX_MEMBER_CALL *)(CNWCPlaceable*))
#ifdef WIN32
		0x591460;
#elif __linux__
		0x081B19B8;
#endif
	void NWNCX_MEMBER_CALL destroy_placeable_hook(CNWCPlaceable* plc)
	{
		unordered_map<uint32_t, PLACEABLE_EXTRA*>::iterator iter = placeables_extra.find(plc->plc_obj.obj_id);
		if (iter != placeables_extra.end())
		{
			delete iter->second;
			placeables_extra.erase(iter);
		}
		return destroy_placeable_org(plc);
	}

	std::deque<std::unique_ptr<DispatchedUpdate>> dispatched_updates;
	aurora::mutex dispatched_updates_mutex;
	void dispatch_update(std::unique_ptr<DispatchedUpdate> dispatched_update)
	{
		aurora::lock_guard lock(dispatched_updates_mutex);
		dispatched_updates.push_back(std::move(dispatched_update));
	}
	void on_render()
	{
		{
			aurora::lock_guard lock(dispatched_updates_mutex);

			while (dispatched_updates.size() > 0)
			{
				dispatched_updates.front()->perform();
				dispatched_updates.pop_front();
			}
		}

		for (CREATURES_EXTRA::iterator iter=creatures_extra.begin(); iter!=creatures_extra.end(); iter++)
		{
			CNWCCreature* creature = creature_by_id(iter->first);
			if (creature)
			{
				update_creature_extra(creature);
			}
		}
	}
	
	ExtendedGob* last_created_gob = NULL;
	void* alloc_gob_hook(uint32_t size)
	{
		/*ExtendedGob* result = new(nwn_alloc(sizeof(ExtendedGob))) ExtendedGob;
		if (!last_created_gob) last_created_gob = result;
		return result;*/
		last_created_gob = new(nwn_alloc(sizeof(ExtendedGob))) ExtendedGob;
		return last_created_gob;
	}
	void* (NWNCX_MEMBER_CALL *destroy_caurobject_org)(ExtendedGob*, NWNCX_MEMBER_X_PARAM int) = (void* (NWNCX_MEMBER_CALL *)(ExtendedGob*, NWNCX_MEMBER_X_PARAM int))0x007A9670;
	void* NWNCX_MEMBER_CALL destroy_caurobject_hook(ExtendedGob* extended_gob, NWNCX_MEMBER_X_PARAM int p2)
	{
		if (p2 & 1)
		{
			extended_gob->extra.~GobExtra();
		}
		return destroy_caurobject_org(extended_gob, NWNCX_MEMBER_X_PARAM_VAL p2);
	}

	#define NWNCX_SINFAR_PLC_HAS_XDATA 0x40000000
	PC_PART_XDATA plc_xdata;
	uint32_t NWNCX_MEMBER_CALL read_placeable_id(CNWMessage* message)
	{
		plc_xdata.clear();
		uint32_t plc_id = read_msg_obj_id(message);
		if (plc_id & NWNCX_SINFAR_PLC_HAS_XDATA && plc_id != OBJECT_INVALID)
		{
			plc_id &= ~NWNCX_SINFAR_PLC_HAS_XDATA;
			plc_xdata.read_pc_part_xdata((const char*&)message);
		}
		return plc_id;
	}

	uint32_t creature_update_appearance_id = OBJECT_INVALID;
	uint32_t read_creature_appearance_obj_id(CNWMessage* message)
	{
		creature_update_appearance_id = read_msg_obj_id(message);
		if (is_on_sinfar)
		{
			CNWCCreature* creature = creature_by_id(creature_update_appearance_id);
			CREATURE_EXTRA* creature_extra = get_creature_extra(creature_update_appearance_id);
			uint8_t extra_data = read_msg_byte(message);

			//do not update the head (used by both the xchest and head)
#ifdef WIN32
			uint16_t* force_update_body_part_jmp = (uint16_t*)0x00449409;
			*force_update_body_part_jmp = 0x0574;
#elif __linux__
			uint16_t* force_update_body_part_jmp = (uint16_t*)0x08153831;
			*force_update_body_part_jmp = 0x0774;
#endif
			//do not update wings,tail and cloak...
			//cloak
			#ifdef WIN32
				*(uint16_t*)0x00542D33 = 0x840F;
				*(uint32_t*)(0x00542D33+2) = 0x000000C0;
				*(uint16_t*)0x00542DF2 = 0x0574;
			#elif __linux__
				*(uint16_t*)0x0829153B = 0x840F;
				*(uint32_t*)(0x0829153B+2) = 0x000000B7;
				*(uint16_t*)0x082915F1 = 0x0574;
			#endif
			//wings
			#ifdef WIN32
				*(uint16_t*)0x0044942B = 0x0574;
				*(uint16_t*)0x00542237 = 0x840F;
				*(uint32_t*)(0x00542237+2) = 0x000002BF;
			#elif __linux__
				*(uint16_t*)0x0815384F = 0x8774;
				*(uint8_t*)0x082907B7 = 0x75;
			#endif
			//tail
			#ifdef WIN32
				*(uint16_t*)0x0044941A = 0x0574;
				*(uint16_t*)0x00541C77 = 0x840F;
				*(uint32_t*)(0x00541C77+2) = 0x000002BF;
			#elif __linux__
				*(uint16_t*)0x0815384F = 0x8774;
				*(uint8_t*)0x0828FF8B = 0x75;
			#endif

			//body xdata
			if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_BODY)
			{
				PC_PART_XDATA new_body_xdata;
				new_body_xdata.read_pc_part_xdata((const char*&)message);
				bool orientation_changed = (
					new_body_xdata.extra_rotate_x != creature_extra->body_xdata.extra_rotate_x || 
					new_body_xdata.extra_rotate_y != creature_extra->body_xdata.extra_rotate_y || 
					new_body_xdata.extra_rotate_z != creature_extra->body_xdata.extra_rotate_z);
				creature_extra->body_xdata = new_body_xdata;
				if (creature)
				{
					if (orientation_changed && is_stationary_animation(creature, creature->animation))
					{
						uint16_t saved_animation = creature->animation;
						creature->animation = -1;
						update_creature_animation(creature);
						creature->animation = saved_animation;
					}
					update_creature_extra(creature);
				}
			}

			//armor parts xdata
			if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_ARMOR)
			{
				uint16_t chest_model = 0;
				uint16_t pelvis_model = 0;
				PC_PARTS_CUSTOM_PLT pc_parts_xdata;
				uint8_t extra_amor_data = read_msg_byte(message);
				if (extra_amor_data & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XCHEST)
				{
					chest_model = read_msg_word(message);
				}
				if (extra_amor_data & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XPELVIS)
				{
					pelvis_model = read_msg_word(message);
				}
				pc_parts_xdata.read_pc_parts_xdata(extra_amor_data, (const char*)message);
				if (creature_extra->chest_model != chest_model)
				{
					*force_update_body_part_jmp = 0x9090;
					creature_extra->chest_model = chest_model;
				}
				if (creature_extra->pelvis_model != pelvis_model)
				{
					*force_update_body_part_jmp = 0x9090;
					creature_extra->pelvis_model = pelvis_model;
				}
				int update_result = creature_extra->custom_pc_parts_plt.update(pc_parts_xdata);
				if (update_result)
				{
					*force_update_body_part_jmp = 0x9090;
					//cloak
					if (update_result & (1<<20))
					{
					#ifdef WIN32
						*(uint16_t*)0x00542D33 = 0x9090;
						*(uint32_t*)(0x00542D33+2) = 0x90909090;
						*(uint16_t*)0x00542DF2 = 0x9090;
					#elif __linux__
						*(uint16_t*)0x0829153B = 0x9090;
						*(uint32_t*)(0x0829153B+2) = 0x90909090;
						*(uint16_t*)0x082915F1 = 0x9090;
					#endif
					}
					//wings
					if (update_result & (1<<21))
					{
					#ifdef WIN32
						*(uint16_t*)0x0044942B = 0x9090;
						*(uint16_t*)0x00542237 = 0x9090;
						*(uint32_t*)(0x00542237+2) = 0x90909090;
					#elif __linux__
						*(uint16_t*)0x0815384F = 0x9090;
						*(uint8_t*)0x082907B7 = 0xEB;		
					#endif
					}
					//tail
					if (update_result & (1<<22))
					{
					#ifdef WIN32
						*(uint16_t*)0x0044941A = 0x9090;
						*(uint16_t*)0x00541C77 = 0x9090;
						*(uint32_t*)(0x00541C77+2) = 0x90909090;
					#elif __linux__
						*(uint16_t*)0x0815384F = 0x9090;
						*(uint8_t*)0x0828FF8B = 0xEB;
					#endif
					}
				}
			}

			//attached by
			creature_extra->DetachAttachedTo();
			if (extra_data & NWNCX_SINFAR_CREATURE_EXTRA_ATTACH)
			{
				CREATURE_EXTRA_ATTACHED* attach_data = new CREATURE_EXTRA_ATTACHED;
				attach_data->target_id = read_msg_obj_id(message);
				attach_data->attach_x = read_msg_float(message);
				attach_data->attach_y = read_msg_float(message);
				attach_data->attach_z = read_msg_float(message);
				creature_extra->attached_to = attach_data;

				CREATURE_EXTRA* creature_extra_attached_by = get_creature_extra(attach_data->target_id);
				creature_extra_attached_by->DetachAttachedBy();
				creature_extra_attached_by->attached_by = creature_update_appearance_id;
			}

			//custom weapon
			if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_LHAND)
			{
				ITEM_PARTS_XDATA item_parts_xdata;
				item_parts_xdata.read_item_parts_xdata((const char*)message);
				creature_extra->left_hand_xdata = item_parts_xdata;
			}
			if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_RHAND)
			{
				ITEM_PARTS_XDATA item_parts_xdata;
				item_parts_xdata.read_item_parts_xdata((const char*)message);
				creature_extra->right_hand_xdata = item_parts_xdata;
			}
		}
		return creature_update_appearance_id;
	}

	inline const char* get_part_name_from_body_part_index(int body_part)
	{
		switch (body_part)
		{
		case 0: return  "p???*_footr???";
		case 1: return  "p???*_footl???";
		case 2: return  "p???*_shinr???";
		case 3: return  "p???*_shinl???";
		case 4: return  "p???*_legl???";
		case 5: return  "p???*_legr???";
		case 6: return  "p???*_*pelvis?*";
		case 7: return  "p???*_*chest?*";
		case 8: return  "p???*_belt???";
		case 9: return  "p???*_neck???";
		case 10: return "p???*_forer???";
		case 11: return "p???*_forel???";
		case 12: return "p???*_bicepr???";
		case 13: return "p???*_bicepl???";
		case 14: return "p???*_shor???";
		case 15: return "p???*_shol???";
		case 16: return "p???*_handr???";
		case 17: return "p???*_handl???";
		default: return NULL;
		}
	}

	void update_aur_part_from_pc_part_xdata(CAurPart* part, PC_PART_XDATA& pc_part_xdata)
	{
		if (part)
		{
			part->scale = pc_part_xdata.scaling;
			part->position.x = pc_part_xdata.extra_x;
			part->position.y = pc_part_xdata.extra_y;
			part->position.z = pc_part_xdata.extra_z;
			Vector euler = quaternion_to_vector(part->orientation);			
			euler.x = static_cast<float>(3.141593 + pc_part_xdata.extra_rotate_x);
			euler.y = pc_part_xdata.extra_rotate_y;
			euler.z = pc_part_xdata.extra_rotate_z;
			part->orientation = vector_to_quaternion(euler);
		}
	}

	std::vector<std::vector<CUSTOM_TGA*>> area_custom_tgas;
	std::vector<CUSTOM_TGA*>* current_plc_custom_tgas = NULL;
	bool creating_static_plc = false;
	int (NWNCX_MEMBER_CALL *add_static_placeable_to_area_org)(void*, NWNCX_MEMBER_X_PARAM uint32_t, uint16_t, Vector, Vector) = (int (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM uint32_t, uint16_t, Vector, Vector))0x0048DAA0;
	int NWNCX_MEMBER_CALL add_static_placeable_to_area_hook(void* area, NWNCX_MEMBER_X_PARAM uint32_t p2, uint16_t p3, Vector p4, Vector p5)
	{
		last_created_gob = NULL;
		creating_static_plc = true;
		if (current_plc_custom_tgas == NULL)
		{
			area_custom_tgas.push_back(std::vector<CUSTOM_TGA*>());
			current_plc_custom_tgas = &(area_custom_tgas.back());
		}
		int ret = add_static_placeable_to_area_org(area, NWNCX_MEMBER_X_PARAM_VAL p2, p3, p4, p5);
		if (!current_plc_custom_tgas->empty())
		{
			current_plc_custom_tgas = NULL;
		}
		creating_static_plc = false;
		return ret;
	}

	void* (NWNCX_MEMBER_CALL *spawn_tile_org)(void*, NWNCX_MEMBER_X_PARAM char*, Vector*, int, int, Quaternion*) = (void* (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM char*, Vector*, int, int, Quaternion*))0x0079E510;
	void* NWNCX_MEMBER_CALL spawn_tile_hook(void* scene, NWNCX_MEMBER_X_PARAM char* p2, Vector* position, int p4, int p5, Quaternion* orientation)
	{
		if (creating_static_plc)
		{
			Vector euler = quaternion_to_vector(*orientation);			
			euler.y += plc_xdata.extra_rotate_y;
			euler.z += plc_xdata.extra_rotate_z;
			*orientation = vector_to_quaternion(euler);
		}
		return spawn_tile_org(scene, NWNCX_MEMBER_X_PARAM_VAL p2, position, p4, p5, orientation);
	}

	int (NWNCX_MEMBER_CALL *load_area_org)(void*, NWNCX_MEMBER_X_PARAM void* aur_camera, void* gui_area_load_screen) = (int (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM void*, void*))0x00486510;
	int NWNCX_MEMBER_CALL load_area_hook(void* area, NWNCX_MEMBER_X_PARAM void* aur_camera, void* gui_area_load_screen)
	{
		for (auto iter=area_custom_tgas.begin(); iter!=area_custom_tgas.end(); iter++)
		{
			for (auto iter2=iter->begin(); iter2!=iter->end(); iter2++)
			{
				delete *iter2;
			}
		}
		area_custom_tgas.clear();
		return load_area_org(area, NWNCX_MEMBER_X_PARAM_VAL aur_camera, gui_area_load_screen);
	}

	CNWCCreature* set_creature_appearance = NULL;
	CREATURE_EXTRA* set_creature_appearance_extra = NULL;
	uint8_t last_appearance_update_phenotype = 0;
	uint32_t last_appearance_update_cloak_id = OBJECT_INVALID;
	int(NWNCX_MEMBER_CALL *set_creature_appearance_info_org)(CNWCCreature*, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo*, uint8_t) = (int (NWNCX_MEMBER_CALL *)(CNWCCreature*, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo*, uint8_t))0x004C5FE0;
	int NWNCX_MEMBER_CALL set_creature_appearance_info_hook(CNWCCreature* creature, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo* appearance_info, uint8_t p3)
	{
		if (set_creature_appearance != NULL)
		{
			return set_creature_appearance_info_org(creature, NWNCX_MEMBER_X_PARAM_VAL appearance_info, p3);
		}

		//pre
		set_creature_appearance = creature;
		last_appearance_update_cloak_id = get_item_in_slot(creature, NWNCX_MEMBER_X_PARAM_VAL(1 << 6));
		set_creature_appearance_extra = get_creature_extra(creature->cre_id);
		last_appearance_update_phenotype = appearance_info->phenotype;
		set_creature_appearance_extra->flags |= CREATURE_EXTRA_FLAG_APPPEARANCE_SET;

		int ret = set_creature_appearance_info_org(creature, NWNCX_MEMBER_X_PARAM_VAL appearance_info, p3);

		//post
		if (is_on_sinfar)
		{
			Gob* creature_gob = get_creature_gob(creature);
			if (creature_gob && creature_gob->root_part)
			{
				for (int i=0; i<18; i++)
				{
					PC_PART_XDATA& pc_part_xdata = set_creature_appearance_extra->custom_pc_parts_plt.parts[i];
					const char* part_name = get_part_name_from_body_part_index(i);
					if (part_name)
					{
						CAurPart* part = find_part(creature_gob->root_part, part_name);
						update_aur_part_from_pc_part_xdata(part, pc_part_xdata);
					}
				}
				Gob* robe_gob = find_gob(creature_gob, "robe*");
				if (robe_gob)
				{
					PC_PART_XDATA& pc_part_xdata = set_creature_appearance_extra->custom_pc_parts_plt.parts[18];
					CAurPart* part = find_part(robe_gob->root_part, "p???*_robe???");
					update_aur_part_from_pc_part_xdata(part, pc_part_xdata);
				}
				Gob* cloak_gob = find_gob(creature_gob, "cloak*");
				if (cloak_gob)
				{
					PC_PART_XDATA& pc_part_xdata = set_creature_appearance_extra->custom_pc_parts_plt.parts[20];
					update_aur_part_from_pc_part_xdata(cloak_gob->root_part, pc_part_xdata);
				}
			}
			
			update_creature_extra(set_creature_appearance);
		}

		//post
		last_appearance_update_phenotype = 0;
		set_creature_appearance_extra = NULL;
		set_creature_appearance = NULL;

		return ret;
	}

	void(NWNCX_MEMBER_CALL *force_update_creature_appearance_org)(CNWCCreature*) = (void(NWNCX_MEMBER_CALL *)(CNWCCreature*))0x004CE8F0;
	void NWNCX_MEMBER_CALL force_update_creature_appearance_hook(CNWCCreature* creature)
	{
		if (get_creature_extra(creature->cre_id)->flags & CREATURE_EXTRA_FLAG_APPPEARANCE_SET)
		{
			return force_update_creature_appearance_org(creature);
		}
		else
		{
#ifdef WIN32
			*(uint32_t*)0x005435BB = 0x90909090;
			*(uint16_t*)(0x005435BB + 2) = 0x9090;
#elif __linux__
			*(uint8_t*)0x08292391 = 0xEB;
#endif
			force_update_creature_appearance_org(creature);
#ifdef WIN32
			*(uint32_t*)0x005435BB = 0x038B840F;
			*(uint16_t*)(0x005435BB + 2) = 0x0000;
#elif __linux__
			*(uint8_t*)0x08292391 = 0x75;
#endif
		}
	}

	#define NWNCX_SINFAR_VFX_ADJUSTED 0x8000
	PC_PART_XDATA vfx_xdata;
	uint16_t NWNCX_MEMBER_CALL on_update_vfx_read_vfx(CNWMessage* message, NWNCX_MEMBER_X_PARAM int num_bits)
	{
		vfx_xdata.clear();
		uint16_t vfx = read_msg_word(message);
		if (vfx & NWNCX_SINFAR_VFX_ADJUSTED)
		{
			vfx &= ~NWNCX_SINFAR_VFX_ADJUSTED;
			vfx_xdata.read_pc_part_xdata((const char*&)message);
		}
		return vfx;
	}

	bool update_plt_colors_with_xdata(PC_PART_XDATA& part_xdata, int num_colors, uint16_t* current_colors, uint16_t* updated_colors)
	{
		//set the colors index considering the mapped colors and overriden colors
		if (num_colors == NUM_PLT_COLOR_CHANNELS)
		{
			uint32_t palette_height_per_channel = *paletteheight / 7;
			for (int channel_index=0; channel_index<NUM_PLT_COLOR_CHANNELS; channel_index++)
			{
				int effective_color_index;
				if (part_xdata.channels_color_override & (1<<channel_index))
				{
					effective_color_index = part_xdata.channels_color[channel_index];
				}
				else
				{
					effective_color_index = current_colors[channel_index] - palette_height_per_channel*color_channels_texture_index[channel_index];
				}
				int effective_channel_index;
				if (part_xdata.channels_mappping[channel_index] != 0xFF)
				{
					effective_channel_index = part_xdata.channels_mappping[channel_index];
					//if color not overriden, use the color of that mapped channel
					if (!(part_xdata.channels_color_override & (1<<channel_index)))
					{
						effective_color_index = current_colors[effective_channel_index] - palette_height_per_channel*color_channels_texture_index[effective_channel_index];
					}
				}
				else
				{
					effective_channel_index = channel_index;
				}
				updated_colors[channel_index] = effective_color_index + (palette_height_per_channel*color_channels_texture_index[effective_channel_index]);
			}
			return true;
		}
		return false;
	}
	
	inline void update_last_created_vfx_with_xdata(ExtendedGob* vfx_gob)
	{
		if (vfx_xdata.has_data())
		{
			Vector position;
			Quaternion orientation;
			get_gob_part_position(&vfx_gob->base, (char*)"root", &position, &orientation);
			position.x += vfx_xdata.extra_x;
			position.y += vfx_xdata.extra_y;
			position.z += vfx_xdata.extra_z;
			Vector euler = quaternion_to_vector(orientation);
			euler.x += vfx_xdata.extra_rotate_x;
			euler.y += vfx_xdata.extra_rotate_y;
			euler.z += vfx_xdata.extra_rotate_z;
			orientation = vector_to_quaternion(euler);
			set_gob_part_position(&vfx_gob->base, (char*)"root", position, orientation);
			set_gob_scale(&vfx_gob->base, vfx_xdata.scaling);

			vfx_gob->extra.set_xdata(vfx_xdata);
		}
	}

	int (NWNCX_MEMBER_CALL *replace_texture_on_gob)(Gob*, NWNCX_MEMBER_X_PARAM char*, char*, int, uint16_t*, int) = (int (NWNCX_MEMBER_CALL *)(Gob*, NWNCX_MEMBER_X_PARAM char*, char*, int, uint16_t*, int))0x007AD200;
	int (NWNCX_MEMBER_CALL *replace_texture_on_gob_sub_tree)(Gob*, NWNCX_MEMBER_X_PARAM char*, char*, int, uint16_t*) = (int (NWNCX_MEMBER_CALL *)(Gob*, NWNCX_MEMBER_X_PARAM char*, char*, int, uint16_t*))
#ifdef WIN32
	0x007AD5E0;
#elif __linux__
	0x084E334C;
#endif
	int creating_vfx = 0;
	struct CREATING_VFX_SCOPE
	{
		CREATING_VFX_SCOPE()
		{
			last_created_gob = NULL;
			creating_vfx++;
		}
		~CREATING_VFX_SCOPE()
		{
			creating_vfx--;
		}
	};
	void update_last_created_vfx_on_object_gob(CNWCObject* object, ExtendedGob* vfx_gob)
	{
		update_last_created_vfx_with_xdata(vfx_gob);
		//apply the plt texture if available
		if (object->obj_type == 5/*OBJECT_TYPE_CREATURE*/)
		{
			CNWCCreature* creature = (CNWCCreature*)object;
			if (creature->appearance)
			{
				char* vfx_model = vfx_gob->base.root_part->mdl_node->name;
				CResRef vfx_model_resref;
				strncpy(vfx_model_resref.value, vfx_model, 16);
				if (resource_exists(vfx_model_resref, aurora::NwnResType_PLT))
				{
					uint16_t channels_color[NUM_PLT_COLOR_CHANNELS];
					memset(channels_color, 0, sizeof(channels_color));
					uint32_t palette_height_per_channel = *paletteheight / 7;
					channels_color[NwnPalette_Skin] = creature->appearance->skin_color;
					channels_color[NwnPalette_Hair] = creature->appearance->hair_color + palette_height_per_channel;
					channels_color[NwnPalette_Tattoo1] = creature->appearance->tattoo1_color + (palette_height_per_channel*6);
					channels_color[NwnPalette_Tattoo2] = creature->appearance->tattoo2_color + (palette_height_per_channel*6);
					uint16_t overriden_channels_color[NUM_PLT_COLOR_CHANNELS];
					if (update_plt_colors_with_xdata(vfx_xdata, NUM_PLT_COLOR_CHANNELS, channels_color, overriden_channels_color))
					{
						memcpy(channels_color, overriden_channels_color, sizeof(channels_color));
					}
					char* vfx_plt = vfx_model;
					CUSTOM_PLT* custom_plt = new CUSTOM_PLT;
					memcpy(dynamic_cast<PC_PART_XDATA*>(custom_plt), &vfx_xdata, sizeof(vfx_xdata)); 
					if (add_custom_resource(vfx_model_resref, *custom_plt))
					{
						vfx_plt = vfx_model_resref.value;
						vfx_gob->extra.set_custom_plt(custom_plt);
					}
					else
					{
						delete custom_plt;
					}
					replace_texture_on_gob_sub_tree(&vfx_gob->base, NWNCX_MEMBER_X_PARAM_VAL 
						vfx_model,
						vfx_plt,
						NUM_PLT_COLOR_CHANNELS,
						channels_color);
				}
			}
		}
	}
	int (NWNCX_MEMBER_CALL *add_visual_effect_to_object_org)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint16_t, int, uint32_t, uint8_t, uint8_t, Vector) = (int (NWNCX_MEMBER_CALL *)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint16_t, int, uint32_t, uint8_t, uint8_t, Vector))0x004919C0;
	int NWNCX_MEMBER_CALL add_visual_effect_to_object_hook(CNWCObject* object, NWNCX_MEMBER_X_PARAM uint16_t vfx, int p3, uint32_t p4, uint8_t p5, uint8_t p6, Vector p7)
	{
		int ret;
		{
			CREATING_VFX_SCOPE creating_vfx_scope;
			ret = add_visual_effect_to_object_org(object, NWNCX_MEMBER_X_PARAM_VAL vfx, p3, p4, p5, p6, p7);
		}
		if (ret && vfx_xdata.has_data())
		{
			CNWCVisualEffectOnObject* vfx_on_object = (CNWCVisualEffectOnObject*)object->vfx_list->last->data;
			if (vfx_on_object->ground_gob) update_last_created_vfx_on_object_gob(object, (ExtendedGob*)vfx_on_object->ground_gob);
			if (vfx_on_object->impact_gob) update_last_created_vfx_on_object_gob(object, (ExtendedGob*)vfx_on_object->impact_gob);
			if (vfx_on_object->head_gob) update_last_created_vfx_on_object_gob(object, (ExtendedGob*)vfx_on_object->head_gob);
			vfx_xdata.clear();
		}
		return ret;
	}
	bool gob_match_vfx_update_xdata(Gob* gob)
	{
		if (!gob) return true;
		PC_PART_XDATA* p_xdata = ((ExtendedGob*)gob)->extra.get_xdata();
		if (p_xdata)
		{
			return p_xdata->equals(vfx_xdata);
		}
		else
		{
			return !vfx_xdata.has_data();
		}
	}
	int (NWNCX_MEMBER_CALL *delete_visual_effect_to_object_org)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint16_t) = (int (NWNCX_MEMBER_CALL *)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint16_t))0x00491AF0;
	int NWNCX_MEMBER_CALL delete_visual_effect_to_object_hook(CNWCObject* object, NWNCX_MEMBER_X_PARAM uint16_t vfx)
	{
		CExoLinkedListNode* vfx_node = object->vfx_list->last;
		while (vfx_node)
		{
			CNWCVisualEffectOnObject* vfx_on_object = (CNWCVisualEffectOnObject*)vfx_node->data;
			if (vfx_on_object->vfx == vfx)
			{
				if (!gob_match_vfx_update_xdata(vfx_on_object->ground_gob) ||
					!gob_match_vfx_update_xdata(vfx_on_object->impact_gob) ||
					!gob_match_vfx_update_xdata(vfx_on_object->head_gob))
				{
					vfx_on_object->vfx |= NWNCX_SINFAR_VFX_ADJUSTED;
				}
			}
			vfx_node = vfx_node->prev;
		}
		int ret = delete_visual_effect_to_object_org(object, NWNCX_MEMBER_X_PARAM_VAL vfx);
		vfx_node = object->vfx_list->last;
		while (vfx_node)
		{
			CNWCVisualEffectOnObject* vfx_on_object = (CNWCVisualEffectOnObject*)vfx_node->data;
			vfx_on_object->vfx &= ~NWNCX_SINFAR_VFX_ADJUSTED;
			vfx_node = vfx_node->prev;
		}
		vfx_xdata.clear();
		return ret;
	}

	int (NWNCX_MEMBER_CALL *apply_fnf_anim_at_location_org)(void*, NWNCX_MEMBER_X_PARAM uint16_t, Vector) = (int (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM uint16_t, Vector))0x0048B360;
	int NWNCX_MEMBER_CALL apply_fnf_anim_at_location_hook(void* area, NWNCX_MEMBER_X_PARAM uint16_t vfx, Vector position)
	{
		int ret;
		{
			CREATING_VFX_SCOPE creating_vfx_scope;
			ret = apply_fnf_anim_at_location_org(area, NWNCX_MEMBER_X_PARAM_VAL vfx, position);
		}
		if (last_created_gob)
		{
			update_last_created_vfx_with_xdata(last_created_gob);
		}
		vfx_xdata.clear();
		return ret;
	}

	inline void update_emitter_hook(CAurPart* part, float p2, void (NWNCX_MEMBER_CALL *update_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float))
	{
		if (part->gob && ((ExtendedGob*)part->gob)->extra.get_xdata() && ((ExtendedGob*)part->gob)->extra.get_xdata()->has_rgba_mod())
		{
			vfx_xdata = *((ExtendedGob*)part->gob)->extra.get_xdata();
			{
				CREATING_VFX_SCOPE creating_vfx_scope;
				update_emitter_org(part, NWNCX_MEMBER_X_PARAM_VAL p2);
			}
			vfx_xdata.clear();
		}
		else
		{
			update_emitter_org(part, NWNCX_MEMBER_X_PARAM_VAL p2);
		}
	}

	void (NWNCX_MEMBER_CALL *update_fountain_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float) = (void (NWNCX_MEMBER_CALL *)(CAurPart* part, NWNCX_MEMBER_X_PARAM float))0x007E8080;
	void NWNCX_MEMBER_CALL update_fountain_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2)
	{
		update_emitter_hook(part, p2, update_fountain_emitter_org);
	}

	void (NWNCX_MEMBER_CALL *update_explosion_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float) = (void (NWNCX_MEMBER_CALL *)(CAurPart* part, NWNCX_MEMBER_X_PARAM float))0x007E8A00;
	void NWNCX_MEMBER_CALL update_explosion_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2)
	{
		update_emitter_hook(part, p2, update_explosion_emitter_org);
	}

	void (NWNCX_MEMBER_CALL *update_single_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float) = (void (NWNCX_MEMBER_CALL *)(CAurPart* part, NWNCX_MEMBER_X_PARAM float))0x007E8EE0;
	void NWNCX_MEMBER_CALL update_single_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2)
	{
		update_emitter_hook(part, p2, update_single_emitter_org);
	}

	void (NWNCX_MEMBER_CALL *update_lightning_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float) = (void (NWNCX_MEMBER_CALL *)(CAurPart* part, NWNCX_MEMBER_X_PARAM float))0x007E92E0;
	void NWNCX_MEMBER_CALL update_lightning_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2)
	{
		update_emitter_hook(part, p2, update_lightning_emitter_org);
	}

	inline std::string update_vfx_texture(std::vector<CUSTOM_TGA*>* custom_tgas, const char* texture, RGBA_MOD rgba_mod)
	{
		if (texture == NULL) return "";

		aurora::strtolower((char*)texture);

		for (auto iter=custom_tgas->begin(); iter!=custom_tgas->end(); iter++)
		{
			if (strncmp((*iter)->original_resref.value, texture, 16)==0)
			{
				char new_texture_resref[17];
				sprintf(new_texture_resref, "¬%u", (*iter)->get_custom_resource_id());
				return new_texture_resref;
			}
		}

		CResRef custom_tga_resref;
		strncpy(custom_tga_resref.value, texture, 16);
		CUSTOM_TGA* custom_tga = new CUSTOM_TGA(rgba_mod);
		if (add_custom_resource(custom_tga_resref, *custom_tga))
		{
			custom_tgas->push_back(custom_tga);
			char new_texture_resref[17];
			strncpy(new_texture_resref, custom_tga_resref.value, 16);
			new_texture_resref[16] = 0;
			return new_texture_resref;
		}
		else
		{
			delete custom_tga;
		}
		return texture;
	}

	void* (*get_texture_ref_org)(const char*, const char*) = (void* (*)(const char*, const char*))0x00786790;
	void* get_texture_ref_hook(const char* texture1, const char* texture2)
	{
		if (texture1)
		{
			PC_PART_XDATA* xdata = NULL;
			std::vector<CUSTOM_TGA*>* custom_tgas = NULL;
			if (last_created_gob && creating_vfx)
			{
				xdata = &vfx_xdata;
				custom_tgas = last_created_gob->extra.get_allocated_custom_tgas();
			}
			else if (creating_static_plc)
			{
				xdata = &plc_xdata;
				custom_tgas = current_plc_custom_tgas;
			}

			if (xdata && custom_tgas)
			{
				std::string texture1_override  = xdata->get_mapped_texture(texture1);
				if (xdata->has_rgba_mod())
				{
					texture1_override = update_vfx_texture(custom_tgas, texture1_override.c_str(), xdata->rgba_mod);
				}
				return get_texture_ref_org(texture1_override.c_str(), texture2);
			}
		}
		return get_texture_ref_org(texture1, texture2);
	}
	
	void* (*find_model_org)(char*) = (void* (*)(char*))0x007D8330; //initial value is only needed on Windows
	bool find_model_internal_call = false;
	void* find_model_internal(char* model_name)
	{
		find_model_internal_call = true;
		void* model = find_model_org(model_name);
		find_model_internal_call = false;
		return model;
	}
	void* find_model_hook(char* model_name)
	{
		if (find_model_internal_call) return find_model_org(model_name);

		if (set_creature_appearance)
		{
			int is_cloak = wildcmp("p???*_cloak_???", model_name);
			if (is_cloak && last_appearance_update_cloak_id == OBJECT_INVALID) return NULL;

			if (last_appearance_update_phenotype != 0)
			{
				if (wildcmp("p??0", model_name))
				{
					char gender, race;
					if (sscanf(model_name, "p%c%c0", &gender, &race)==2)
					{
						char real_pheno_model[17];
						sprintf(real_pheno_model, "p%c%c%u", gender, race, last_appearance_update_phenotype);
						void* model = find_model_internal(real_pheno_model);
						if (model != NULL) return model;
					}
				}
				else if (is_cloak)
				{
					void* model = find_model_internal(model_name);
					if (model != NULL) return model;

					char gender, race;
					int current_pheno, appr_index;
					if (sscanf(model_name, "p%c%c%d_cloak_%d", &gender, &race, &current_pheno, &appr_index)==4 && current_pheno != 0)
					{
						char default_model[17];
						sprintf(default_model, "p%c%c0_cloak_%03d", gender, race, appr_index);
						return find_model_org(default_model);
					}
				}
			}
		}
		return find_model_org(model_name);	
	}
	
	Vector (NWNCX_MEMBER_CALL *set_gob_position_org)(Gob*, NWNCX_MEMBER_X_PARAM Vector) = (Vector (NWNCX_MEMBER_CALL *)(Gob*, NWNCX_MEMBER_X_PARAM Vector))0x007B4540;
	Vector NWNCX_MEMBER_CALL set_gob_position_hook(Gob* gob, NWNCX_MEMBER_X_PARAM Vector v)
	{
		for (CREATURES_EXTRA::iterator iter=creatures_extra.begin(); iter!=creatures_extra.end(); iter++)
		{
			CNWCCreature* creature = creature_by_id(iter->first);
			if (!creature) continue;
			Gob* creature_gob = get_creature_gob(creature);
			if (creature_gob == gob)
			{
				CREATURE_EXTRA* creature_extra = iter->second;
				
				if (creature_extra->attached_to)
				{
					CNWCCreature* creature_attached_to = creature_by_id(creature_extra->attached_to->target_id);
					if (creature_attached_to)
					{
						v.x = creature_attached_to->position.x + creature_extra->attached_to->attach_x;
						v.y = creature_attached_to->position.y + creature_extra->attached_to->attach_y;
						v.z = creature_attached_to->position.z + creature_extra->attached_to->attach_z;
					}
				}
				if (creature_extra->attached_by != OBJECT_INVALID)
				{
					CNWCCreature* creature_attached_by = creature_by_id(creature_extra->attached_by);
					if (creature_attached_by)
					{
						Gob* gob_attached_by = get_creature_gob(creature_attached_by);
						if (gob_attached_by)
						{
							set_gob_position_hook(gob_attached_by, NWNCX_MEMBER_X_PARAM_VAL v);
						}
					}
				}

				v.x += creature_extra->body_xdata.extra_x;
				v.y += creature_extra->body_xdata.extra_y;
				v.z += creature_extra->body_xdata.extra_z;

				break;
			}
		}
		return set_gob_position_org(gob, NWNCX_MEMBER_X_PARAM_VAL v);
	}
	
	Quaternion (NWNCX_MEMBER_CALL *set_gob_orientation_org)(Gob*, NWNCX_MEMBER_X_PARAM Quaternion) = (Quaternion (NWNCX_MEMBER_CALL *)(Gob*, NWNCX_MEMBER_X_PARAM Quaternion))0x007AE5D0;
	Quaternion NWNCX_MEMBER_CALL set_gob_orientation_hook(Gob* gob, NWNCX_MEMBER_X_PARAM Quaternion orientation)
	{
		for (CREATURES_EXTRA::iterator iter=creatures_extra.begin(); iter!=creatures_extra.end(); iter++)
		{
			CNWCCreature* creature = creature_by_id(iter->first);
			if (!creature) continue;
			Gob* creature_gob = get_creature_gob(creature);
			if (creature_gob == gob)
			{
				Vector v = quaternion_to_vector(orientation);
				v.x += iter->second->body_xdata.extra_rotate_x;
				v.y += iter->second->body_xdata.extra_rotate_y;
				v.z += iter->second->body_xdata.extra_rotate_z;
				orientation = vector_to_quaternion(v);
				break;
			}
		}
		return set_gob_orientation_org(gob, NWNCX_MEMBER_X_PARAM_VAL orientation);
	}
	
	int (NWNCX_MEMBER_CALL *load_item_visual_effect_org)(void*, NWNCX_MEMBER_X_PARAM uint8_t) = (int (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM uint8_t))0x004EA070;
	int NWNCX_MEMBER_CALL load_item_visual_effect_hook(void* item, NWNCX_MEMBER_X_PARAM uint8_t vfx)
	{
		if (vfx > 9)
		{
			C2DA* two_dimension_array = get_cached_2da("iprp_visualfx");
			if (two_dimension_array)
			{
				CExoString column("ModelSuffix");
				CExoString value;
				get_2da_string(two_dimension_array, vfx-1, &column, &value);
				if (value.text)
				{
#ifdef WIN32
					uint32_t model_addr = 0x004EA0FB;
					uint32_t model_org_val = 0x009088D0;
#elif __linux__
					uint32_t model_addr = 0x0819A740;
					uint32_t model_org_val = 0x85FDF87;
#endif
					char model_suffix[17];
					sprintf(model_suffix, "_fx%s", value.text);
					*(uint32_t*)model_addr = (uint32_t)&model_suffix;
					int ret = load_item_visual_effect_org(item, NWNCX_MEMBER_X_PARAM_VAL 1);
					*(uint32_t*)model_addr = model_org_val;
					return ret;
				}
			}
		}
		return load_item_visual_effect_org(item, NWNCX_MEMBER_X_PARAM_VAL vfx);
	}
	
	int creating_body_parts = false;
	int (NWNCX_MEMBER_CALL *create_body_parts_org)(char*, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo*) = (int (NWNCX_MEMBER_CALL *)(char*, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo*))0x00543A40;
	int NWNCX_MEMBER_CALL create_body_parts_hook(char* creature_appearance, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo* appearance_info)
	{
		if (set_creature_appearance_extra)
		{
			//55 = a random armor part appearance that wont be used as the base of an xmodel appearance
			if (set_creature_appearance_extra->chest_model)
			{
				*(creature_appearance+7) = 55;
				*(creature_appearance+0x13+7) = 55;
			}
			if (set_creature_appearance_extra->pelvis_model)
			{
				*(creature_appearance+6) = 55;
				*(creature_appearance+0x13+6) = 55;
			}
		}
		creating_body_parts = true;
		int ret = create_body_parts_org(creature_appearance, NWNCX_MEMBER_X_PARAM_VAL appearance_info);
		creating_body_parts = false;
		return ret;
	}

	inline void set_alternate_race(char* race)
	{
		char temp_race = toupper(*race);
		bool was_lower_case = temp_race != *race;
		switch (temp_race)
		{
		case 'C': temp_race = 'H'; break;
		case 'W': temp_race = 'O'; break;
		case 'R': temp_race = 'E'; break;
		case 'S': temp_race = 'H'; break;
		}
		if (was_lower_case) temp_race = tolower(temp_race);
		*race = temp_race;
	}
	
#ifdef WIN32
	CExoString& NWNCX_MEMBER_CALL on_get_armor_model_concat_str_hook(CExoString* str1, NWNCX_MEMBER_X_PARAM CExoString& result, CExoString* str2)
#elif __linux__
	CExoString on_get_armor_model_concat_str_hook(CExoString* str1, CExoString* str2)
#endif
	{
#ifdef WIN32
		((CExoString& (__fastcall *)(CExoString*, int, CExoString&, CExoString*))0x005BAA10)(str1, 0, result, str2);
#elif __linux__
		CExoString result = ((CExoString (*)(CExoString*, CExoString*))0x085A4F64)(str1, str2);
#endif

		set_alternate_race(&result.text[2]);

		size_t result_len = strlen(result.text);
		if (set_creature_appearance_extra)
		{
			if (set_creature_appearance_extra->chest_model && result_len==13 && strncmp(result.text+5, "CHEST", 5)==0)
			{
				char gender, race;
				int phenotype;
				if (sscanf(result.text, "P%c%c%d_", &gender, &race, &phenotype)==3)
				{
					char real_pheno_model[17];
					sprintf(real_pheno_model, "P%c%c0_XCHEST%u", gender, race, set_creature_appearance_extra->chest_model);
					result = real_pheno_model;
				}
			}
			else if (set_creature_appearance_extra->pelvis_model && result_len==14 && strncmp(result.text+5, "PELVIS", 6)==0)
			{
				char gender, race;
				int phenotype;
				if (sscanf(result.text, "P%c%c%d_", &gender, &race, &phenotype)==3)
				{
					char real_pheno_model[17];
					sprintf(real_pheno_model, "P%c%c0_XPELVIS%u", gender, race, set_creature_appearance_extra->pelvis_model);
					result = real_pheno_model;
				}
			}
		}
		return result;
	}

	uint16_t* skip_innactive_anim_jb = (uint16_t*) 
#ifdef WIN32
		0x004BFBA1;
#elif __linux__
		0x08122F66;
#endif
	void (NWNCX_MEMBER_CALL *creature_ai_update_org)(CNWCCreature*) = (void (NWNCX_MEMBER_CALL *)(CNWCCreature*))0x004BFB00;
	void NWNCX_MEMBER_CALL creature_ai_update_hook(CNWCCreature* creature)
	{
		if (ini_play_innactive_anim == 0 || (ini_play_innactive_anim == 1 && creature->cre_id >= 0xF0000000))
		{
			*skip_innactive_anim_jb = 0xE990;
		}
		else
		{
			*skip_innactive_anim_jb = 0x820f;
		}

		return creature_ai_update_org(creature);
	}
	
	CREATURE_EXTRA* creature_walk_extra = NULL;
	uint32_t(NWNCX_MEMBER_CALL *creature_walk_update_org)(CNWCCreature*) = (uint32_t(NWNCX_MEMBER_CALL *)(CNWCCreature*))0x004C2F70;
	uint32_t NWNCX_MEMBER_CALL creature_walk_update_hook(CNWCCreature* creature)
	{
		creature_walk_extra = get_creature_extra(creature->cre_id);
		uint32_t ret = creature_walk_update_org(creature);
		creature_walk_extra = NULL;
		return ret;
	}
	int (NWNCX_MEMBER_CALL *gob_play_animation_org)(void*, NWNCX_MEMBER_X_PARAM char*, float, int, float) = (int (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM char*, float, int, float))0x007EDC40;
	int NWNCX_MEMBER_CALL gob_play_animation_hook(void* gob, NWNCX_MEMBER_X_PARAM char* anim, float speed, int unk1, float unk2)
	{
		if (creature_walk_extra)
		{
			speed /= creature_walk_extra->body_xdata.scaling;
		}
		return gob_play_animation_org(gob, NWNCX_MEMBER_X_PARAM_VAL anim, speed, unk1, unk2);
	}

	inline void update_gob_with_part_x_data(Gob* gob, PC_PART_XDATA& pc_part_xdata)
	{
		Vector euler;
		euler.x = static_cast<float>(3.141593 + pc_part_xdata.extra_rotate_x);
		euler.y = pc_part_xdata.extra_rotate_y;
		euler.z = pc_part_xdata.extra_rotate_z;
		Quaternion orientation = vector_to_quaternion(euler);
		Vector position;
		position.x = pc_part_xdata.extra_x;
		position.y = pc_part_xdata.extra_y;
		position.z = pc_part_xdata.extra_z;
		set_gob_part_position(gob, "root", position, orientation);
		set_gob_scale(gob, gob->scale2*pc_part_xdata.scaling);
	}
	
	int (NWNCX_MEMBER_CALL *create_head_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*) = (int (NWNCX_MEMBER_CALL *)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*))0x00540D20;
	int NWNCX_MEMBER_CALL create_head_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info)
	{
		last_created_gob = NULL;
		int ret = create_head_org(creature_appearance, NWNCX_MEMBER_X_PARAM_VAL p2, appearance_info);
		Gob* head_gob = NULL;
		CNWCItem* helmet = get_item_by_id(appearance_info->helmet_id);
		if (helmet && helmet->object.anim_base)
		{
			head_gob = helmet->object.anim_base->gob;
		}
		if (head_gob == NULL) head_gob = &(last_created_gob->base);
		if (head_gob)
		{
			CREATURE_EXTRA* creature_extra = get_creature_extra(creature_appearance->creature->cre_id);
			PC_PART_XDATA& head_xdata = creature_extra->custom_pc_parts_plt.parts[19];
			update_gob_with_part_x_data(head_gob, head_xdata);
		}
		return ret;
	}
	
	bool creating_tail = false;
	int (NWNCX_MEMBER_CALL *create_tail_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*) = (int (NWNCX_MEMBER_CALL *)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*))0x005419C0;
	int NWNCX_MEMBER_CALL create_tail_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info)
	{
		last_created_gob = NULL;
		creating_tail = true;
		int ret = create_tail_org(creature_appearance, NWNCX_MEMBER_X_PARAM_VAL p2, appearance_info);
		creating_tail = false;
		if (last_created_gob)
		{
			CREATURE_EXTRA* creature_extra = get_creature_extra(creature_appearance->creature->cre_id);
			PC_PART_XDATA& tail_data = creature_extra->custom_pc_parts_plt.parts[22];
			update_gob_with_part_x_data(&(last_created_gob->base), tail_data);
		}
		return ret;
	}

	bool creating_cloak = false;
	int (NWNCX_MEMBER_CALL *create_cloak_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*) = (int (NWNCX_MEMBER_CALL *)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*))0x00542540;
	int NWNCX_MEMBER_CALL create_cloak_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info)
	{
		creating_cloak = true;
		int ret = create_cloak_org(creature_appearance, NWNCX_MEMBER_X_PARAM_VAL p2, appearance_info);
		creating_cloak = false;
		return ret;
	}
	
	bool creating_wings = false;
	int (NWNCX_MEMBER_CALL *create_wings_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*) = (int (NWNCX_MEMBER_CALL *)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*))0x00541F80;
	int NWNCX_MEMBER_CALL create_wings_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info)
	{
		last_created_gob = NULL;
		creating_wings = true;
		int ret = create_wings_org(creature_appearance, NWNCX_MEMBER_X_PARAM_VAL p2, appearance_info);
		creating_wings = false;
		if (last_created_gob)
		{
			CREATURE_EXTRA* creature_extra = get_creature_extra(creature_appearance->creature->cre_id);
			PC_PART_XDATA& wings_data = creature_extra->custom_pc_parts_plt.parts[21];
			update_gob_with_part_x_data(&(last_created_gob->base), wings_data);
		}
		return ret;
	}
	
	inline bool is_bow_baseitem(int baseitem)
	{
		return (baseitem == 8 ||
				baseitem == 11 ||
				baseitem == 214 || 
				baseitem == 215 || 
				baseitem == 216);
	}
	inline bool is_bow(CNWCItem* item)
	{
		return (item && is_bow_baseitem(item->it_baseitem));
	}
	int NWNCX_MEMBER_CALL on_switch_weapon_set_equipped_by(CNWCItem* item, NWNCX_MEMBER_X_PARAM CNWCCreature* creature)
	{
		if (is_bow(item))
		{
#ifdef WIN32
			*(uint16_t*)0x004BE29F = 0xE990;
#elif __linux__
			*(uint8_t*)0x08121296 = 0xEB;
#endif
		}
		else
		{
#ifdef WIN32
			*(uint16_t*)0x004BE29F = 0x840F;
#elif __linux__
			*(uint8_t*)0x08121296 = 0x74;
#endif
		}
		return set_item_equipped_by(item, creature);
	}
	void (NWNCX_MEMBER_CALL *update_visible_weapons_org)(CNWCCreature*, NWNCX_MEMBER_X_PARAM uint32_t, CNWCItem*)  = (void (NWNCX_MEMBER_CALL *)(CNWCCreature*, NWNCX_MEMBER_X_PARAM uint32_t, CNWCItem*))0x004C7520;
	void NWNCX_MEMBER_CALL update_visible_weapons_hook(CNWCCreature* creature, NWNCX_MEMBER_X_PARAM uint32_t p3, CNWCItem* item)
	{
		if (is_bow(item))
		{
			//temporary set the baseitem to longbow
			uint16_t saved_baseitem = item->it_baseitem;
			item->it_baseitem = 8;
			update_visible_weapons_org(creature, NWNCX_MEMBER_X_PARAM_VAL p3, item);
			item->it_baseitem = saved_baseitem;
		}
		else
		{
			update_visible_weapons_org(creature, NWNCX_MEMBER_X_PARAM_VAL p3, item);
		}
	}
	
	int (NWNCX_MEMBER_CALL *anim_wield_org)(char*, NWNCX_MEMBER_X_PARAM uint32_t, CExoString, uint8_t, float) = (int (NWNCX_MEMBER_CALL *)(char*, NWNCX_MEMBER_X_PARAM uint32_t, CExoString, uint8_t, float))0x591460;
	int NWNCX_MEMBER_CALL anim_wield_hook(char* wield_anim, NWNCX_MEMBER_X_PARAM uint32_t item_id, CExoString p3, uint8_t p4, float scaling)
	{
		CNWCItem* item = get_item_by_id(item_id);
		if (item)
		{
			CNWCCreature* creature = get_item_equipped_by(item);
			if (creature)
			{
				CREATURE_EXTRA* creature_extra = get_creature_extra(creature->cre_id);
				if (p3.text)
				{
					if (strcmp(p3.text, "lhand") == 0 || strcmp(p3.text, "lforearm") == 0)
					{
						scaling *= (is_bow(item) ? creature_extra->right_hand_xdata[0].scaling : creature_extra->left_hand_xdata[0].scaling);
					}
					else if (strcmp(p3.text, "rhand") == 0)
					{
						scaling *= creature_extra->right_hand_xdata[0].scaling;
					}
				}
			}
		}
		return anim_wield_org(wield_anim, NWNCX_MEMBER_X_PARAM_VAL item_id, p3, p4, scaling);
	}

	bool update_dialog = false;
	int(NWNCX_MEMBER_CALL *handle_server_to_player_dialog_org)(void*, NWNCX_MEMBER_X_PARAM uint8_t) = (int(NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM uint8_t))0x00443C70;
	int NWNCX_MEMBER_CALL handle_server_to_player_dialog_hook(void* message, NWNCX_MEMBER_X_PARAM uint8_t p3)
	{
		update_dialog = true;
		return handle_server_to_player_dialog_org(message, NWNCX_MEMBER_X_PARAM_VAL p3);
	}
	
	void* last_server_list_panel = NULL;
	void(NWNCX_MEMBER_CALL* cancel_gamespy_connect)(void*, NWNCX_MEMBER_X_PARAM int) = (void(NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM int))
#ifdef WIN32	
		0x004DD790;
#elif __linux__
		0x08095B04;
#endif
	void(NWNCX_MEMBER_CALL *fix_dialog_size)(CGuiInGameChatDialog*) = (void(NWNCX_MEMBER_CALL *)(CGuiInGameChatDialog*))
#ifdef WIN32		
		0x00583120;
#elif __linux__
		0x0826A260;
#endif
	void(NWNCX_MEMBER_CALL *update_gui_and_render_org)(void*, NWNCX_MEMBER_X_PARAM float) = (void(NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM float))0x005DF960;
	void NWNCX_MEMBER_CALL update_gui_and_render_hook(void* gui_man, NWNCX_MEMBER_X_PARAM float p3)
	{
		update_gui_and_render_org(gui_man, NWNCX_MEMBER_X_PARAM_VAL p3);
		if (update_dialog)
		{
			if ((**p_client_exo_app)->app_internal->in_game_gui && (**p_client_exo_app)->app_internal->in_game_gui->chat_dialog)
			{
				fix_dialog_size((**p_client_exo_app)->app_internal->in_game_gui->chat_dialog);
			}
			update_dialog = false;
		}
		if (last_server_list_panel)
		{
			cancel_gamespy_connect(last_server_list_panel, NWNCX_MEMBER_X_PARAM_VAL 0);
			last_server_list_panel = NULL;
		}
	}

	void(NWNCX_MEMBER_CALL *parse_str_org)(void*, NWNCX_MEMBER_X_PARAM CExoString*) = (void(NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM CExoString*))0x005CAA20;
	void NWNCX_MEMBER_CALL parse_str_hook(void* self, NWNCX_MEMBER_X_PARAM CExoString* exo_str)
	{
		if (exo_str && exo_str->text)
		{
			std::vector<std::string> replaced_color_tokens;
			char* c_str_iter = exo_str->text;
			while (*c_str_iter)
			{
				if (*c_str_iter == '<')
				{
					if (*(c_str_iter + 1) == 'c' &&
						*(c_str_iter + 2) &&
						*(c_str_iter + 3) &&
						*(c_str_iter + 4) &&
						*(c_str_iter + 5) == '>')
					{
						replaced_color_tokens.push_back(std::string(c_str_iter, 6));
						memcpy(c_str_iter, "\x01\x02\x03\x04\x05\x06", 6);
						c_str_iter += 6;
						continue;
					}
					if (*(c_str_iter + 1) == '/' &&
						*(c_str_iter + 2) == 'c' &&
						*(c_str_iter + 3) == '>')
					{
						memcpy(c_str_iter, "\x07\x08\x09\x0A", 4);
						c_str_iter += 4;
						continue;
					}
				}
				c_str_iter++;
			}
			parse_str_org(self, NWNCX_MEMBER_X_PARAM_VAL exo_str);
			c_str_iter = exo_str->text;
			if (c_str_iter)
			{
				size_t token_found_index = 0;
				while (*c_str_iter)
				{
					if (*(c_str_iter) == 0x1 &&
						*(c_str_iter + 1) == 0x2 &&
						*(c_str_iter + 2) == 0x3 &&
						*(c_str_iter + 3) == 0x4 &&
						*(c_str_iter + 4) == 0x5 &&
						*(c_str_iter + 5) == 0x6)
					{
						memcpy(c_str_iter, replaced_color_tokens.at(token_found_index++).c_str(), 6);
						c_str_iter += 6;
						continue;
					}
					if (*(c_str_iter) == 0x7 &&
						*(c_str_iter + 1) == 0x8 &&
						*(c_str_iter + 2) == 0x9 &&
						*(c_str_iter + 3) == 0xA)
					{
						memcpy(c_str_iter, "</c>", 4);
						c_str_iter += 4;
						continue;
					}
					c_str_iter++;
				}
			}
		}
		else
		{
			return parse_str_org(self, NWNCX_MEMBER_X_PARAM_VAL exo_str);
		}
	}

	uint32_t* next_proj_type;
	int NWNCX_MEMBER_CALL read_projectile_type_2da_string(C2DA* table, NWNCX_MEMBER_X_PARAM int row, CExoString* column, CExoString* value)
	{
		*next_proj_type = 0;
		int ret = get_2da_string(table, row, column, value);
		if (value && value->text)
		{
			if (strcmp(value->text, "homing")==0)
				*next_proj_type = 1;
			else if (strcmp(value->text, "ballistic")==0)
				*next_proj_type = 2;
			else if (strcmp(value->text, "highballistic")==0)
				*next_proj_type = 3;
			else if (strcmp(value->text, "burstup")==0)
				*next_proj_type = 4;
			else if (strcmp(value->text, "accelerating")==0)
				*next_proj_type = 5;
			else if (strcmp(value->text, "spiral")==0)
				*next_proj_type = 6;
			else if (strcmp(value->text, "linked")==0)
				*next_proj_type = 7;
			else if (strcmp(value->text, "bounce")==0)
				*next_proj_type = 8;
			else if (strcmp(value->text, "burst")==0)
				*next_proj_type = 9;
			else if (strcmp(value->text, "linkedburstup")==0)
				*next_proj_type = 10;
			else if (strcmp(value->text, "tripleballistic")==0)
				*next_proj_type = 11;
			else if (strcmp(value->text, "tripleballistic2")==0)
				*next_proj_type = 12;
			else if (strcmp(value->text, "doubleballistic")==0)
				*next_proj_type = 13;
			else if (strcmp(value->text, "delayedburstup")==0)
				*next_proj_type = 0x32;
		}
		return ret;
	}

	void(NWNCX_MEMBER_CALL *add_party_member)(void*, NWNCX_MEMBER_X_PARAM uint32_t, CNWCCreature*) = (void(NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM uint32_t, CNWCCreature*))
#ifdef WIN32
		0x00526650;
#elif __linux__
		0x080C00D8;
#endif
	void NWNCX_MEMBER_CALL on_update_creature_add_to_dm_party(void* party_bar, NWNCX_MEMBER_X_PARAM uint32_t target_id, CNWCCreature* creature)
	{
		if (ini_dm_party == 0) return;

		if (ini_dm_party == 2 && target_id < 0xF0000000) return;

		return add_party_member(party_bar, NWNCX_MEMBER_X_PARAM_VAL target_id, creature);
	}

	void* (NWNCX_MEMBER_CALL *get_list_item_as_pos)(void*, NWNCX_MEMBER_X_PARAM void*) = (void* (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM void*))
#ifdef WIN32
		0x005BECC0;
#elif __linux__
		0x08075D48;
#endif
	CNWCPlayer* NWNCX_MEMBER_CALL on_build_player_list_get_player_at_pos(void* list, NWNCX_MEMBER_X_PARAM void* node)
	{
		CNWCPlayer* player = (CNWCPlayer*)get_list_item_as_pos(list, NWNCX_MEMBER_X_PARAM_VAL node);
		if (ini_show_all_players_in_player_list == 0 && player && player->player_oid < 0xF0000000)
		{
			return NULL;
		}
		return player;
	}
	
	uint32_t last_player_list_change_obj_id = OBJECT_INVALID;
	void(NWNCX_MEMBER_CALL *append_to_msg_buffer)(CGuiInGame*, NWNCX_MEMBER_X_PARAM CExoString, uint32_t, uint32_t, int, CResRef) = (void (NWNCX_MEMBER_CALL *)(CGuiInGame*, NWNCX_MEMBER_X_PARAM CExoString, uint32_t, uint32_t, int, CResRef))
#ifdef WIN32
		0x00493BD0;
#elif __linux__
		0x080B89F0;
#endif
	void NWNCX_MEMBER_CALL on_player_list_change_append_to_msg_buffer(CGuiInGame* gui_in_game, NWNCX_MEMBER_X_PARAM CExoString msg, uint32_t p3, uint32_t p4, int p5, CResRef p6)
	{
		if (ini_show_joined_msg == 0) return;

		if (ini_show_joined_msg == 2 && last_player_list_change_obj_id < 0xF0000000) return;

		append_to_msg_buffer(gui_in_game, NWNCX_MEMBER_X_PARAM_VAL msg, p3, p4, p5, p6);
	}
	uint32_t NWNCX_MEMBER_CALL on_player_list_change_read_obj_id(CNWMessage* msg)
	{
		last_player_list_change_obj_id = read_msg_obj_id(msg);
		return last_player_list_change_obj_id;
	}

	void*(NWNCX_MEMBER_CALL* new_server_list_panel_org)(void*) = (void*(NWNCX_MEMBER_CALL *)(void*))0x004DAAB0;
	void* NWNCX_MEMBER_CALL new_server_list_panel_hook(void* panel)
	{
		void* ret = new_server_list_panel_org(panel);
		last_server_list_panel = panel;
		return ret;
	}

	CNWCItem*(NWNCX_MEMBER_CALL *create_item_org)(CNWMessage*, NWNCX_MEMBER_X_PARAM uint32_t, uint8_t*, uint8_t*, char, uint8_t) = 
		(CNWCItem*(NWNCX_MEMBER_CALL *)(CNWMessage*, NWNCX_MEMBER_X_PARAM uint32_t, uint8_t*, uint8_t*, char, uint8_t))0x00441EC0;
	CNWCItem* NWNCX_MEMBER_CALL create_item_hook(CNWMessage* msg, NWNCX_MEMBER_X_PARAM uint32_t type, uint8_t* p2, uint8_t* p3, char p4, uint8_t p5)
	{
		return create_item_org(msg, NWNCX_MEMBER_X_PARAM_VAL type, p2, p3, p4, p5);
	}

	unordered_map<uint32_t, CUSTOM_RESOURCE*> custom_resources_map;
	uint32_t last_custom_resource_id = 0;
	bool add_custom_resource(CResRef& original_resref, CUSTOM_RESOURCE& custom_resource)
	{
		if (custom_resource.get_custom_resource_id()==0 || strncmp(custom_resource.original_resref.value, original_resref.value, 16)!=0)
		{
			if (resource_exists_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL &original_resref, custom_resource.get_main_res_type(), NULL) ||
				(custom_resource.get_second_res_type() != aurora::NwnResType_Unknown && resource_exists_hook(NULL, NWNCX_MEMBER_X_PARAM_VAL &original_resref, custom_resource.get_second_res_type(), NULL)))
			{
				last_custom_resource_id++;
				custom_resource.set_custom_resource_id(last_custom_resource_id);
				custom_resource.original_resref = original_resref;
				custom_resources_map[last_custom_resource_id] = &custom_resource;
			}
			else
			{
				return false;
			}
		}
		sprintf(original_resref.value, "¬%u", custom_resource.get_custom_resource_id());
		return true;
	}
	CUSTOM_RESOURCE* get_custom_resource(const std::string& res_filename)
	{
		if (!res_filename.empty() && res_filename.at(0) == '¬')
		{
			uint32_t custom_resource_id;
			sscanf(res_filename.c_str(), "¬%u", &custom_resource_id);
			auto custom_resource_iter = custom_resources_map.find(custom_resource_id);
			if (custom_resource_iter != custom_resources_map.end())
			{
				return custom_resource_iter->second;
			}
			else
			{
				debug_log_printf(3, "ERROR: custom resource not found:%s\n", res_filename.c_str());	
			}
		}
		return NULL;
	}
	CUSTOM_RESOURCE* get_custom_resource(CResRef* resref)
	{
		char resref_str[17];
		strncpy(resref_str, resref->value, 16);
		resref_str[16] = 0;
		return get_custom_resource(resref_str);
	}

	inline int armor_part_model_name_to_index(const char* armor_part_model_name)
	{
		size_t model_name_len = strlen(armor_part_model_name);
		if (model_name_len < 12) return -1;
		armor_part_model_name += 5;
		while (*armor_part_model_name && *armor_part_model_name < '0' || *armor_part_model_name > '9') armor_part_model_name++;
		armor_part_model_name -= 4;
		switch(*(uint32_t*)armor_part_model_name)
		{
			case 0x72746f6f: return 0;
			case 0x6c746f6f: return 1;
			case 0x726e6968: return 2;
			case 0x6c6e6968: return 3;
			case 0x6c67656c: return 4;
			case 0x7267656c: return 5;
			case 0x7369766c: return 6;
			case 0x74736568: return 7;
			case 0x746c6562: return 8;
			case 0x6b63656e: return 9;
			case 0x7265726f: return 10;
			case 0x6c65726f: return 11;
			case 0x72706563: return 12;
			case 0x6c706563: return 13;
			case 0x726f6873: return 14;
			case 0x6c6f6873: return 15;
			case 0x72646e61: return 16;
			case 0x6c646e61: return 17;
			case 0x65626f72: return 18;
		}
		return -1;
	}

	int(NWNCX_MEMBER_CALL *replace_texture_on_object_org)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint8_t, CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t) =
		(int (NWNCX_MEMBER_CALL *)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint8_t, CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t))0x0048FAA0;
	int NWNCX_MEMBER_CALL replace_texture_on_object_hook(CNWCObject* object, NWNCX_MEMBER_X_PARAM uint8_t p1, CResRef model_resref, CResRef texture_resref, uint32_t num_channels, uint16_t* channels_color, int p6, uint32_t p7)
	{
		uint16_t overriden_channels_color[NUM_PLT_COLOR_CHANNELS];
		if (set_creature_appearance_extra && channels_color)
		{
			int pc_part_index = -1;
			if (creating_cloak)
			{
				pc_part_index = 20;
			}
			else if (creating_wings)
			{
				pc_part_index = 21;
			}
			else if (creating_tail)
			{
				pc_part_index = 22;
			}

			if (pc_part_index != -1)
			{
				auto& part_xdata = set_creature_appearance_extra->custom_pc_parts_plt[pc_part_index];
				//apply the shadow
				if (part_xdata.has_texture_data())
				{
					add_custom_resource(texture_resref, part_xdata);
				}
				if (update_plt_colors_with_xdata(part_xdata, num_channels, channels_color, overriden_channels_color))
				{
					channels_color = overriden_channels_color;
				}
			}
		}
		return replace_texture_on_object_org(object, NWNCX_MEMBER_X_PARAM_VAL p1, model_resref, texture_resref, num_channels, channels_color, p6, p7);
	}

	CREATURE_EXTRA* restore_texture_creature_extra = NULL;
	int (NWNCX_MEMBER_CALL *restore_texture_on_object_org)(CNWCObject*) = (int (NWNCX_MEMBER_CALL *)(CNWCObject*))0x004919B0;
	int NWNCX_MEMBER_CALL restore_texture_on_object_hook(CNWCObject* object)
	{
		if (object->obj_type == 5)
		{
			restore_texture_creature_extra = get_creature_extra(object->obj_id);
		}
		int ret = restore_texture_on_object_org(object);
		restore_texture_creature_extra = NULL;
		return ret;
	}

	int NWNCX_MEMBER_CALL replace_texture_on_gob_sub_tree_hook(Gob* gob, NWNCX_MEMBER_X_PARAM char* model, char* plt, int num_channels, uint16_t* channels_color)
	{
		CResRef texture_resref;
		uint16_t overriden_channels_color[NUM_PLT_COLOR_CHANNELS];
		if (channels_color)
		{
			int pc_part_index = -1;
			CREATURE_EXTRA* creature_extra = NULL;
			if (set_creature_appearance_extra)
			{
				creature_extra = set_creature_appearance_extra;
				if (creating_body_parts)
				{
					pc_part_index = armor_part_model_name_to_index(plt);
				}
			}
			else if (restore_texture_creature_extra)
			{
				creature_extra = restore_texture_creature_extra;
				pc_part_index = armor_part_model_name_to_index(plt);
			}

			if (pc_part_index != -1 && creature_extra)
			{
				auto& part_xdata = creature_extra->custom_pc_parts_plt[pc_part_index];
				//apply the shadow
				if (part_xdata.has_texture_data())
				{
					strncpy(texture_resref.value, plt, 16);
					add_custom_resource(texture_resref, part_xdata);
					plt = texture_resref.value; //TODO fix if len == 16
				}
				if (update_plt_colors_with_xdata(part_xdata, num_channels, channels_color, overriden_channels_color))
				{
					channels_color = overriden_channels_color;
				}
			}
		}
		return replace_texture_on_gob_sub_tree(gob, NWNCX_MEMBER_X_PARAM_VAL model, plt, num_channels, channels_color);
	}


	void (NWNCX_MEMBER_CALL *remove_ghosted_items)(CNWCItem*) = (void (NWNCX_MEMBER_CALL *)(CNWCItem*))
#ifdef WIN32
0x4EB400;
#elif __linux__
0x0819BC28;
#endif
	CNWCItem* NWNCX_MEMBER_CALL on_update_creature_appearance_get_item_by_id(CClientExoApp* client_exo_app, NWNCX_MEMBER_X_PARAM uint32_t item_id)
	{
		CNWCItem* ret = get_item_by_id(item_id);
		if (ret)
		{
			CNWBaseItem* base_item = get_base_item((*p_rules)->ru_baseitems, NWNCX_MEMBER_X_PARAM_VAL ret->it_baseitem);
			if (base_item->bi_equip_slots & 1) //helmet
			{
				//force recreate the item
				remove_ghosted_items(ret);
				delete_game_object_id(get_game_objects_array(**p_client_exo_app), NWNCX_MEMBER_X_PARAM_VAL item_id);
#ifdef WIN32
				ret->vtable->Destructor(ret, NWNCX_MEMBER_X_PARAM_VAL 3);
#elif __linux__
				ret->object.vtable->Destructor(&ret->object, 3);
#endif
				ret = NULL;
			}
		}
		return ret;
	}

	int creating_item_for_creature = -1;
	CNWCItem* NWNCX_MEMBER_CALL on_update_creature_appearance_create_item(CNWMessage* msg, NWNCX_MEMBER_X_PARAM uint32_t type, uint8_t* p2, uint8_t* p3, char p4, uint8_t p5)
	{
		creating_item_for_creature = type;
		CNWCItem* ret = create_item_hook(msg, NWNCX_MEMBER_X_PARAM_VAL type, p2, p3, p4, p5);
		creating_item_for_creature = -1;
		return ret;
	}

	uint32_t NWNCX_MEMBER_CALL on_apply_vfx_on_object_get_helmet_id(CNWCCreature* creature, NWNCX_MEMBER_X_PARAM uint32_t item_id)
	{
		return OBJECT_INVALID;
	}

	int (NWNCX_MEMBER_CALL *cnwcanimbase_replace_texture_org)(CNWCAnimBase*, NWNCX_MEMBER_X_PARAM CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t) = 
		(int (NWNCX_MEMBER_CALL *)(CNWCAnimBase*, NWNCX_MEMBER_X_PARAM CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t))0x005757A0;
	int NWNCX_MEMBER_CALL cnwcanimbase_replace_texture_hook(CNWCAnimBase* animbase, NWNCX_MEMBER_X_PARAM CResRef resref1, CResRef resref2, uint32_t num_channels, uint16_t* channels_color, int p6, uint32_t p7)
	{
		uint16_t overriden_channels_color[NUM_PLT_COLOR_CHANNELS];
		if (creating_item_for_creature != -1)
		{
			CREATURE_EXTRA* creature_extra = get_creature_extra(creature_update_appearance_id);
			if (creature_extra)
			{
				/*//to find an appearance index
				CNWCCreature* creature = creature_by_id(creature_update_appearance_id);
				if (creature)
				{
					for (int i=0; i<0x100; i++)
					{
						uint8_t* appearance_data = (uint8_t*)creature->appearance;
						if (*(appearance_data+i) == 163)
						{
							char data[333];
							sprintf(data, "found at:%i\n", i);
							MessageBoxA(0, data, "", 0);
						}
					}
				}*/

				CNWBaseItem* base_item = get_base_item((*p_rules)->ru_baseitems, NWNCX_MEMBER_X_PARAM_VAL creating_item_for_creature);
				if (base_item->bi_equip_slots & 1) //helmet
				{
					/*//this is not working well.. sometime the helmet is rendered before the armor/body
					CNWCCreature* creature = creature_by_id(creature_update_appearance_id);
					if (creature)
					{
						uint32_t palette_height_per_channel = *paletteheight / 7;
						channels_color[NwnPalette_Skin] = creature->appearance->skin_color;
						channels_color[NwnPalette_Hair] = creature->appearance->hair_color + palette_height_per_channel;
						channels_color[NwnPalette_Tattoo1] = creature->appearance->tattoo1_color + (palette_height_per_channel*6);
						channels_color[NwnPalette_Tattoo2] = creature->appearance->tattoo2_color + (palette_height_per_channel*6);
					}*/
					auto& part_xdata = creature_extra->custom_pc_parts_plt[19];
					if (part_xdata.has_texture_data())
					{
						add_custom_resource(resref2, part_xdata);
					}
					if (update_plt_colors_with_xdata(part_xdata, num_channels, channels_color, overriden_channels_color))
					{
						channels_color = overriden_channels_color;
					}
				}
			}
		}
		return cnwcanimbase_replace_texture_org(animbase, NWNCX_MEMBER_X_PARAM_VAL resref1, resref2, num_channels, channels_color, p6, p7);
	}

	void (NWNCX_MEMBER_CALL *destroy_game_object_array_org)(CGameObjectArray* NWNCX_DESTRUCTOR_P2) = (void (NWNCX_MEMBER_CALL *)(CGameObjectArray* NWNCX_DESTRUCTOR_P2))0x0042E2B0;
	void NWNCX_MEMBER_CALL destroy_game_object_array_hook(CGameObjectArray* go_array NWNCX_DESTRUCTOR_P2)
	{
		destroy_game_object_array_org(go_array NWNCX_DESTRUCTOR_P2_VAL);
		for (CREATURES_EXTRA::iterator iter=creatures_extra.begin(); iter!=creatures_extra.end(); iter++)
		{
			delete iter->second;
		}
		creatures_extra.clear();
	}

	void (NWNCX_MEMBER_CALL *enable_vertex_program_org)(void*, NWNCX_MEMBER_X_PARAM void*, bool) = (void (NWNCX_MEMBER_CALL *)(void*, NWNCX_MEMBER_X_PARAM void*, bool))0x007DE790;
	void NWNCX_MEMBER_CALL enable_vertex_program_hook(void* vertex_program, NWNCX_MEMBER_X_PARAM void* part, bool enabled)
	{
		if (vertex_program)
		{
			enable_vertex_program_org(vertex_program, NWNCX_MEMBER_X_PARAM_VAL part, enabled);
		}
	}
}
}
