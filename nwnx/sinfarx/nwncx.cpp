#include "nwncx.h"
#include "nwscript.h"
#include "xdata.h"
#include "creature.h"
#include "player.h"

#include <boost/algorithm/string.hpp>

using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::creature;
using namespace nwnx::player;

namespace nwnx { namespace nwncx {

uint16_t get_nwncx_version(CNWSPlayer* player)
{
	return players_extra_info[player->pl_id].nwncx_version;
}
const int latest_nwncx_protocol_version = 385;

CNWSPlayer* sending_obj_update_player = NULL;
uint16_t sending_obj_update_nwncx_version = 0;
unsigned char d_ret_code_upateclientgo[0x20];
int (*UpdateClientGameObjectsForPlayer_Org)(CServerExoAppInternal*, CNWSPlayer*, int, unsigned long long) = (int (*)(CServerExoAppInternal*, CNWSPlayer*, int, unsigned long long))&d_ret_code_upateclientgo;
int UpdateClientGameObjectsForPlayer_Hook(CServerExoAppInternal* server_internal, CNWSPlayer* player, int p3, unsigned long long p4)
{
	sending_obj_update_player = player;
	sending_obj_update_nwncx_version = get_nwncx_version(player);
	return UpdateClientGameObjectsForPlayer_Org(server_internal, player, p3, p4);
}

#define NWNCX_SINFAR_PLC_HAS_XDATA 0x40000000
uint16_t sending_client_area_nwncx_version = 0;
void OnPackArea_WritePlaceableId(CNWSMessage* message, uint32_t plc_id)
{
	if (sending_client_area_nwncx_version >= latest_nwncx_protocol_version)
	{
		CNWSPlaceable* plc = GetPlaceableById(plc_id);
		if (plc)
		{
			const char* xdata_raw = get_local_string(&plc->obj.obj_vartable, "XDATA");
			if (xdata_raw && *xdata_raw)
			{
				PC_PART_XDATA pc_part_xdata;
				pc_part_xdata.read_pc_part_xdata(xdata_raw);
				std::string xdata = pc_part_xdata.get_pc_part_xdata();
				xdata_raw = xdata.c_str();
				if (xdata_raw && *xdata_raw)
				{
					plc_id |= NWNCX_SINFAR_PLC_HAS_XDATA;
					WriteObjectId(message, plc_id);
					while (*xdata_raw)
					{
						WriteByte((CNWMessage*)message, read_pc_part_xdata_byte(xdata_raw), 0x8);
					}
					return;
				}
			}
		}
	}
	WriteObjectId(message, plc_id);
}

struct CLoopingVFX
{
	uint16_t vfx;
	uint16_t spacer_0x2;
	uint32_t beam_obj_id; //0x4
	uint8_t beam_type; //0x8
	uint8_t spacer_0x9;
	uint16_t spacer_0xA;
	char xdata[1]; //0xC
};
std::vector<CLoopingVFX*> last_vfx_to_delete;
std::vector<CLoopingVFX*> last_vfx_to_add;
int CompareVisualEffectLists_Hook(CNWSMessage* message, CExoArrayList<void*>& luo_visuals, CExoArrayList<void*>& obj_visuals)
{
	last_vfx_to_delete.clear();
	last_vfx_to_add.clear();
	for (uint32_t i=0; i<luo_visuals.len; i++)
	{
		CLoopingVFX* luo_vfx = (CLoopingVFX*)luo_visuals.data[i];
		uint32_t j;
		for (j=0; j<obj_visuals.len; j++)
		{
			CLoopingVFX* obj_vfx = (CLoopingVFX*)obj_visuals.data[j];
			if (luo_vfx->vfx == obj_vfx->vfx &&
				luo_vfx->beam_obj_id == obj_vfx->beam_obj_id &&
				luo_vfx->beam_type == obj_vfx->beam_type &&
				strcmp(luo_vfx->xdata, obj_vfx->xdata) == 0) break;
		}
		if (j == obj_visuals.len) //vfx not on obj anymore
		{
			last_vfx_to_delete.push_back(luo_vfx);
		}
	}
	for (uint32_t i=0; i<obj_visuals.len; i++)
	{
		CLoopingVFX* obj_vfx = (CLoopingVFX*)obj_visuals.data[i];
		uint32_t j;
		for (j=0; j<luo_visuals.len; j++)
		{
			CLoopingVFX* luo_vfx = (CLoopingVFX*)luo_visuals.data[j];
			if (luo_vfx->vfx == obj_vfx->vfx &&
				luo_vfx->beam_obj_id == obj_vfx->beam_obj_id &&
				luo_vfx->beam_type == obj_vfx->beam_type &&
				strcmp(luo_vfx->xdata, obj_vfx->xdata) == 0) break;
		}
		if (j == luo_visuals.len) //vfx not on obj anymore
		{
			last_vfx_to_add.push_back(obj_vfx);
		}
	}
	return (last_vfx_to_add.size() == 0 && last_vfx_to_delete.size() == 0);
}
void StoreLoopingVFXInLUO(CNWSMessage*, CExoArrayList<void*>* to, CExoArrayList<void*>* from)
{
	if (to->data)
	{
		for (uint32_t i=0; i<to->len; i++)
		{
			free(to->data[i]);
		}
		free(to->data);
		to->len = 0;
		to->alloc = 0;
	}

	if (from->data)
	{
		int data_size = from->len*4;
		to->data = (void**)malloc(data_size);
		for (uint32_t i=0; i<from->len; i++)
		{
			CLoopingVFX* from_looping_vfx = (CLoopingVFX*)from->data[i];
			uint32_t from_looping_vfx_size = sizeof(CLoopingVFX)+strlen(from_looping_vfx->xdata);
			CLoopingVFX* new_looping_vfx = (CLoopingVFX*)malloc(from_looping_vfx_size);
			memcpy(new_looping_vfx, from_looping_vfx, from_looping_vfx_size);
			to->data[i] = new_looping_vfx;
		}
		to->len = from->len;
		to->alloc = from->len;
	}
}
CGameEffect* last_looping_vfx_effect = NULL;
unsigned char d_ret_code_onapplyvfx[0x20];
int (*OnApplyVisualEffect_Org)(CNWSEffectListHandler*, CNWSObject*, CGameEffect*, int) = (int (*)(CNWSEffectListHandler*, CNWSObject*, CGameEffect*, int))&d_ret_code_onapplyvfx;
int OnApplyVisualEffect_Hook(CNWSEffectListHandler* handler, CNWSObject* object, CGameEffect* effect, int p4)
{
	last_looping_vfx_effect = effect;
	return OnApplyVisualEffect_Org(handler, object, effect, p4);
}
unsigned char d_ret_code_onapplyvfxatloc[0x20];
void (*ApplyEffectAtLocation_Org)(CNWSArea*, CGameEffect*, Vector, Vector) = (void (*)(CNWSArea*, CGameEffect*, Vector, Vector))&d_ret_code_onapplyvfxatloc;
void ApplyEffectAtLocation_Hook(CNWSArea* area, CGameEffect* effect, Vector position, Vector orientation)
{
	last_looping_vfx_effect = effect;
	return ApplyEffectAtLocation_Org(area, effect, position, orientation);
}
int (*GetIsBeam)(CLoopingVFX*) = (int (*)(CLoopingVFX*))0x81C8800;
CLoopingVFX* OnApplyVisualEffect_AllocLoopingVFX(int size)
{
	const char* xdata = "";
	if (last_looping_vfx_effect->extra)
	{
		xdata = last_looping_vfx_effect->extra->xdata.c_str();
	}
	CLoopingVFX* ret = (CLoopingVFX*)malloc(sizeof(CLoopingVFX)+strlen(xdata));
	strcpy(ret->xdata, xdata);
	return ret;
}
#define NWNCX_SINFAR_VFX_ADJUSTED 0x8000
inline void write_vfx_with_xdata(CNWSMessage* message, uint16_t vfx, const std::string& xdata)
{
	if (!xdata.empty()) vfx |= NWNCX_SINFAR_VFX_ADJUSTED;
	WriteWord((CNWMessage*)message, vfx, 0x10);
	if (vfx & NWNCX_SINFAR_VFX_ADJUSTED)
	{
		const char* xdata_raw = xdata.c_str();
		while (*xdata_raw)
		{
			WriteByte((CNWMessage*)message, read_pc_part_xdata_byte(xdata_raw), 0x8);
		}
	}
}
void write_looping_vfx_add_or_remove(CNWSMessage* message, CLoopingVFX* looping_vfx, CNWSPlayer* player, int add_or_remove)
{
	std::string xdata;
	if (get_nwncx_version(player) >= latest_nwncx_protocol_version)
	{
		if (looping_vfx->xdata[0])
		{
			xdata = looping_vfx->xdata;
		}
	}
	WriteByte((CNWMessage*)message, add_or_remove, 0x8);
	write_vfx_with_xdata(message, looping_vfx->vfx, xdata);
	if (GetIsBeam(looping_vfx))
	{
		WriteObjectId(message, looping_vfx->beam_obj_id);
		WriteByte((CNWMessage*)message, looping_vfx->beam_type, 0x8);
	}
}
unsigned char d_ret_code_writeobjupdate[0x20];
void (*WriteGameObjectUpdate_UpdateObject_Org)(CNWSMessage*, CNWSPlayer*, CNWSObject*, CLastUpdateObject*, uint32_t, uint32_t) = (void (*)(CNWSMessage*, CNWSPlayer*, CNWSObject*, CLastUpdateObject*, uint32_t, uint32_t))&d_ret_code_writeobjupdate;
CNWSPlayer* WriteGameObjectUpdate_UpdateObject_player = NULL;
CNWSObject* WriteGameObjectUpdate_UpdateObject_object = NULL;
void WriteGameObjectUpdate_UpdateObject_HookProc(CNWSMessage* message, CNWSPlayer* player, CNWSObject* object, CLastUpdateObject* luo, uint32_t flags, uint32_t p6)
{
	if (flags & 8)
	{
		WriteGameObjectUpdate_UpdateObject_player = player;
		WriteGameObjectUpdate_UpdateObject_object = object;
		uint32_t saved_obj_visuals_len = object->obj_visuals.len;
		uint32_t saved_luo_visuals_len = luo->visuals.len;
		object->obj_visuals.len = 0;
		luo->visuals.len = 0;
		WriteGameObjectUpdate_UpdateObject_Org(message, player, object, luo, flags, p6);
		object->obj_visuals.len = saved_obj_visuals_len;
		luo->visuals.len = saved_luo_visuals_len;
	}
	else
	{
		WriteGameObjectUpdate_UpdateObject_Org(message, player, object, luo, flags, p6);
	}
}
void WriteNumAddedOrRemovedVFX(CNWSMessage* message, uint16_t value, int num_bits)
{
	if (value != 0)
	{
		WriteWord((CNWMessage*)message, 0, 0x10);
		fprintf(stderr, "WriteNumAddedOrRemovedVFX value != 0");
	}
	else
	{
		WriteWord((CNWMessage*)message, last_vfx_to_delete.size()+last_vfx_to_add.size(), 0x10);
		for (CLoopingVFX* looping_vfx : last_vfx_to_delete)
		{
			write_looping_vfx_add_or_remove(message, looping_vfx, WriteGameObjectUpdate_UpdateObject_player, 0x44);
		}
		for (CLoopingVFX* looping_vfx : last_vfx_to_add)
		{
			write_looping_vfx_add_or_remove(message, looping_vfx, WriteGameObjectUpdate_UpdateObject_player, 0x41);
		}
	}
}
void OnWriteInstantVFX_AddXDATA(CNWSMessage* message, uint16_t value, int num_bits)
{
	if (last_looping_vfx_effect->extra)
	{
		write_vfx_with_xdata(message, value, last_looping_vfx_effect->extra->xdata);
	}
	else
	{
		WriteWord((CNWMessage*)message, value, num_bits);
	}

}
int OnRemoveVFX_Hook(void* effects_list, CNWSObject* object, CGameEffect* effect)
{
	uint32_t vfx = effect->eff_integers[0];
	const char* xdata = effect->extra ? effect->extra->xdata.c_str() : "";
	uint32_t visuals_len = object->obj_visuals.len;
	for (uint32_t i=0; i<visuals_len; i++)
	{
		CLoopingVFX* looping_vfx = (CLoopingVFX*)object->obj_visuals.data[i];
		if (looping_vfx->vfx == vfx && strcmp(looping_vfx->xdata, xdata)==0)
		{
			free(looping_vfx);
			object->obj_visuals.delindex(i);
			break;
		}
	}
	return 1;
}
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
bool is_body_part_model(uint8_t model)
{
	return (model <3 || model == 255);
}
void merge_pc_part_xdata(PC_PART_XDATA& from, PC_PART_XDATA& to)
{
	//merge colours mapping
	for (int channel_index=0; channel_index<NUM_PLT_COLOR_CHANNELS; channel_index++)
	{
		if (from.channels_mappping[channel_index] != 0xFF ||
			from.channels_lightness_mod[channel_index] != 0)
		{
			to.channels_mappping[channel_index] = from.channels_mappping[channel_index];
			to.channels_lightness_mod[channel_index] = from.channels_lightness_mod[channel_index];
		}
		if (from.channels_color_override & (1<<channel_index))
		{
			to.channels_color_override |= (1<<channel_index);
			to.channels_color[channel_index] = from.channels_color[channel_index];
		}
	}
	//merge scaling/moving/flipping
	if (from.scaling != 1.0 ||
		from.extra_x != 0.0 ||
		from.extra_y != 0.0 ||
		from.extra_z != 0.0 ||
		from.extra_rotate_x != 0.0 ||
		from.extra_rotate_y != 0.0 ||
		from.extra_rotate_z != 0.0)
	{
		to.scaling = from.scaling;
		to.extra_x = from.extra_x;
		to.extra_y = from.extra_y;
		to.extra_z = from.extra_z;
		to.extra_rotate_x = from.extra_rotate_x;
		to.extra_rotate_y = from.extra_rotate_y;
		to.extra_rotate_z = from.extra_rotate_z;
	}
}
bool get_armor_xdata(CNWSObject* object, PC_PARTS_XDATA& pc_parts_xdata)
{
	const char* xdata_raw = get_local_string(&object->obj_vartable, "XDATA");
	if (xdata_raw && *xdata_raw)
	{
		uint8_t pc_parts_with_xdata = read_pc_part_xdata_byte(xdata_raw);
		pc_parts_xdata.read_pc_parts_xdata(pc_parts_with_xdata, xdata_raw);
		return true;
	}
	return false;
}
bool get_body_xdata(CNWSObject* object, PC_PART_XDATA& body_xdata)
{
	const char* xdata_raw = get_local_string(&object->obj_vartable, "BODY_XDATA");
	if (xdata_raw && *xdata_raw)
	{
		body_xdata.read_pc_part_xdata(xdata_raw);
		return true;
	}
	return false;
}
bool get_item_xdata(CNWSItem* item, ITEM_PARTS_XDATA& item_parts_xdata)
{
	const char* xdata_raw = get_local_string(&item->obj.obj_vartable, "ITEM_XDATA");
	if (xdata_raw && *xdata_raw)
	{
		item_parts_xdata.read_item_parts_xdata(xdata_raw);
		return true;
	}
	return false;
}

void OnSendCreatureAppearance_WriteObjectId(CNWSMessage* message, uint32_t object_id)
{
	WriteObjectId(message, object_id);
	if (sending_obj_update_nwncx_version < latest_nwncx_protocol_version)
	{
		if (sending_obj_update_nwncx_version > 206)
		{
			WriteByte((CNWMessage*)message, 0, 0x8);
		}
		return;
	}

	CNWSCreature* creature = GetCreatureById(object_id);
	uint8_t extra_data = 0;
	uint8_t extra_armor_data = 0;
	uint16_t chest_model = 0;
	uint16_t pelvis_model = 0;
	std::string body_xdata_string;
	const char* body_xdata_raw = NULL;
	std::string armor_xdata_string;
	const char* armor_xdata_raw = NULL;
	std::string lhand_xdata;
	std::string rhand_xdata;
	CREATURE_EXTRA* creature_extra = NULL;
	CREATURE_EXTRA_ATTACHED* attach_data = NULL;
	if (creature)
	{
		creature_extra = GetCreatureExtra(creature);
		MyCLastUpdateObject* luo = (MyCLastUpdateObject*)get_last_update_object(sending_obj_update_player, object_id);
		CNWSItem* pcskin = get_item_in_slot(creature->cre_equipment, 131072);
		CNWSObject* pcstorage = (pcskin ? &(pcskin->obj) : (CNWSObject*)creature);

		//body_xdata
		PC_PART_XDATA body_xdata;
		get_body_xdata((CNWSObject*)creature, body_xdata);
		body_xdata_string = body_xdata.get_pc_part_xdata();
		if (luo->sf.body_xdata != body_xdata_string)
		{
			luo->sf.body_xdata = body_xdata_string;
			body_xdata_raw = body_xdata_string.c_str();
			extra_data |= NWNCX_SINFAR_CREATURE_XDATA_BODY;
		}

		//extra armor appearances
		CNWSItem* armor = get_item_in_slot(creature->cre_equipment, 2);
		CNWSObject* xchest_var_object = pcstorage;
		if (armor && !is_body_part_model(armor->it_model[10])) xchest_var_object = &armor->obj;
		chest_model = get_local_int(xchest_var_object, "XCHEST");
		CNWSObject* xpelvis_var_object = pcstorage;
		if (armor && !is_body_part_model(armor->it_model[9])) xpelvis_var_object = &armor->obj;
		pelvis_model = get_local_int(xpelvis_var_object, "XPELVIS");
		//xdata
		PC_PARTS_XDATA pc_parts_xdata1;
		if (get_armor_xdata(pcstorage, pc_parts_xdata1))
		{
			//remove the wings/tail data if the PC is not using the skin wings/tail
			if (creature->cre_stats->cs_wings == 0 || creature->cre_stats->cs_wings != (uint32_t)get_local_int(pcstorage, "WING"))
			{
				pc_parts_xdata1.parts[21].clear();
			}
			if (creature->cre_stats->cs_tail == 0 || creature->cre_stats->cs_tail != (uint32_t)get_local_int(pcstorage, "TAIL"))
			{
				pc_parts_xdata1.parts[22].clear();
			}
		}
		if (pcstorage != &creature->obj)
		{
			PC_PARTS_XDATA creature_xdata;
			if (get_armor_xdata(&creature->obj, creature_xdata))
			{
				for (int model_index=0; model_index<creature_xdata.get_num_parts(); model_index++)
				{
					merge_pc_part_xdata(creature_xdata[model_index], pc_parts_xdata1[model_index]);
				}
			}
		}
		if (armor)
		{
			PC_PARTS_XDATA armor_xdata;
			if (get_armor_xdata(&armor->obj, armor_xdata))
			{
				for (int model_index=0; model_index<ITEM_APPR_ARMOR_NUM_MODELS; model_index++)
				{
					if (!is_body_part_model(armor->it_model[3+model_index]))
					{
						merge_pc_part_xdata(armor_xdata[model_index], pc_parts_xdata1[model_index]);
					}
				}
			}
		}
		//helmet xdata
		CNWSItem* helmet = get_item_in_slot(creature->cre_equipment, 1);
		if (helmet && !get_local_int((CNWSObject*)creature, "HIDE_HELMET"))
		{
			PC_PARTS_XDATA helmet_xdata;
			if (get_armor_xdata(&helmet->obj, helmet_xdata))
			{
				pc_parts_xdata1[19] = helmet_xdata[19];
			}
		}
		//cloak xdata
		CNWSItem* cloak = get_item_in_slot(creature->cre_equipment, 64);
		if (cloak)
		{
			PC_PARTS_XDATA cloak_xdata;
			if (get_armor_xdata(&cloak->obj, cloak_xdata))
			{
				pc_parts_xdata1[20] = cloak_xdata[20];
			}
		}
		//final xdata
		armor_xdata_string = pc_parts_xdata1.get_pc_parts_xdata();
		if (luo->sf.armor_xdata != armor_xdata_string ||
			luo->sf.xchest != chest_model ||
			luo->sf.xpelvis != pelvis_model)
		{
			luo->sf.armor_xdata = armor_xdata_string;
			luo->sf.xchest = chest_model;
			luo->sf.xpelvis = pelvis_model;
			armor_xdata_raw = armor_xdata_string.c_str();
			extra_armor_data |= read_pc_part_xdata_byte(armor_xdata_raw);
			if (chest_model)
			{
				extra_armor_data |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XCHEST;
			}
			if (pelvis_model)
			{
				extra_armor_data |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XPELVIS;
			}
			extra_data |= NWNCX_SINFAR_CREATURE_XDATA_ARMOR;
		}

		//attach to
		if (creature_extra->attached_to != OBJECT_INVALID)
		{
			CNWSCreature* creature_attached_to = GetCreatureById(creature_extra->attached_to);
			if (creature_attached_to)
			{
				attach_data = GetCreatureExtra(creature_attached_to)->attached_by;
				if (attach_data)
				{
					extra_data |= NWNCX_SINFAR_CREATURE_EXTRA_ATTACH;
				}
			}
		}

		//equipped items
		CNWSItem* rhand_item = get_item_in_slot(creature->cre_equipment, 0x10);
		if (rhand_item)
		{
			ITEM_PARTS_XDATA item_parts_xdata;
			get_item_xdata(rhand_item, item_parts_xdata);
			if (item_parts_xdata[0].scaling == 1.0)
			{
				float rhand_scaling = get_local_float(&rhand_item->obj, "XSCALE");
				if (rhand_scaling > 0.0 && item_parts_xdata[0].scaling == 1.0) item_parts_xdata[0].scaling = rhand_scaling;
			}
			rhand_xdata = item_parts_xdata.get_item_parts_xdata();
			if (luo->sf.rhand_xdata != rhand_xdata)
			{
				extra_data |= NWNCX_SINFAR_CREATURE_XDATA_RHAND;
				luo->sf.rhand_xdata = rhand_xdata;
			}
		}
		CNWSItem* lhand_item = get_item_in_slot(creature->cre_equipment, 0x20);
		if (lhand_item)
		{
			ITEM_PARTS_XDATA item_parts_xdata;
			get_item_xdata(lhand_item, item_parts_xdata);
			if (item_parts_xdata[0].scaling == 1.0)
			{
				float lhand_scaling = get_local_float(&lhand_item->obj, "XSCALE");
				if (lhand_scaling > 0.0 && item_parts_xdata[0].scaling == 1.0) item_parts_xdata[0].scaling = lhand_scaling;
			}
			lhand_xdata = item_parts_xdata.get_item_parts_xdata();
			if (luo->sf.lhand_xdata != lhand_xdata)
			{
				extra_data |= NWNCX_SINFAR_CREATURE_XDATA_LHAND;
				luo->sf.lhand_xdata = lhand_xdata;
			}
		}
	}
	WriteByte((CNWMessage*)message, extra_data, 0x8);
	if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_BODY)
	{
		if (body_xdata_raw && *body_xdata_raw)
		{
			while (*body_xdata_raw)
			{
				WriteByte((CNWMessage*)message, read_pc_part_xdata_byte(body_xdata_raw), 0x8);
			}
		}
		else
		{
			WriteByte((CNWMessage*)message, 0, 0x8);
		}
	}
	if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_ARMOR)
	{
		WriteByte((CNWMessage*)message, extra_armor_data, 0x8);
		if (extra_armor_data & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XCHEST)
		{
			WriteWord((CNWMessage*)message, chest_model, 0x10);
		}
		if (extra_armor_data & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XPELVIS)
		{
			WriteWord((CNWMessage*)message, pelvis_model, 0x10);
		}
		if (armor_xdata_raw && *armor_xdata_raw)
		{
			while (*armor_xdata_raw)
			{
				WriteByte((CNWMessage*)message, read_pc_part_xdata_byte(armor_xdata_raw), 0x8);
			}
		}
	}
	if (extra_data & NWNCX_SINFAR_CREATURE_EXTRA_ATTACH)
	{
		WriteObjectId(message, creature_extra->attached_to);
		WriteFloat((CNWMessage*)message, attach_data->attach_x, 1.0, 0x20);
		WriteFloat((CNWMessage*)message, attach_data->attach_y, 1.0, 0x20);
		WriteFloat((CNWMessage*)message, attach_data->attach_z, 1.0, 0x20);
	}
	if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_LHAND)
	{
		const char* item_xdata_raw = lhand_xdata.c_str();
		while (*item_xdata_raw)
		{
			WriteByte((CNWMessage*)message, read_pc_part_xdata_byte(item_xdata_raw), 0x8);
		}
	}
	if (extra_data & NWNCX_SINFAR_CREATURE_XDATA_RHAND)
	{
		const char* item_xdata_raw = rhand_xdata.c_str();
		while (*item_xdata_raw)
		{
			WriteByte((CNWMessage*)message, read_pc_part_xdata_byte(item_xdata_raw), 0x8);
		}
	}
}

