#include "NWNCXSinfar.h"

#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <malloc.h>

#include "../../nwncx/sinfar/nwncx_sinfar.h"
#include "../../nwncx/sinfar/resman.h"
using namespace nwncx;
using namespace nwncx::sinfar;

#include "../nwncx_core/utils.h"
using namespace nwncx::utils;

namespace 
{

int (*DestroyCreature_Org)(CNWCCreature* creature);
int DestroyCreature_Hook(CNWCCreature* creature)
{
	on_destroy_creature(creature);
	return DestroyCreature_Org(creature);
}

void (*RenderScene_Org)(void*);
void RenderScene_Hook(void* scene)
{
	on_render();
	return RenderScene_Org(scene);
}

int (*SendDirectMessage)(CNetLayerInternal*, uint32_t, char*, uint32_t, uint32_t, uint32_t) = (int (*)(CNetLayerInternal*, uint32_t, char*, uint32_t, uint32_t, uint32_t))0x084A39C8;
int SendPlayerInfo_Hook(CNetLayerInternal* net_internal, uint32_t p1, char* send_data, uint32_t p3, uint32_t p4, uint32_t p5)
{
	char* exo_net_internal = (char*)net_internal->exo_net->exo_net_internal;
	char* addr_info = *(char**)(exo_net_internal+0x3C);
	server_ip = *(uint32_t*)((addr_info+4)+(p1*16));
	int sinfar_server_type = is_sinfar_server(server_ip);
	if (sinfar_server_type && *(uint16_t*)(send_data+0xb) == 3)
	{
		*(uint16_t*)(send_data+0xb) = NWNCX_SINFAR_VERSION;

		if (!is_on_sinfar)
		{
			is_on_sinfar = sinfar_server_type;
			
			//read extra appearance info
			hook_call(0x081530F5, (long)read_creature_appearance_obj_id);
			
			if (is_on_sinfar >= 2)
			{
				delete_visual_effect_to_object_org = (int (*)(CNWCObject*, uint16_t))hook_function(0x081AEF04, (long)delete_visual_effect_to_object_hook, 12);
				add_visual_effect_to_object_org = (int (*)(CNWCObject*, uint16_t, int, uint32_t, uint8_t, uint8_t, Vector))hook_function(0x081AEDB4, (long)add_visual_effect_to_object_hook, 12);
				apply_fnf_anim_at_location_org = (int (*)(void*, uint16_t, Vector))hook_function(0x08111668, (long)apply_fnf_anim_at_location_hook, 12);
				hook_call(0x0815AF3B, (long)on_update_vfx_read_vfx);
				hook_call(0x0815496D, (long)on_update_vfx_read_vfx);
				hook_call(0x0814CEED, (long)on_update_vfx_read_vfx);
				get_texture_ref_org = (void* (*)(const char*, const char*))hook_function(0x08529C18, (long)get_texture_ref_hook, 10);
			
				enable_write((long)skip_innactive_anim_jb);
				creature_ai_update_org = (void (*)(CNWCCreature*))hook_function(0x08122EC8, (long)creature_ai_update_hook, 12);
			
				update_fountain_emitter_org = (void (*)(CAurPart*, float))hook_function(0x0853D30C, (long)update_fountain_emitter_hook, 12);
				update_explosion_emitter_org = (void (*)(CAurPart*, float))hook_function(0x0853DCC8, (long)update_explosion_emitter_hook, 12);
				update_single_emitter_org = (void (*)(CAurPart*, float))hook_function(0x0853E1C4, (long)update_single_emitter_hook, 12);
				update_lightning_emitter_org = (void (*)(CAurPart*, float))hook_function(0x0853E588, (long)update_lightning_emitter_hook, 12);
				
				hook_call(0x0810C135, (long)read_placeable_id);
				hook_call(0x0810C241, (long)read_placeable_id);
				add_static_placeable_to_area_org = (int (*)(void*, uint32_t, uint16_t, Vector, Vector))hook_function(0x08113C20, (long)add_static_placeable_to_area_hook, 12);
				spawn_tile_org = (void* (*)(void*, char*, Vector*, int, int, Quaternion*))hook_function(0x085174AC, (long)spawn_tile_hook, 12);
				load_area_org = (int (*)(void*, void*, void*))hook_function(0x0810B01C, (long)load_area_hook, 12);
			}
			
			RenderScene_Org = (void (*)(void*))hook_function(0x08512FAC, (long)RenderScene_Hook, 11);

			set_creature_appearance_info_org = (int (*)(CNWCCreature*, CCreatureAppearanceInfo*, uint8_t))hook_function(0x0812AA9C, (long)set_creature_appearance_info_hook, 12);

			enable_write(0x08292391);
			force_update_creature_appearance_org = (void (*)(CNWCCreature*))hook_function(0x08134D30, (long)force_update_creature_appearance_hook, 12);
			
			find_model_org = (void* (*)(char*))hook_function(0x084F7A00, (long)find_model_hook, 12);
			
			set_gob_position_org = (Vector (*)(Gob*, Vector))hook_function(0x084E840C, (long)set_gob_position_hook, 8);
			
			set_gob_orientation_org = (Quaternion (*)(Gob*, Quaternion))hook_function(0x084E8440, (long)set_gob_orientation_hook, 8);
			
			//extra chests
			hook_call(0x08293526, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x08293352, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x0829378B, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x08293890, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x08293A4A, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x08293E68, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x08293F6C, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x08294126, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x082965D9, (long)on_get_armor_model_concat_str_hook);
			hook_call(0x082967A1, (long)on_get_armor_model_concat_str_hook);
			create_body_parts_org = (int (*)(char*, CCreatureAppearanceInfo*))hook_function(0x08292A28, (long)create_body_parts_hook, 12);
			
			gob_play_animation_org = (int (*)(void*, char*, float, int, float))hook_function(0x08531A7C, (long)gob_play_animation_hook, 12);
			creature_walk_update_org = (uint32_t (*)(CNWCCreature*))hook_function(0x08126C04, (long)creature_walk_update_hook, 12);
			
			create_head_org = (int (*)(CNWCCreatureAppearance*, uint8_t, CCreatureAppearanceInfo*))hook_function(0x0828E948, (long)create_head_hook, 12);

			create_tail_org = (int (*)(CNWCCreatureAppearance*, uint8_t, CCreatureAppearanceInfo*))hook_function(0x0828FCA4, (long)create_tail_hook, 12);
			enable_write(0x0815384F);
			enable_write(0x0828FF8B);

			create_wings_org = (int (*)(CNWCCreatureAppearance*, uint8_t, CCreatureAppearanceInfo*))hook_function(0x082904D0, (long)create_wings_hook, 12);
			enable_write(0x0815384F);
			enable_write(0x082907B7);
			
			//custom longbows
			hook_call(0x0812122D, (long)on_switch_weapon_set_equipped_by);
			enable_write(0x08121296);
			update_visible_weapons_org = (void (*)(CNWCCreature* creature, uint32_t p3, CNWCItem* item))hook_function(0x081178D0, (long)update_visible_weapons_hook, 12);

			anim_wield_org = (int (*)(char*, uint32_t, CExoString, uint8_t, float))hook_function(0x08288F84, (long)anim_wield_hook, 12);
		}
	}
	else if (is_on_sinfar)
	{
		exit(0);
	}
	return SendDirectMessage(net_internal, p1, send_data, p3, p4, p5);
}

} //anonymous namesapce

