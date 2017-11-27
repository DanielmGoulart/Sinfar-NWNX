#pragma once

#include "../../nwn_tools/CKey/CKey.h"
#include "../../nwn_tools/CERF/CERF.h"
#include "../nwncx.h"
#include <deque>
#include "../../nwnx/sinfarx/xdata.h"
#include "../ini/IniFile.h"

#define INLINE

namespace nwncx
{
namespace sinfar
{
	extern int NWNCX_SINFAR_VERSION;

	extern void(*print_stacktrace)(const std::string&);
	
	extern char log_filename[];
	
	INLINE void log_printf(const char* format, ...);
	INLINE void debug_log_printf(int min_log_level, const char* format, ...);

	extern const char* INI_SECTION;
	std::string format_float_for_ini(float f);
	void nwncx_sinfar_init(void (*load_extra_ini_settings)(CIniFileA&) = NULL);

#define TESTING 1
#ifdef TESTING
#ifdef WIN32
	extern "C" __declspec(dllexport)
#endif
	const char* NWNCXSinfar_DebugMe(const char* info);
#endif
	
	extern int is_on_sinfar;
	extern uint32_t server_ip;
	extern bool testing;

	INLINE int is_sinfar_server(uint32_t ip_data);
	
	extern uint8_t crypt_key[2];
	extern uint8_t crypt_key2[3];
	inline const char* my_decrypt(const uint8_t to_decrypt[], size_t to_decrypt_len, const uint8_t crypt_key[], size_t crypt_key_len);
	#define MY_DECRYPT(arg1, arg2) nwncx::sinfar::my_decrypt(arg1, sizeof(arg1), arg2, sizeof(arg2))

	extern char ini_filename[];
	extern const char* INI_KEY_DISABLE_ATI_SHADERS;

	extern int ini_log_level;
	extern int ini_disable_ati_shaders;
	extern uint32_t resman_cache_size;
	extern int ini_resman_log_requested_resources;
	extern int ini_resman_log_missing_resources;
	extern int ini_resman_use_nwndata_folder;
	extern int ini_enable_camera_hack;
	extern float ini_cam_min_zoom;
	extern float ini_cam_max_zoom;
	extern float ini_cam_min_pitch;
	extern float ini_cam_max_pitch;
	extern float ini_cam_default_min_zoom;
	extern float ini_cam_default_max_zoom;

	struct CREATURE_EXTRA_ATTACHED
	{
		float attach_x;
		float attach_y;
		float attach_z;
		uint32_t target_id;
		CREATURE_EXTRA_ATTACHED():	
							attach_x(0),
							attach_y(0),
							attach_z(0),
							target_id(OBJECT_INVALID)
		{
			//constructor
		}
	};
	class CUSTOM_RESOURCE;
	extern unordered_map<uint32_t, CUSTOM_RESOURCE*> custom_resources_map;
	class CUSTOM_RESOURCE
	{
	public:
		CUSTOM_RESOURCE() : custom_resource_id(0) {original_resref.value[0] = 0;}
		CUSTOM_RESOURCE(const CUSTOM_RESOURCE& copy) : custom_resource_id(0), original_resref(copy.original_resref) {}
		inline void remove_from_map()
		{
			if (custom_resource_id)
			{
				custom_resources_map.erase(custom_resource_id);
				custom_resource_id = 0;
			}
		}
		virtual ~CUSTOM_RESOURCE() 
		{
			remove_from_map();
		}
		inline void set_custom_resource_id(uint32_t custom_res_id)
		{
			remove_from_map();
			custom_resource_id = custom_res_id;
		}
		inline uint32_t get_custom_resource_id()
		{
			return custom_resource_id;
		}
		virtual uint16_t get_main_res_type() { print_stacktrace("CUSTOM_RESOURCE::get_main_res_type"); return (uint16_t)aurora::NwnResType_Unknown; }
		virtual uint16_t get_second_res_type() { return (uint16_t)aurora::NwnResType_Unknown; }
		virtual void pre_modify_original_res(CRes* res, uint16_t res_type) {};
		virtual void post_modify_original_res(CRes* res, uint16_t res_type) {};