void* OnAllocCLastUpdateObject_Extend(uint32_t size)
{
	size += sizeof(MyCLastUpdateObjectExtra);
	MyCLastUpdateObject* my_luo = new MyCLastUpdateObject;
	return my_luo;
}
unsigned char d_ret_code_destroy_luo[0x20];
int (*DestroyMyCLastUpdateObject_Org)(MyCLastUpdateObject*, int) = (int (*)(MyCLastUpdateObject*, int))&d_ret_code_destroy_luo;
int DestroyMyCLastUpdateObject_Hook(MyCLastUpdateObject* my_luo, int p2)
{
	my_luo->sf.~MyCLastUpdateObjectExtra();
	return DestroyMyCLastUpdateObject_Org(my_luo, p2);
}

unsigned char d_ret_code_compareitems[0x20];
int (*CompareItems_Org)(CNWSItem*, CNWSItem*) = (int (*)(CNWSItem*, CNWSItem*))&d_ret_code_compareitems;
int CompareItems_HookProc(CNWSItem* item1, CNWSItem* item2)
{
	if (get_local_int(&item1->obj, "XCHEST") != get_local_int(&item2->obj, "XCHEST"))
	{
		return 0;
	}

	if (get_local_int(&item1->obj, "XPELVIS") != get_local_int(&item2->obj, "XPELVIS"))
	{
		return 0;
	}

	return CompareItems_Org(item1, item2);
}