const char* miles_folder = "miles_linux";
CNWNCXSinfar::CNWNCXSinfar()
{
	mallopt(M_CHECK_ACTION, 1);
	
	/*printf("attach to debugger and press enter...\n");
	char wait_debugger;
	scanf("%c", &wait_debugger);//*/

	nwncx_sinfar_init();

	//disable master server
	char* patch = (char*)0x0808A3B2;
	enable_write((long)patch);
	patch[1] = 0x84;
	*(long*)(patch+2) = 0x0808AFAF-0x0808A3B8;
	
	//cleanup when destroying a creature
	DestroyCreature_Org = (int (*)(CNWCCreature*))hook_function(0x0812226C, (long)DestroyCreature_Hook, 12);
	
	//send nwncx version
	hook_call(0x08498EB2, (long)SendPlayerInfo_Hook);
	
	next_proj_type = (uint32_t*)0x081EC21E;
	enable_write((uint32_t)next_proj_type);
	enable_write(0x081EC216);
	*(uint16_t*)0x081EC216 = 0x9090;
	hook_call(0x081EC1C9, (long)read_projectile_type_2da_string);
	
	load_item_visual_effect_org = (int (*)(void*, uint8_t))hook_function(0x0819A690, (long)load_item_visual_effect_hook, 12);
	
	create_item_org = (CNWCItem*(*)(CNWMessage*, uint32_t, uint8_t*, uint8_t*, char, uint8_t))hook_function(0x08149478, (long)create_item_hook, 12);
	
	hook_call(0x08155B57, (long)on_update_creature_add_to_dm_party);
	hook_call(0x080C20A7, (long)on_update_creature_add_to_dm_party);
	hook_call(0x08160644, (long)on_player_list_change_append_to_msg_buffer);
	hook_call(0x08161411, (long)on_player_list_change_append_to_msg_buffer);
	hook_call(0x08160C41, (long)on_player_list_change_read_obj_id);
	hook_call(0x08160BDF, (long)on_player_list_change_read_obj_id);
	
	handle_server_to_player_dialog_org = (int (*)(void*, uint8_t))hook_function(0x0814B95C, (long)handle_server_to_player_dialog_hook, 12);
	update_gui_and_render_org = (void (*)(void*, float))hook_function(0x084B61E4, (long)update_gui_and_render_hook, 12);
	
	parse_str_org = (void (*)(void*, CExoString*))hook_function(0x0858D078, (long)parse_str_hook, 12);
	
	//bypass gamespy connection
	new_server_list_panel_org = (void*(*)(void*))hook_function(0x080924FC, (long)new_server_list_panel_hook, 12);
	enable_write(0x08095EDF);
	*(uint8_t*)0x08095EDF = 0xEB;
	
	//custom resman
	add_encapsulated_resource_file_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858AD38, (long)add_encapsulated_resource_file_hook, 13);
	add_resource_image_file_org = (int (*)(CExoResMan*, CExoString*, uint8_t*))hook_function(0x0858AD54, (long)add_resource_image_file_hook, 14);
	add_fixed_key_table_file_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858AD70, (long)add_fixed_key_table_file_hook, 13);
	add_resource_directory_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858AD8C, (long)add_resource_directory_hook, 13);
	remove_encapsulated_resource_file_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858ADA8, (long)remove_encapsulated_resource_file_hook, 11);
	remove_resource_image_file_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858ADC0, (long)remove_resource_image_file_hook, 11);
	remove_fixed_key_table_file_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858ADD8, (long)remove_fixed_key_table_file_hook, 11);
	remove_resource_directory_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858ADF0, (long)remove_resource_directory_hook, 11);
	update_resource_directory_org = (int (*)(CExoResMan*, CExoString*))hook_function(0x0858AE38, (long)update_resource_directory_hook, 11);
	destroy_res_org = (void (*)(CRes* NWNCX_DESTRUCTOR_P2))hook_function(0x0858ABBC, (long)destroy_res_hook, 12);
	get_table_count_org = (uint32_t (*)(CExoResMan*, CRes*, int))hook_function(0x08587BD8, (long)get_table_count_hook, 12);
	get_res_of_type_org = (CExoStringList* (*)(CExoResMan*, uint16_t, CRes*))hook_function(0x0858AF48, (long)get_res_of_type_hook, 13);
	resource_exists_org = (int (*)(CExoResMan*, CResRef*, uint16_t, uint32_t*))hook_function(0x0858AE90, (long)resource_exists_hook, 12);
	release_res_org = (int (*)(CExoResMan*, CRes*))hook_function(0x08587CB0, (long)release_res_hook, 12);
	release_res_object_org = (int (*)(CExoResMan*, CRes*))hook_function(0x0858B29C, (long)release_res_object_hook, 13);
	hook_call(0x0858FAC7, (long)on_release_res_before_destroying_it);
	hook_call(0x085929AA, (long)on_release_res_before_destroying_it);
	free_res_org = 	(int (*)(CExoResMan*, CRes*))hook_function(0x0858710C, (long)free_res_hook, 12);
	free_res_data_org = (void (*)(CExoResMan*, CRes*))hook_function(0x0858AEE4, (long)free_res_data_hook, 13);
	free_chunk_org = (int (*)(CExoResMan*))hook_function(0x085871D0, (long)free_chunk_hook, 12);
	request_res_org = (int(*)(CRes*))hook_function(0x0858B304, (long)request_res_hook, 12);
	cancel_request_res_org = (int(*)(CRes*))hook_function(0x08586838, (long)cancel_request_res_hook, 11);
	demand_res_org = (void* (*)(CExoResMan*, CRes*))hook_function(0x0858695C, (long)demand_res_hook, 12);
	get_res_object_org = (CRes* (*)(CExoResMan*, CResRef*, uint16_t))hook_function(0x0858AF84, (long)get_res_object_hook, 14);
	set_res_object_org = (void (*)(CExoResMan*, CResRef*, uint16_t, CRes*))hook_function(0x0858AFD4, (long)set_res_object_hook, 12);
	resman_update_org = (void (*)(CExoResMan*, uint32_t))hook_function(0x08588CA4, (long)resman_update_hook, 12);//*/

	*(long*)&replace_texture_on_object_org = (long)hook_function(0x081B05DC, (long)replace_texture_on_object_hook, 12);
	enable_write(0x0829153B);
	enable_write(0x082915F1);
	cnwcanimbase_replace_texture_org = (int (*)(CNWCAnimBase*, CResRef, CResRef, uint32_t, uint16_t*, int, uint32_t))hook_function(0x082835B8, (long)cnwcanimbase_replace_texture_hook, 12);
	hook_call(0x08153C1C, (long)on_update_creature_appearance_create_item);
	hook_call(0x08153BE0, (long)on_update_creature_appearance_get_item_by_id);
	create_cloak_org = (int (*)(CNWCCreatureAppearance*, uint8_t, CCreatureAppearanceInfo*))hook_function(0x08290CFC, (long)create_cloak_hook, 12);
	
	//gob creation and destruction
	hook_call(0x084E4B14, (long)alloc_gob_hook);
	destroy_caurobject_org = (void* (*)(ExtendedGob*, int))hook_function(0x084E0900, (long)destroy_caurobject_hook, 12);
	
	replace_texture_on_gob_sub_tree = (int (*)(Gob*, char*, char*, int, uint16_t*))hook_function((long)replace_texture_on_gob_sub_tree, (long)replace_texture_on_gob_sub_tree_hook, 12);
	restore_texture_on_object_org = (int (*)(CNWCObject*))hook_function(0x081B09B0, (long)restore_texture_on_object_hook, 12);
	hook_call(0x081D53B6, (long)on_apply_vfx_on_object_get_helmet_id);
	
	destroy_game_object_array_org = (void (*)(CGameObjectArray*, int))hook_function(0x080A8588, (long)destroy_game_object_array_hook, 12);
	
	hook_call(0x080C7DB5, (long)on_build_player_list_get_player_at_pos);
	
	//camera hack
	if (ini_enable_camera_hack)
	{
		enable_write(0x081A6D5C);
		*(float*)(0x081A6D5C + 6) = ini_cam_min_zoom;
		*(float*)(0x081A6D66 + 6) = ini_cam_max_zoom;
		*(float*)(0x081A6D70 + 6) = ini_cam_min_pitch;
		*(float*)(0x081A6D7A + 6) = ini_cam_max_pitch;
		enable_write(0x081A6A5C);
		*(float*)(0x081A6A5C + 6) = ini_cam_min_zoom;
		*(float*)(0x081A6A66 + 6) = ini_cam_max_zoom;
		*(float*)(0x081A6A70 + 6) = ini_cam_min_pitch;
		*(float*)(0x081A6A7A + 6) = ini_cam_max_pitch;
		enable_write(0x081A6444);
		*(float*)(0x081A6444 + 6) = ini_cam_min_zoom;
		*(float*)(0x081A644E + 6) = ini_cam_max_zoom;
		*(float*)(0x081A6458 + 6) = ini_cam_min_pitch;
		*(float*)(0x081A6462 + 6) = ini_cam_max_pitch;
		enable_write(0x0807AB3B + 1);
		*(float**)(0x0807AB3B + 1) = &ini_cam_default_min_zoom;
		enable_write(0x0807ABAF + 1);
		*(float**)(0x0807ABAF + 1) = &ini_cam_default_max_zoom;
	}
	
	//unlock effect icons
	enable_write(0x080C4DF9);
	*(uint8_t*)0x080C4DF9 = 0xFD;
	
	//always reset flag when freeing GFF data
	enable_write(0x0859292E);
	*(uint16_t*)0x0859292E = 0x9090;
	
	//custom miles folder to share the same installation as windows
	enable_write(0x8581440);
	*(const char**)0x8581440 = miles_folder;
}

CNWNCXSinfar::~CNWNCXSinfar()
{
}