		CResRef original_resref;
	private:
		uint32_t custom_resource_id;
	};

#pragma pack(push, 1)
	struct CResDDSHeader
	{
		uint32_t width; //0x0
		uint32_t height; //0x4
		uint32_t num_channels; //0x8
		uint32_t unk_pixels; //0xc
	};
	struct CResDDS
	{
		CRes res;
		uint32_t unk_0x2c;
		uint32_t size; //0x30
		void* data; //0x34
		CResDDSHeader* header; //0x38
	};
	struct CResTGAHeader
	{
		int unk_0x0;
		int unk_0x4;
		int unk_0x8;
		uint16_t width; //0xc
		uint16_t height; //0xe
		uint8_t num_bits;
	};
	struct CResTGA
	{
		CRes res;
		uint32_t unk_0x2c;
		uint32_t unk_0x30;
		uint32_t unk_0x34;
		uint32_t unk_0x38;
		uint32_t size; //0x3c;
		uint32_t unk_0x40;
		uint32_t color_map; //0x44
		char* tga_data; //0x48;
		CResTGAHeader* header; //0x4c;
	};
	struct TGAPixel_32
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;
	};
	struct TGAPixel_24
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	};
#pragma pack(pop)

	#define CREATURE_EXTRA_FLAG_APPPEARANCE_SET 0x1
	struct CREATURE_EXTRA;
	INLINE CREATURE_EXTRA* get_creature_extra(uint32_t creature_id);
	typedef unordered_map<uint32_t, CREATURE_EXTRA*> CREATURES_EXTRA;
	extern CREATURES_EXTRA creatures_extra;
	CREATURE_EXTRA* get_creature_extra(uint32_t creature_id);
	void update_creature_extra(CNWCCreature* creature);
	void on_destroy_creature(CNWCCreature* creature);

	class DispatchedUpdate
	{
	public:
		virtual DispatchedUpdate* clone() = 0;
		virtual void perform() = 0;
		virtual ~DispatchedUpdate(){}
	};
	extern std::deque<std::unique_ptr<DispatchedUpdate>> dispatched_updates;
	extern aurora::mutex dispatched_updates_mutex;
	void dispatch_update(std::unique_ptr<DispatchedUpdate> dispatched_update);
	void on_render();

	struct PLACEABLE_EXTRA
	{
		float scale;
		PLACEABLE_EXTRA():	scale(1.0)
		{
			//constructor
		}
	};
	typedef unordered_map<uint32_t, PLACEABLE_EXTRA*> PLACEABLES_EXTRA;
	extern PLACEABLES_EXTRA placeables_extra;
	PLACEABLE_EXTRA* get_placeable_extra(uint32_t plc_id);

	extern void (NWNCX_MEMBER_CALL *destroy_placeable_org)(CNWCPlaceable*);
	void NWNCX_MEMBER_CALL destroy_placeable_hook(CNWCPlaceable*);
	class CUSTOM_PLT;
	class ExtendedGob;
	extern ExtendedGob* last_created_gob;
	void* alloc_gob_hook(uint32_t size);
	extern void* (NWNCX_MEMBER_CALL *destroy_caurobject_org)(ExtendedGob*, NWNCX_MEMBER_X_PARAM int);
	void* NWNCX_MEMBER_CALL destroy_caurobject_hook(ExtendedGob* gob, NWNCX_MEMBER_X_PARAM int p2);
	
	extern uint32_t creature_update_appearance_id;

	//base flags
	#define NWNCX_SINFAR_CREATURE_XDATA_BODY			0x00000001
	#define NWNCX_SINFAR_CREATURE_XDATA_ARMOR			0x00000002
	#define NWNCX_SINFAR_CREATURE_XDATA_LHAND			0x00000004
	#define NWNCX_SINFAR_CREATURE_XDATA_RHAND			0x00000008
	#define NWNCX_SINFAR_CREATURE_EXTRA_ATTACH			0x00000020
	//armor
	#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XCHEST		0x00000001
	#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XPELVIS		0x00000002
	//position
	#define NWNCX_SINFAR_CREATURE_EXTRA_POSITION_Z				0x00000001
	#define NWNCX_SINFAR_CREATURE_EXTRA_POSITION_ROTATE_Z		0x00000002
	#define NWNCX_SINFAR_CREATURE_EXTRA_POSITION_ROTATE_Y		0x00000004
	//right & left hands item
	#define NWNCX_SINFAR_CREATURE_CUSTOM_WEAPON_LEFT_SCALE		0x00000001
	#define NWNCX_SINFAR_CREATURE_CUSTOM_WEAPON_RIGHT_SCALE		0x00000002
	uint32_t read_creature_appearance_obj_id(CNWMessage* message);
	extern CNWCCreature* set_creature_appearance;
	extern CREATURE_EXTRA* set_creature_appearance_extra;
	extern uint8_t last_appearance_update_phenotype;

	uint16_t NWNCX_MEMBER_CALL on_update_vfx_read_vfx(CNWMessage* message, NWNCX_MEMBER_X_PARAM int num_bits);
	
	extern int (NWNCX_MEMBER_CALL *delete_visual_effect_to_object_org)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint16_t);
	int NWNCX_MEMBER_CALL delete_visual_effect_to_object_hook(CNWCObject* object, NWNCX_MEMBER_X_PARAM uint16_t vfx);
	extern int (NWNCX_MEMBER_CALL *add_visual_effect_to_object_org)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint16_t, int, uint32_t, uint8_t, uint8_t, Vector);
	int NWNCX_MEMBER_CALL add_visual_effect_to_object_hook(CNWCObject* object, NWNCX_MEMBER_X_PARAM uint16_t vfx, int p3, uint32_t p4, uint8_t p5, uint8_t p6, Vector p7);	
	extern int (NWNCX_MEMBER_CALL *apply_fnf_anim_at_location_org)(void*, NWNCX_MEMBER_X_PARAM uint16_t, Vector);
	int NWNCX_MEMBER_CALL apply_fnf_anim_at_location_hook(void* area, NWNCX_MEMBER_X_PARAM uint16_t vfx, Vector position);
	extern void (NWNCX_MEMBER_CALL *update_fountain_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float);
	void NWNCX_MEMBER_CALL update_fountain_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2);
	extern void (NWNCX_MEMBER_CALL *update_explosion_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float);
	void NWNCX_MEMBER_CALL update_explosion_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2);
	extern void (NWNCX_MEMBER_CALL *update_single_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float);
	void NWNCX_MEMBER_CALL update_single_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2);
	extern void (NWNCX_MEMBER_CALL *update_lightning_emitter_org)(CAurPart* part, NWNCX_MEMBER_X_PARAM float);
	void NWNCX_MEMBER_CALL update_lightning_emitter_hook(CAurPart* part, NWNCX_MEMBER_X_PARAM float p2);

	extern void* (*find_model_org)(char*);
	void* find_model_hook(char*);

	extern int(NWNCX_MEMBER_CALL *set_creature_appearance_info_org)(CNWCCreature*, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo*, uint8_t);
	int NWNCX_MEMBER_CALL set_creature_appearance_info_hook(CNWCCreature* creature, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo* appearance_info, uint8_t p3);
	
	extern void(NWNCX_MEMBER_CALL *force_update_creature_appearance_org)(CNWCCreature*);
	void NWNCX_MEMBER_CALL force_update_creature_appearance_hook(CNWCCreature* creature);

	extern Vector (NWNCX_MEMBER_CALL *set_gob_position_org)(Gob*, NWNCX_MEMBER_X_PARAM Vector);
	Vector NWNCX_MEMBER_CALL set_gob_position_hook(Gob* gob, NWNCX_MEMBER_X_PARAM Vector v);
	
	extern Quaternion (NWNCX_MEMBER_CALL *set_gob_orientation_org)(Gob*, NWNCX_MEMBER_X_PARAM Quaternion);
	Quaternion NWNCX_MEMBER_CALL set_gob_orientation_hook(Gob* gob, NWNCX_MEMBER_X_PARAM Quaternion orientation);
	
	extern int (NWNCX_MEMBER_CALL *load_item_visual_effect_org)(void*, NWNCX_MEMBER_X_PARAM uint8_t);
	int NWNCX_MEMBER_CALL load_item_visual_effect_hook(void* item, NWNCX_MEMBER_X_PARAM uint8_t vfx);

	extern int (NWNCX_MEMBER_CALL *create_body_parts_org)(char*, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo*);
	int NWNCX_MEMBER_CALL create_body_parts_hook(char* creature_appearance, NWNCX_MEMBER_X_PARAM CCreatureAppearanceInfo* appearance_info);