void init()
{
	//alloc CLastUpdateObject
	hook_function(0x081E2FDC, (uint32_t)DestroyMyCLastUpdateObject_Hook, d_ret_code_destroy_luo, 12);
	hook_call(0x0806BCEC, (uint32_t)OnAllocCLastUpdateObject_Extend);

	//save the player receiving the game objects update
	hook_function(0x080a4a70, (unsigned long)UpdateClientGameObjectsForPlayer_Hook, d_ret_code_upateclientgo, 12);

	hook_call(0x080D1EEF, (uint32_t)OnPackArea_WritePlaceableId);
	hook_call(0x080D1DBB, (uint32_t)OnPackArea_WritePlaceableId);

	//more strict update of vfx
	hook_call(0x08077F56, (uint32_t)StoreLoopingVFXInLUO);
	hook_function(0x80626AC, (uint32_t)CompareVisualEffectLists_Hook, d_ret_code_nouse, 11);
	//save the looping vfx object
	hook_function(0x08174AAC, (uint32_t)OnApplyVisualEffect_Hook, d_ret_code_onapplyvfx, 12);
	hook_function(0x080D3AF0, (uint32_t)ApplyEffectAtLocation_Hook, d_ret_code_onapplyvfxatloc, 12);
	//add xdata to the looping vfx object
	hook_call(0x081D4EB2, (uint32_t)OnApplyVisualEffect_AllocLoopingVFX);
	//send vfx xdata
	hook_function(0x08071a24, (uint32_t)WriteGameObjectUpdate_UpdateObject_HookProc, d_ret_code_writeobjupdate, 12);
	hook_call(0x080725AB, (uint32_t)WriteNumAddedOrRemovedVFX);
	hook_call(0x0807B31F, (uint32_t)OnWriteInstantVFX_AddXDATA);
	hook_call(0x080655B4, (uint32_t)OnWriteInstantVFX_AddXDATA);
	//Remove the right CLoopingVFX
	hook_function(0x0817D004, (uint32_t)OnRemoveVFX_Hook, d_ret_code_nouse, 12);

	//nwncx_sinfar extra creature rendering info
	hook_call(0x0806a2e9, (long)OnSendCreatureAppearance_WriteObjectId);
    
    //check for XCHEST when comparing items
	hook_function(0x081a2ed0, (unsigned long)CompareItems_HookProc, d_ret_code_compareitems, 12);

}
REGISTER_INIT(init);

