#include "core.h"

using namespace nwnx::core;

namespace nwnx { namespace spell {

unsigned char d_ret_code_getspellgain[0x20];
uint8_t (*GetSpellGain_Org)(void*, uint8_t, uint8_t) = (uint8_t (*)(void*, uint8_t, uint8_t))&d_ret_code_getspellgain;
uint8_t GetSpellGain_Hook(void* nwclass, uint8_t class_level, uint8_t p2)
{
	if (class_level > 40) class_level = 40;
	return GetSpellGain_Org(nwclass, class_level, p2);
}
unsigned char d_ret_code_getspellknown[0x20];
uint8_t (*GetSpellsKnownPerLevel_Org)(void*, uint8_t, uint8_t, uint8_t, uint16_t, uint8_t) = (uint8_t (*)(void*, uint8_t, uint8_t, uint8_t, uint16_t, uint8_t))&d_ret_code_getspellknown;
uint8_t GetSpellsKnownPerLevel_Hook(void* nwclass, uint8_t class_level, uint8_t p2, uint8_t p3, uint16_t p4, uint8_t p5)
{
	if (class_level > 40) class_level = 40;
	return GetSpellsKnownPerLevel_Org(nwclass, class_level, p2, p3, p4, p5);
}

void ByPassAddEvent(CServerAIMaster* server, uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4, uint32_t d5, void* data)
{
	if (data) free(data);
}
int (*SpellImpact)(CNWSObject*, uint32_t, Vector, uint32_t, uint8_t, uint32_t, int, int, uint8_t, int) = (int (*)(CNWSObject*, uint32_t, Vector, uint32_t, uint8_t, uint32_t, int, int, uint8_t, int))0x081d261c;
int CreatureCastSpell_Impact(CNWSObject* object, uint32_t d1, Vector target_location, uint32_t d2, uint8_t b1, uint32_t d3, int i1, int i2, uint8_t b2, int i3)
{
	if (object->obj_type == 5)
	{
		if (object->obj_ai_action->field_40 & 0x1000000)
		{
			*(uint32_t*)(0x081d2b0c) = ((uint32_t)ByPassAddEvent-(uint32_t)0x081d2b10);
			return 	SpellImpact(object, d1, target_location, d2, b1, d3, i1, i2, b2, i3);
		}
		else
		{
			*(uint32_t*)(0x081d2b0c) = ((uint32_t)0x08096388-(uint32_t)0x081d2b10);
			int nRet = SpellImpact(object, d1, target_location, d2, b1, d3, i1, i2, b2, i3);
			object->obj_ai_action->field_40 |=0x1000000;
			return nRet;
		}
	}
	else
	{
		*(uint32_t*)(0x081d2b0c) = ((uint32_t)0x08096388-(uint32_t)0x081d2b10);
		return SpellImpact(object, d1, target_location, d2, b1, d3, i1, i2, b2, i3);
	}
}

int (*VMPushInteger)(CVirtualMachine*, int) = (int (*)(CVirtualMachine*, int))0x826434c;
int OnGetMetaMagicFeats_PushInteger(CVirtualMachine* vm, int metamagicfeats)
{
	if (metamagicfeats > 0)
	{
		CNWSCreature* creature = GetCreatureById(vm->vm_cmd->cmd_self_id);
		if (creature)
		{
			/*uint16_t METAMAGIC_FEATS[] = {11, 12, 25, 29, 33, 37};
			for (uint8_t i=0; i<6; i++)
			{
				if (metamagicfeats & 1<<i && !has_feat(creature->cre_stats, METAMAGIC_FEATS[i]))
				{
					if (get_player_by_game_object_id(creature->obj.obj_id))
					{
						fprintf(stderr, "%x does not have the feat for metamagic!\n", creature->obj.obj_id);
						metamagicfeats &= ~(1<<i);
					}
				}
			}*/

			if (creature->cre_is_poly)
			{
				metamagicfeats = 0;
			}
		}
	}
	return VMPushInteger(vm, metamagicfeats);
}

void init()
{
	//spell gain/known tables support for caster levels 41+
	hook_function(0x080bf30c, (unsigned long)GetSpellGain_Hook, d_ret_code_getspellgain, 12);
	hook_function(0x080bf344, (unsigned long)GetSpellsKnownPerLevel_Hook, d_ret_code_getspellknown, 12);
    
    //infinite cast spell bug
	enable_write(0x080fd2e3);
	*(uint32_t*)(0x080fd2e3) = ((uint32_t)CreatureCastSpell_Impact-(uint32_t)0x080fd2e7);
	enable_write(0x081d2b0c);
    
    //push current spell metamagic feats
	enable_write(0x082327e1);
	*(uint32_t*)(0x082327e1) = ((uint32_t)OnGetMetaMagicFeats_PushInteger-(uint32_t)0x082327e5);
}
REGISTER_INIT(init);
 
}
}