#ifdef WIN32
	CExoString& NWNCX_MEMBER_CALL on_get_armor_model_concat_str_hook(CExoString* str1, NWNCX_MEMBER_X_PARAM CExoString& result, CExoString* str2);
#elif __linux__
	CExoString on_get_armor_model_concat_str_hook(CExoString* str1, CExoString* str2);
#endif

	extern uint16_t* skip_innactive_anim_jb;
	extern void (NWNCX_MEMBER_CALL *creature_ai_update_org)(CNWCCreature*);
	void NWNCX_MEMBER_CALL creature_ai_update_hook(CNWCCreature* creature);

	extern uint32_t(NWNCX_MEMBER_CALL *creature_walk_update_org)(CNWCCreature*);
	uint32_t NWNCX_MEMBER_CALL creature_walk_update_hook(CNWCCreature* creature);
	extern int (NWNCX_MEMBER_CALL *gob_play_animation_org)(void*, NWNCX_MEMBER_X_PARAM char*, float, int, float);
	int NWNCX_MEMBER_CALL gob_play_animation_hook(void* gob, NWNCX_MEMBER_X_PARAM char* anim, float speed, int unk1, float unk2);

	extern int (NWNCX_MEMBER_CALL *load_area_org)(void*, NWNCX_MEMBER_X_PARAM void* aur_camera, void* gui_area_load_screen);
	int NWNCX_MEMBER_CALL load_area_hook(void* area, NWNCX_MEMBER_X_PARAM void* aur_camera, void* gui_area_load_screen);

	uint32_t NWNCX_MEMBER_CALL read_placeable_id(CNWMessage* message);
	extern int (NWNCX_MEMBER_CALL *add_static_placeable_to_area_org)(void*, NWNCX_MEMBER_X_PARAM uint32_t, uint16_t, Vector, Vector);
	int NWNCX_MEMBER_CALL add_static_placeable_to_area_hook(void* area, NWNCX_MEMBER_X_PARAM uint32_t p2, uint16_t p3, Vector p4, Vector p5);
	extern void* (NWNCX_MEMBER_CALL *spawn_tile_org)(void*, NWNCX_MEMBER_X_PARAM char*, Vector*, int, int, Quaternion*);
	void* NWNCX_MEMBER_CALL spawn_tile_hook(void* scene, NWNCX_MEMBER_X_PARAM char* p2, Vector* position, int p4, int p5, Quaternion* orientation);

	extern int (NWNCX_MEMBER_CALL *create_head_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*);
	int NWNCX_MEMBER_CALL create_head_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info);

	extern int (NWNCX_MEMBER_CALL *create_tail_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*);
	int NWNCX_MEMBER_CALL create_tail_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info);

	extern int (NWNCX_MEMBER_CALL *create_wings_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*);
	int NWNCX_MEMBER_CALL create_wings_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info);
	
	int NWNCX_MEMBER_CALL on_switch_weapon_set_equipped_by(CNWCItem* item, NWNCX_MEMBER_X_PARAM CNWCCreature* creature);
	extern void (NWNCX_MEMBER_CALL *update_visible_weapons_org)(CNWCCreature*, NWNCX_MEMBER_X_PARAM uint32_t, CNWCItem*);
	void NWNCX_MEMBER_CALL update_visible_weapons_hook(CNWCCreature* creature, NWNCX_MEMBER_X_PARAM uint32_t p3, CNWCItem* item);
	
	extern int (NWNCX_MEMBER_CALL *anim_wield_org)(char*, NWNCX_MEMBER_X_PARAM uint32_t, CExoString, uint8_t, float);
	int NWNCX_MEMBER_CALL anim_wield_hook(char* wield_anim, NWNCX_MEMBER_X_PARAM uint32_t item_id, CExoString p3, uint8_t p4, float scaling);
	
	extern int(NWNCX_MEMBER_CALL *handle_server_to_player_dialog_org)(void*, NWNCX_MEMBER_X_PARAM uint8_t);
	int NWNCX_MEMBER_CALL handle_server_to_player_dialog_hook(void* message, NWNCX_MEMBER_X_PARAM uint8_t p3);

	extern void(NWNCX_MEMBER_CALL *update_gui_and_render_org)(void*, NWNCX_MEMBER_X_PARAM float);
	void NWNCX_MEMBER_CALL update_gui_and_render_hook(void* gui_man, NWNCX_MEMBER_X_PARAM float p3);

	extern void(NWNCX_MEMBER_CALL *parse_str_org)(void*, NWNCX_MEMBER_X_PARAM CExoString*);
	void NWNCX_MEMBER_CALL parse_str_hook(void*, NWNCX_MEMBER_X_PARAM CExoString*);

	extern uint32_t* next_proj_type;
	int NWNCX_MEMBER_CALL read_projectile_type_2da_string(C2DA* table, NWNCX_MEMBER_X_PARAM int row, CExoString* column, CExoString* value);

	CNWCPlayer* NWNCX_MEMBER_CALL on_build_player_list_get_player_at_pos(void* list, NWNCX_MEMBER_X_PARAM void* node);
	
	extern void(NWNCX_MEMBER_CALL *add_party_member)(void*, NWNCX_MEMBER_X_PARAM uint32_t, CNWCCreature*);
	void NWNCX_MEMBER_CALL on_update_creature_add_to_dm_party(void* party_bar, NWNCX_MEMBER_X_PARAM uint32_t p2, CNWCCreature* creature);

	extern void(NWNCX_MEMBER_CALL *append_to_msg_buffer)(CGuiInGame*, NWNCX_MEMBER_X_PARAM CExoString, uint32_t, uint32_t, int, CResRef);
	void NWNCX_MEMBER_CALL on_player_list_change_append_to_msg_buffer(CGuiInGame* gui_in_game, NWNCX_MEMBER_X_PARAM CExoString msg, uint32_t p3, uint32_t p4, int p5, CResRef p6);

	uint32_t NWNCX_MEMBER_CALL on_player_list_change_read_obj_id(CNWMessage* msg);

	extern void*(NWNCX_MEMBER_CALL* new_server_list_panel_org)(void*);
	void* NWNCX_MEMBER_CALL new_server_list_panel_hook(void* panel);
	
	extern CNWCItem*(NWNCX_MEMBER_CALL *create_item_org)(CNWMessage*, NWNCX_MEMBER_X_PARAM uint32_t, uint8_t*, uint8_t*, char, uint8_t);
	CNWCItem* NWNCX_MEMBER_CALL create_item_hook(CNWMessage* msg, NWNCX_MEMBER_X_PARAM uint32_t type, uint8_t* p2, uint8_t* p3, char p4, uint8_t p5);
	
	extern int(NWNCX_MEMBER_CALL *replace_texture_on_object_org)(CNWCObject*, NWNCX_MEMBER_X_PARAM uint8_t, CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t);
	int NWNCX_MEMBER_CALL replace_texture_on_object_hook(CNWCObject* object, NWNCX_MEMBER_X_PARAM uint8_t p1, CResRef resref1, CResRef resref2, uint32_t p4, uint16_t* p5, int p6, uint32_t p7);

	uint32_t NWNCX_MEMBER_CALL on_apply_vfx_on_object_get_helmet_id(CNWCCreature* creature, NWNCX_MEMBER_X_PARAM uint32_t item_id);

	extern int (NWNCX_MEMBER_CALL *replace_texture_on_gob_sub_tree)(Gob*, NWNCX_MEMBER_X_PARAM char*, char*, int, uint16_t*);
	int NWNCX_MEMBER_CALL replace_texture_on_gob_sub_tree_hook(Gob* gob, NWNCX_MEMBER_X_PARAM char* model, char* plt, int num_channels, uint16_t* channels_color);

	extern int (NWNCX_MEMBER_CALL *restore_texture_on_object_org)(CNWCObject*);
	int NWNCX_MEMBER_CALL restore_texture_on_object_hook(CNWCObject* object);

	CNWCItem* NWNCX_MEMBER_CALL on_update_creature_appearance_get_item_by_id(CClientExoApp* client_exo_app, NWNCX_MEMBER_X_PARAM uint32_t item_id);
	CNWCItem* NWNCX_MEMBER_CALL on_update_creature_appearance_create_item(CNWMessage* msg, NWNCX_MEMBER_X_PARAM uint32_t type, uint8_t* p2, uint8_t* p3, char p4, uint8_t p5);
	extern int (NWNCX_MEMBER_CALL *cnwcanimbase_replace_texture_org)(CNWCAnimBase*, NWNCX_MEMBER_X_PARAM CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t);
	int NWNCX_MEMBER_CALL cnwcanimbase_replace_texture_hook(CNWCAnimBase* animbase, NWNCX_MEMBER_X_PARAM CResRef resref1, CResRef resref2, uint32_t p4, uint16_t* p5, int p6, uint32_t p7);

	extern int (NWNCX_MEMBER_CALL *create_cloak_org)(CNWCCreatureAppearance*, NWNCX_MEMBER_X_PARAM uint8_t, CCreatureAppearanceInfo*);
	int NWNCX_MEMBER_CALL create_cloak_hook(CNWCCreatureAppearance* creature_appearance, NWNCX_MEMBER_X_PARAM uint8_t p2, CCreatureAppearanceInfo* appearance_info);

	extern void* (*get_texture_ref_org)(const char*, const char*);
	void* get_texture_ref_hook(const char* texture_resref, const char* txi_resref);

	bool add_custom_resource(CResRef& original_resref, CUSTOM_RESOURCE& custom_resource);
	CUSTOM_RESOURCE* get_custom_resource(const std::string& res_filename);
	CUSTOM_RESOURCE* get_custom_resource(CResRef* resref);

	extern void (NWNCX_MEMBER_CALL *destroy_game_object_array_org)(CGameObjectArray* NWNCX_DESTRUCTOR_P2);
	void NWNCX_MEMBER_CALL destroy_game_object_array_hook(CGameObjectArray* go_array NWNCX_DESTRUCTOR_P2);

	extern void (NWNCX_MEMBER_CALL *enable_vertex_program_org)(void*, NWNCX_MEMBER_X_PARAM void*, bool);
	void NWNCX_MEMBER_CALL enable_vertex_program_hook(void* vertex_program, NWNCX_MEMBER_X_PARAM void* part, bool enabled);
}
}