int (*VMEffectVisualEffect)(CNWVirtualMachineCommands*, int, int) = (int (*)(CNWVirtualMachineCommands*, int, int))0x0820BD38;
VM_FUNC_NEW(EffectVisualEffectWithXDATA, 255)
{
	std::string xdata = vm_pop_string();
	int vfx_created = VMEffectVisualEffect(virtual_machine->vm_cmd, 0xb4, 2);
	if (vfx_created == 0 && !xdata.empty())
	{
		CGameEffect* vfx_effect = (CGameEffect*)virtual_machine->vm_stack.stack_data_values[virtual_machine->vm_stack.size-1];
		vfx_effect->extra = new CGameEffect_EXTRA;
		vfx_effect->extra->xdata = xdata;
	}
}
VM_FUNC_NEW(GetVisualEffectXDATA, 256)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
	CExoString result;
	if (effect->extra)
	{
		result = CExoString(effect->extra->xdata);
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(GetNWNCXVersion, 157)
{
	CNWSPlayer* player = get_player_by_game_object_id(vm_pop_object());
	int result = 0;
	if (player)
	{
		result = get_nwncx_version(player);
	}
	vm_push_int(result);
}
PC_PART_XDATA vm_get_armor_extra_data(uint32_t object_id, uint32_t pc_part)
{
	PC_PARTS_XDATA pc_parts_xdata;
	if (pc_part < 23)
	{
		CGameObject* go = GetGameObject(object_id);
		if (go)
		{
			CNWSScriptVarTable* var_table = NULL;
			if (go->type == OBJECT_TYPE_CREATURE)
			{
				var_table = &(go->vtable->AsNWSCreature(go)->obj.obj_vartable);
			}
			else if (go->type == OBJECT_TYPE_ITEM)
			{
				var_table = &(go->vtable->AsNWSItem(go)->obj.obj_vartable);
			}
			if (var_table)
			{
				const char* xdata = get_local_string(var_table, "XDATA");
				if (xdata)
				{
					uint8_t pc_parts_with_xdata = read_pc_part_xdata_byte(xdata);
					pc_parts_xdata.read_pc_parts_xdata(pc_parts_with_xdata, xdata);
				}
			}
		}
	}
	else
	{
		pc_part = 0;
	}
	return pc_parts_xdata.parts[pc_part];
}
int vm_get_pc_part_extra_data(PC_PART_XDATA& pc_part_xdata, int extra_info, uint32_t info_extra_param)
{
	int result = 0;
	if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORMAP)
	{
		if (info_extra_param < NUM_PLT_COLOR_CHANNELS)
		{
			result = pc_part_xdata.channels_mappping[info_extra_param];
		}
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_LIGHTMOD)
	{
		if (info_extra_param < NUM_PLT_COLOR_CHANNELS)
		{
			result = pc_part_xdata.channels_lightness_mod[info_extra_param];
		}
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORSOVERRIDE)
	{
		if (info_extra_param < NUM_PLT_COLOR_CHANNELS)
		{
			if (pc_part_xdata.channels_color_override & (1<<info_extra_param))
			{
				result = pc_part_xdata.channels_color[info_extra_param];
			}
			else
			{
				result = -1;
			}
		}
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_RGBA_OVERRIDE)
	{
		if (info_extra_param < sizeof(RGBA_MOD))
		{
			result = pc_part_xdata.rgba_mod[info_extra_param];
		}
		else
		{
			result = 0;
		}
	}
	return result;
}
VM_FUNC_NEW(GetArmorExtraData, 15)
{
	uint32_t object_id = vm_pop_object();
	unsigned int pc_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	unsigned int info_extra_param = vm_pop_int();
	PC_PART_XDATA pc_part_xdata = vm_get_armor_extra_data(object_id, pc_part);
	vm_push_int(vm_get_pc_part_extra_data(pc_part_xdata, extra_info, info_extra_param));
}
float vm_get_pc_part_extra_data_float(PC_PART_XDATA& pc_part_xdata, int extra_info)
{
	float result = 0.0;
	if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_SCALE)
	{
		result = pc_part_xdata.scaling;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEX)
	{
		result = pc_part_xdata.extra_x;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEY)
	{
		result = pc_part_xdata.extra_y;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEZ)
	{
		result = pc_part_xdata.extra_z;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEX)
	{
		result = pc_part_xdata.extra_rotate_x;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEY)
	{
		result = pc_part_xdata.extra_rotate_y;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEZ)
	{
		result = pc_part_xdata.extra_rotate_z;
	}
	return result;
}
VM_FUNC_NEW(GetArmorExtraDataFloat, 17)
{
	uint32_t object_id = vm_pop_object();
	unsigned int pc_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	PC_PART_XDATA pc_part_xdata = vm_get_armor_extra_data(object_id, pc_part);
	vm_push_float(vm_get_pc_part_extra_data_float(pc_part_xdata, extra_info));
}
void vm_set_armor_extra_data(uint32_t object_id, uint32_t pc_part, std::function<void(PC_PART_XDATA&)> set_value_func)
{
	if (pc_part < 23)
	{
		CGameObject* go = GetGameObject(object_id);
		if (go)
		{
			CNWSScriptVarTable* var_table = NULL;
			if (go->type == OBJECT_TYPE_CREATURE)
			{
				var_table = &(go->vtable->AsNWSCreature(go)->obj.obj_vartable);
			}
			else if (go->type == OBJECT_TYPE_ITEM)
			{
				var_table = &(go->vtable->AsNWSItem(go)->obj.obj_vartable);
			}
			if (var_table)
			{
				const char* xdata = get_local_string(var_table, "XDATA");
				PC_PARTS_XDATA pc_parts_xdata;
				if (xdata)
				{
					uint8_t pc_parts_with_xdata = read_pc_part_xdata_byte(xdata);
					pc_parts_xdata.read_pc_parts_xdata(pc_parts_with_xdata, xdata);
				}
				set_value_func(pc_parts_xdata.parts[pc_part]);
				std::string xdata_str = pc_parts_xdata.get_pc_parts_xdata();
				if (xdata_str.empty())
				{
					delete_local_string(var_table, "XDATA");
				}
				else
				{
					set_local_string(var_table, "XDATA", xdata_str);
				}
			}
		}
	}
}
void vm_set_pc_part_extra_data(PC_PART_XDATA& pc_part_xdata, int extra_info, uint32_t info_extra_param, int extra_data_value)
{
	if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORMAP)
	{
		if (info_extra_param < NUM_PLT_COLOR_CHANNELS)
		{
			pc_part_xdata.channels_mappping[info_extra_param] = extra_data_value;
		}
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_LIGHTMOD)
	{
		if (info_extra_param < NUM_PLT_COLOR_CHANNELS)
		{
			pc_part_xdata.channels_lightness_mod[info_extra_param] = extra_data_value;
		}
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORSOVERRIDE)
	{
		if (info_extra_param < NUM_PLT_COLOR_CHANNELS)
		{
			if (extra_data_value != -1)
			{
				pc_part_xdata.channels_color_override |= (1 << info_extra_param);
				pc_part_xdata.channels_color[info_extra_param] = extra_data_value;
			}
			else
			{
				pc_part_xdata.channels_color_override &= ~(1 << info_extra_param);
				pc_part_xdata.channels_color[info_extra_param] = 0;
			}
		}
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_RGBA_OVERRIDE)
	{
		if (info_extra_param < sizeof(RGBA_MOD))
		{
			pc_part_xdata.rgba_mod[info_extra_param] = extra_data_value;
		}
	}
}
VM_FUNC_NEW(SetArmorExtraData, 16)
{
	uint32_t object_id = vm_pop_object();
	unsigned int pc_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	unsigned int info_extra_param = vm_pop_int();
	int extra_data_value = vm_pop_int();
	vm_set_armor_extra_data(object_id, pc_part, [=](PC_PART_XDATA& pc_part_xdata)
		{
			vm_set_pc_part_extra_data(pc_part_xdata, extra_info, info_extra_param, extra_data_value);
		}
	);
}
void vm_set_pc_part_extra_data_float(PC_PART_XDATA& pc_part_xdata, int extra_info, float extra_data_value)
{
	if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_SCALE)
	{
		pc_part_xdata.scaling = extra_data_value;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEX)
	{
		pc_part_xdata.extra_x = extra_data_value;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEY)
	{
		pc_part_xdata.extra_y = extra_data_value;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEZ)
	{
		pc_part_xdata.extra_z = extra_data_value;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEX)
	{
		pc_part_xdata.extra_rotate_x = extra_data_value;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEY)
	{
		pc_part_xdata.extra_rotate_y = extra_data_value;
	}
	else if (extra_info == NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEZ)
	{
		pc_part_xdata.extra_rotate_z = extra_data_value;
	}
}
VM_FUNC_NEW(SetArmorExtraDataFloat, 18)
{
	uint32_t object_id = vm_pop_object();
	unsigned int pc_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	float extra_data_value = vm_pop_float();
	vm_set_armor_extra_data(object_id, pc_part, [=](PC_PART_XDATA& pc_part_xdata)
		{
			vm_set_pc_part_extra_data_float(pc_part_xdata, extra_info, extra_data_value);
		}
	);
}
PC_PART_XDATA vm_get_item_extra_data(uint32_t item_id, uint32_t item_part)
{
	ITEM_PARTS_XDATA item_parts_xdata;
	if (item_part < 4)
	{
		CNWSItem* item = GetItemById(item_id);
		if (item)
		{
			get_item_xdata(item, item_parts_xdata);
		}
		return item_parts_xdata[item_part];
	}
	else
	{
		return item_parts_xdata[0];
	}
}
void vm_set_item_extra_data(uint32_t item_id, uint32_t item_part, std::function<void(PC_PART_XDATA&)> set_value_func)
{
	if (item_part < 4)
	{
		CNWSItem* item = GetItemById(item_id);
		if (item)
		{
			ITEM_PARTS_XDATA item_parts_xdata;
			get_item_xdata(item, item_parts_xdata);
			set_value_func(item_parts_xdata[item_part]);
			std::string xdata = item_parts_xdata.get_item_parts_xdata();
			if (xdata.empty() || xdata == "00")
			{
				delete_local_string(&item->obj.obj_vartable, "ITEM_XDATA");
			}
			else
			{
				set_local_string(&item->obj.obj_vartable, "ITEM_XDATA", xdata);
			}
		}
	}
}
VM_FUNC_NEW(GetItemExtraData, 139)
{
	uint32_t item_id = vm_pop_object();
	unsigned int item_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	unsigned int info_extra_param = vm_pop_int();
	PC_PART_XDATA pc_part_xdata = vm_get_item_extra_data(item_id, item_part);
	vm_push_int(vm_get_pc_part_extra_data(pc_part_xdata, extra_info, info_extra_param));
}
VM_FUNC_NEW(GetItemExtraDataFloat, 140)
{
	uint32_t item_id = vm_pop_object();
	unsigned int item_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	PC_PART_XDATA pc_part_xdata = vm_get_item_extra_data(item_id, item_part);
	vm_push_float(vm_get_pc_part_extra_data_float(pc_part_xdata, extra_info));
}
VM_FUNC_NEW(SetItemExtraData, 141)
{
	uint32_t item_id = vm_pop_object();
	unsigned int item_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	unsigned int info_extra_param = vm_pop_int();
	int extra_data_value = vm_pop_int();
	vm_set_item_extra_data(item_id, item_part, [=](PC_PART_XDATA& pc_part_xdata)
		{
			vm_set_pc_part_extra_data(pc_part_xdata, extra_info, info_extra_param, extra_data_value);
		}
	);
}
VM_FUNC_NEW(SetItemExtraDataFloat, 142)
{
	uint32_t item_id = vm_pop_object();
	unsigned int item_part = vm_pop_int();
	unsigned int extra_info = vm_pop_int();
	float extra_data_value = vm_pop_float();
	vm_set_item_extra_data(item_id, item_part, [=](PC_PART_XDATA& pc_part_xdata)
		{
			vm_set_pc_part_extra_data_float(pc_part_xdata, extra_info, extra_data_value);
		}
	);
}
VM_FUNC_NEW(GetSinglePCPartExtraData, 30)
{
	CExoString xdata_exo_str = vm_pop_string();
	unsigned int extra_info = vm_pop_int();
	unsigned int info_extra_param = vm_pop_int();
	PC_PART_XDATA pc_part_xdata;
	if (xdata_exo_str.text)
	{
		const char* xdata_raw = xdata_exo_str.text;
		pc_part_xdata.read_pc_part_xdata(xdata_raw);
	}
	vm_push_int(vm_get_pc_part_extra_data(pc_part_xdata, extra_info, info_extra_param));
}
VM_FUNC_NEW(SetSinglePCPartExtraData, 31)
{
	CExoString xdata_exo_str = vm_pop_string();
	unsigned int extra_info = vm_pop_int();
	unsigned int info_extra_param = vm_pop_int();
	int extra_data_value = vm_pop_int();
	PC_PART_XDATA pc_part_xdata;
	if (xdata_exo_str.text)
	{
		const char* xdata_raw = xdata_exo_str.text;
		pc_part_xdata.read_pc_part_xdata(xdata_raw);
	}
	vm_set_pc_part_extra_data(pc_part_xdata, extra_info, info_extra_param, extra_data_value);
	CExoString result(pc_part_xdata.get_pc_part_xdata().c_str());
	vm_push_string(&result);
}
VM_FUNC_NEW(GetSinglePCPartExtraDataFloat, 32)
{
	CExoString xdata_exo_str = vm_pop_string();
	unsigned int extra_info = vm_pop_int();
	PC_PART_XDATA pc_part_xdata;
	if (xdata_exo_str.text)
	{
		const char* xdata_raw = xdata_exo_str.text;
		pc_part_xdata.read_pc_part_xdata(xdata_raw);
	}
	vm_push_float(vm_get_pc_part_extra_data_float(pc_part_xdata, extra_info));
}
VM_FUNC_NEW(SetSinglePCPartExtraDataFloat, 33)
{
	CExoString xdata_exo_str = vm_pop_string();
	unsigned int extra_info = vm_pop_int();
	float extra_data_value = vm_pop_float();
	PC_PART_XDATA pc_part_xdata;
	if (xdata_exo_str.text)
	{
		const char* xdata_raw = xdata_exo_str.text;
		pc_part_xdata.read_pc_part_xdata(xdata_raw);
	}
	vm_set_pc_part_extra_data_float(pc_part_xdata, extra_info, extra_data_value);
	CExoString result(pc_part_xdata.get_pc_part_xdata().c_str());
	vm_push_string(&result);
}

VM_FUNC_NEW(GetEntireItemAppearance, 1)
{
	uint32_t item_id = vm_pop_object();
	std::string result;
	CNWSItem* item = GetItemById(item_id);
	if (item)
	{
	    uint8_t* colors = &item->it_color[0];
		uint8_t* models = &item->it_model[0];
		char item_appr[1000];
		sprintf(item_appr, "%02X%02X%02X%02X%02X%02X"
			"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			colors[0],colors[1],colors[2],colors[3],colors[4],colors[5],
			models[0],models[1],models[2],models[3],models[4],models[5],models[6],
			models[7],models[8],models[9],models[10],models[11],models[12],models[13],
			models[14],models[15],models[16],models[17],models[18],models[19],models[20],models[21]);
		//size_t xdata_index = 56;
		if (item->it_baseitem == 16) //armor
		{
			uint16_t xchest = get_local_int(&(item->obj.obj_vartable), "XCHEST");
			uint16_t xpelvis = get_local_int(&(item->obj.obj_vartable), "XPELVIS");
			char* x_armor_parts_buffer = &item_appr[56];
			sprintf(x_armor_parts_buffer, "%04X%04X", xchest, xpelvis);
			//xdata_index = 64;
		}
		result = item_appr;
		const char* extra_data = get_local_string(&(item->obj.obj_vartable), "XDATA");
		if (extra_data)
		{
			result += extra_data;
		}
	}
	CExoString result_exo_str(result);
	vm_push_string(&result_exo_str);
}
VM_FUNC_NEW(RestoreItemAppearance, 2)
{
	uint32_t item_id = vm_pop_object();
	std::string item_appr = vm_pop_string();
	if (item_appr.length() < 56) return;
	boost::to_upper(item_appr);
	CNWSItem* item = GetItemById(item_id);
	if (item)
	{
	    uint8_t* colors = &item->it_color[0];
		uint8_t* models = &item->it_model[0];
		sscanf(item_appr.c_str(), "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
			"%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
			colors,colors+1,colors+2,colors+3,colors+4,colors+5,
			models,models+1,models+2,models+3,models+4,models+5,models+6,
			models+7,models+8,models+9,models+10,models+11,models+12,models+13,
			models+14,models+15,models+16,models+17,models+18,models+19,models+20,models+21);
		size_t xdata_index = 56;
		if (item->it_baseitem == 16)
		{
			uint16_t xchest = 0;
			uint16_t xpelvis = 0;
			if (item_appr.length() >= 60)
			{
				sscanf(item_appr.c_str()+56, "%04hX%04hX", &xchest, &xpelvis);
				xdata_index = 64;
			}
			if (xchest)
			{
				set_local_int(&(item->obj.obj_vartable), "XCHEST", xchest);
			}
			else
			{
				delete_local_int(&(item->obj.obj_vartable), "XCHEST");
			}
			if (xpelvis)
			{
				set_local_int(&(item->obj.obj_vartable), "XPELVIS", xpelvis);
			}
			else
			{
				delete_local_int(&(item->obj.obj_vartable), "XPELVIS");
			}
		}
		std::string xdata;
		if (item_appr.length() > xdata_index)
		{
			//validate the value
			const char* xdata_raw = item_appr.c_str()+xdata_index;
			uint8_t pc_parts_with_xdata = read_pc_part_xdata_byte(xdata_raw);
			PC_PARTS_XDATA pc_parts_xdata;
			pc_parts_xdata.read_pc_parts_xdata(pc_parts_with_xdata, xdata_raw);
			xdata = pc_parts_xdata.get_pc_parts_xdata();
		}
		if (xdata.empty())
		{
			delete_local_string(&(item->obj.obj_vartable), "XDATA");
		}
		else
		{
			set_local_string(&(item->obj.obj_vartable), "XDATA", xdata);
		}
	}
}

}
}
