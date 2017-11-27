#include "effect.h"
#include "script_event.h"

#include <boost/lexical_cast.hpp>

using namespace nwnx::core;
using namespace nwnx::script_event;

namespace nwnx { namespace effect {

void* OnAllocCGameEffect_Extend(uint32_t size)
{
	if (size != 0x90)
	{
		fprintf(stderr, "OnAllocCGameEffect_Extend unexpected size:%d\n", size);
		print_backtrace();
		return malloc(size);
	}
	else
	{
		CGameEffect* result = (CGameEffect*)malloc(sizeof(CGameEffect));
		result->extra = NULL;
		return result;
	}
}

unsigned char d_ret_code_destroygameeffect[0x20];
void (*DestroyGameEffect_Org)(CGameEffect*, int) = (void (*)(CGameEffect*, int))&d_ret_code_destroygameeffect;
void DestroyGameEffect_Hook(CGameEffect* effect, int p2)
{
	if (effect->extra)
	{
		delete (CGameEffect_EXTRA*)effect->extra;
	}
	return DestroyGameEffect_Org(effect, p2);
}
unsigned char d_ret_code_copygameeffect[0x20];
void (*CopyGameEffect_Org)(CGameEffect*, CGameEffect*, int) = (void (*)(CGameEffect*, CGameEffect*, int))&d_ret_code_copygameeffect;
void CopyGameEffect_Hook(CGameEffect* to, CGameEffect* from, int p3)
{
	CopyGameEffect_Org(to, from, p3);
	if (from && from->extra)
	{
		to->extra = new CGameEffect_EXTRA(*from->extra);
	}
}

int OnGetInvis_GetAIReaction(CNWSObject* source, uint32_t target_id)
{
	if (source->obj_type == OBJECT_TYPE_CREATURE)
	{
		CNWSCreature* target = GetCreatureById(target_id);
		if (target)
		{
			if (((CNWSCreature*)source)->cre_stats->cs_faction_id == target->cre_stats->cs_faction_id)
			{
				return 0;
			}
		}
	}
	return 2;
}
void* (*GetVisibleListElement)(CNWSCreature*, uint32_t) = (void* (*)(CNWSCreature*, uint32_t))0x081131fc;
void* OnApplySanctuary_GetVisibleListElement(CNWSCreature* source, uint32_t target_id)
{
	void* visible_element = GetVisibleListElement(source, target_id);
	if (visible_element)
	{
		CNWSCreature* target = GetCreatureById(target_id);
		if (target)
		{
			if (source->cre_stats->cs_faction_id == target->cre_stats->cs_faction_id)
			{
				visible_element = NULL;
			}
		}
	}
	return visible_element;
}
int OnGetInvis_DoSanctuarySave(CNWSCreature*, uint8_t, uint16_t, uint8_t, uint32_t, int, uint16_t, int)
{
	return 1;
}

CGameEffect** OnRemoveEffect_GetEffect(CExoArrayList<void*>* list, uint32_t index)
{
	CGameEffect** result = (CGameEffect**)list->data+index;
	(*result)->eff_creator = 0xFFFFFFFF;
	return result;
}
void OnCopyEffects_AddEffect(CExoArrayList<void*>* list, CGameEffect* effect)
{
	if (effect->eff_creator == 0xFFFFFFFF)
	{
		delete effect;
	}
	else
	{
		list->add(effect);
	}
}

unsigned char d_ret_code_applyeffect[0x20];
int (*OnApplyEffect_Org)(CNWSEffectListHandler*, CNWSObject*, CGameEffect*, int) = (int (*)(CNWSEffectListHandler*, CNWSObject*, CGameEffect*, int))&d_ret_code_applyeffect;
int OnApplyEffect_HookProc(CNWSEffectListHandler* handler, CNWSObject* object, CGameEffect* effect, int p4)
{
	std::string event_script = "_applyeffect_"+boost::lexical_cast<std::string>(effect->eff_type);
	if (nwscript::has_registered_events(event_script))
	{
		script_event::RESULT result = script_event::run(event_script, object->obj_id, {effect});
		if (result)
		{
			return (result->as_int()==0?0:1);
		}
	}
	return OnApplyEffect_Org(handler, object, effect, p4);
}

unsigned char d_ret_code_removeeffect[0x20];
int (*OnRemoveEffect_Org)(CNWSEffectListHandler*, CNWSObject*, CGameEffect*) = (int (*)(CNWSEffectListHandler*, CNWSObject*, CGameEffect*))&d_ret_code_removeeffect;
int OnRemoveEffect_HookProc(CNWSEffectListHandler* handler, CNWSObject* object, CGameEffect* effect)
{
	if (script_event::run("_removeffect_"+boost::lexical_cast<std::string>(effect->eff_type), object->obj_id))
	{
		return 1;
	}
	else
	{
		return OnRemoveEffect_Org(handler, object, effect);
	}
}


void init()
{
	hook_function(0x0817DFDC, (unsigned long)DestroyGameEffect_Hook, d_ret_code_destroygameeffect, 12);
	hook_function(0x0817E08C, (unsigned long)CopyGameEffect_Hook, d_ret_code_copygameeffect, 12);
    
    //extend CGameEffect
	long CGameEffects_New_Calls[] = {
		0x080546C8, 0x08055F23, 0x08056D01, 0x0805DAE8, 0x080609F0, 0x08097AEC, 0x0809AB1C, 0x080CD3E2,
		0x080E7316, 0x080E7463, 0x080E74DB, 0x080E96DC, 0x080E9CA0, 0x080E9DFE, 0x080E9E74, 0x080E9F8B,
		0x080EA0D6, 0x080EA282, 0x080EA52B, 0x080EAE5F, 0x080ED121, 0x080ED20D, 0x080ED34F, 0x080ED3BF,
		0x080ED928, 0x080EDA5D, 0x080EE5DB, 0x080EFE2C, 0x080EFECB, 0x080F19EE, 0x080F1A6F, 0x080F1AB7,
		0x080F1B12, 0x080F1C7A, 0x080F1CBF, 0x080F1D1F, 0x080F1D92, 0x080F1DEE, 0x080F1F72, 0x080F1FB7,
		0x080F2017, 0x080F208A, 0x080F20E6, 0x080F226A, 0x080F22EB, 0x080F2326, 0x080F2382, 0x080F24EA,
		0x080F252F, 0x080F258F, 0x080F2602, 0x080F265E, 0x080F27E2, 0x080F2863, 0x080F28CF, 0x080F2948,
		0x080F29BA, 0x080F2A33, 0x080F2AA7, 0x080F2B1C, 0x080F2B89, 0x080F2BE6, 0x080F2E0A, 0x080F2E4F,
		0x080F2EAF, 0x080F2F1A, 0x080F2F76, 0x080F30F2, 0x080F3137, 0x080F3197, 0x080F320A, 0x080F3263,
		0x080F32D1, 0x080F332E, 0x080F34F2, 0x080F3544, 0x080F35A3, 0x080F3616, 0x080F366F, 0x080F36DD,
		0x080F373A, 0x080F38FE, 0x080F397F, 0x080F39BA, 0x080F3A16, 0x080F3B7E, 0x080F3BFF, 0x080F3C51,
		0x080F3CAB, 0x080F3D1E, 0x080F3D7A, 0x080F3F1E, 0x080F3F9F, 0x080F3FDA, 0x080F4036, 0x080F419E,
		0x080F41E3, 0x080F4243, 0x080F42B6, 0x080F4312, 0x080F4C60, 0x080F4CD8, 0x080F4DE2, 0x080F4E9C,
		0x080F4F5C, 0x080F4FD4, 0x080F50E1, 0x080F5293, 0x080F530D, 0x080F54FE, 0x080F5578, 0x080F5691,
		0x080F570B, 0x080F57F0, 0x080F58D8, 0x080F59A2, 0x080FC668, 0x08108F95, 0x0810A00D, 0x0810BBC3,
		0x0810BC31, 0x0810CFCF, 0x0810DBFB, 0x0810E04E, 0x08115462, 0x0811D5FB, 0x0811E2FE, 0x08124733,
		0x08124808, 0x08135C63, 0x08135C84, 0x08135D67, 0x0813638E, 0x081363B0, 0x08136E34, 0x08138AD5,
		0x08138BB8, 0x08138FF0, 0x0813909C, 0x081393AD, 0x0813942C, 0x0813B6C2, 0x0813B80C, 0x08169C7B,
		0x0816C6EE, 0x0816C782, 0x0816CCCE, 0x0816D3F0, 0x0816D515, 0x0816D56D, 0x0816D5C1, 0x0816D611,
		0x0816D952, 0x0816DE46, 0x0816EC24, 0x0816EFF1, 0x0816F427, 0x0816F74E, 0x0816FB01, 0x0816FE82,
		0x0816FFC8, 0x081700FE, 0x08170601, 0x081709E7, 0x08170AFB, 0x08170BB5, 0x08170CA7, 0x08170D7C,
		0x08170F5A, 0x0817101A, 0x0817106D, 0x081710BD, 0x08171109, 0x0817133A, 0x08171619, 0x0817194A,
		0x08171966, 0x08171982, 0x08171A2F, 0x08171D16, 0x08172604, 0x08172620, 0x0817267F, 0x08172817,
		0x081733AA, 0x08173470, 0x081739A6, 0x08173B03, 0x08173B40, 0x08173BB7, 0x08173CB7, 0x08173CFB,
		0x08173D3B, 0x08173D89, 0x08173DC3, 0x08173E14, 0x08173E6E, 0x08173EB4, 0x08173F0E, 0x08173F54,
		0x08173F8E, 0x08173FF0, 0x08174051, 0x081740A4, 0x081740DE, 0x0817411B, 0x08174158, 0x08174180,
		0x081746FA, 0x0817493C, 0x08175374, 0x081754F4, 0x08175794, 0x08175979, 0x08175F0E, 0x08175FAC,
		0x081762EA, 0x0817690F, 0x0817694B, 0x08176CFC, 0x08176FA8, 0x08177060, 0x0817709F, 0x08177121,
		0x081771ED, 0x08177229, 0x08177297, 0x081772EC, 0x08177343, 0x08177396, 0x08177508, 0x08177543,
		0x0817788C, 0x08177C2F, 0x08177ECC, 0x08177F22, 0x08177F78, 0x08177FF0, 0x08178046, 0x0817809C,
		0x081780F2, 0x08178362, 0x0817839E, 0x081784B9, 0x08178DEE, 0x08178E61, 0x08178EA8, 0x08178FBF,
		0x08179126, 0x0817922A, 0x0817930F, 0x081794B9, 0x08179511, 0x08179565, 0x081795B5, 0x08179624,
		0x081796AC, 0x08179DBF, 0x08179F20, 0x0817A2B3, 0x0817A32E, 0x0817A409, 0x0817A55A, 0x0817A5C7,
		0x0817A82B, 0x0817A92D, 0x0817A9AD, 0x0817AC13, 0x0817AE4E, 0x0817AE98, 0x0817AED4, 0x0817AF29,
		0x0817B170, 0x0817B232, 0x0817B29F, 0x0817B2E1, 0x0817B31D, 0x0817B8D7, 0x0817BA68, 0x0817BAD1,
		0x0817BB33, 0x0817BB98, 0x0817BCB1, 0x0817BD60, 0x0817BF37, 0x0817C024, 0x0817C8BD, 0x0817C931,
		0x0817C96D, 0x0817C9B6, 0x0817C9FF, 0x0817CA68, 0x0817D1A1, 0x0817D3FC, 0x0817D695, 0x0817D7AD,
		0x0817D924, 0x0817DB50, 0x0817E117, 0x0817E153, 0x0817E302, 0x0817E33E, 0x081858C1, 0x08185906,
		0x08185BF0, 0x081861AD, 0x0818700F, 0x081871C4, 0x081874AF, 0x081875E0, 0x08187779, 0x0818780A,
		0x0818789B, 0x08188E87, 0x08188EDE, 0x08189333, 0x0818F368, 0x081A0F02, 0x081A5245, 0x081A8087,
		0x081A833F, 0x081A8388, 0x081A86D9, 0x081A8F63, 0x081A8F7C, 0x081A90DD, 0x081A9239, 0x081A9739,
		0x081A99D1, 0x081A9E62, 0x081AA11B, 0x081AA5D6, 0x081AA735, 0x081AAADE, 0x081AACF7, 0x081AAFFA,
		0x081AB66B, 0x081ABAC4, 0x081ABBCC, 0x081AC5F5, 0x081AC671, 0x081AC83D, 0x081AC8C0, 0x081AC927,
		0x081AC98E, 0x081ACCD3, 0x081ACF7E, 0x081AD21F, 0x081AD395, 0x081AD6B6, 0x081AD90E, 0x081ADBA1,
		0x081ADE84, 0x081AE1E0, 0x081AE484, 0x081AE8C3, 0x081AE9BC, 0x081AEA10, 0x081AEB24, 0x081AEB78,
		0x081AEC87, 0x081AECDB, 0x081AED44, 0x081AEE3C, 0x081AEE90, 0x081AEFA4, 0x081AF4CA, 0x081AF54E,
		0x081AF78E, 0x081AF970, 0x081AFB55, 0x081AFD44, 0x081AFEDD, 0x081AFF96, 0x081B0338, 0x081B06D8,
		0x081B07D0, 0x081B08EC, 0x081B0A8C, 0x081CA103, 0x081CF1D7, 0x081CFAFD, 0x081DEFE3, 0x081FB843,
		0x081FB8D5, 0x081FB9DE, 0x081FBA9D, 0x081FBC40, 0x081FBCB5, 0x08205360, 0x0820551E, 0x082056B7,
		0x082057FF, 0x082058F4, 0x082059CD, 0x08205F74, 0x08205FF3, 0x082071FB, 0x08207442, 0x082075D2,
		0x08207854, 0x082079D8, 0x08207B3F, 0x08207C97, 0x08207E27, 0x08207FA9, 0x08208131, 0x08209310,
		0x0820A878, 0x0820A994, 0x0820B388, 0x0820BD94, 0x0820D328, 0x0820D730, 0x0820D808, 0x0820EE43,
		0x0820FFD0, 0x082100A8, 0x0821149C, 0x08211F00, 0x082123B4, 0x082124E4, 0x0821325E, 0x08213A14,
		0x08216094, 0x08216208, 0x08218564, 0x0821A498, 0x0821A5B4, 0x0821AC23, 0x0821AD10, 0x0821ADB4,
		0x0821AEC7, 0x0821AFA0, 0x0821B044, 0x0821B140, 0x0821B26C, 0x0821B344, 0x0821B40C, 0x0821B4FC,
		0x0821B630, 0x0821B760, 0x0821B8F7, 0x0821C9BC, 0x0821CE64, 0x0821E918, 0x0821FBDC, 0x082274C4,
		0x08227543, 0x082276CD, 0x08228B2A, 0x08229D78, 0x0822D4D8, 0x082365A4};
	for (uint32_t i=0; i<sizeof(CGameEffects_New_Calls)/sizeof(long); i++)
	{
		hook_call(CGameEffects_New_Calls[i], (long)OnAllocCGameEffect_Extend);
	}
    
    //dont save removed effects
	hook_call(0x08206187, (long)OnRemoveEffect_GetEffect);
	hook_call(0x08060A26, (long)OnCopyEffects_AddEffect);
    
	//----------------- Sanctuarry Effect Fix ---------------------------
	//do not end after the first invis/gs
	enable_write(0x08137109);
	*(int*)0x08137109 = 1112-377;
	enable_write(0x08137114);
	*(int*)0x08137114 = 1112-388;
	enable_write(0x081371c2);
	*(int*)0x081371c2 = 1112-562;
	enable_write(0x081371cd);
	*(int*)0x081371cd = 1112-573;
	enable_write(0x081372c7);
	*(int*)0x081372c7 = 1112-823;
	//loop all effects (0 to len)
	enable_write(0x0813700c);
	*(short*)0x0813700c = 0xf631;
	enable_write(0x0813700e);
	*(int*)0x0813700e = 0x90909090;
	enable_write(0x08137012);
	*(char*)0x08137012 = 0x90;
	//if gs dont work for weird reasons
	enable_write(0x08137417);
	*(short*)0x08137417 = 0xe990;
	//true seeing dont see gs
	enable_write(0x081374bf);
	*(char*)0x081374bf = 0x40;
	*(short*)0x081374c0 = 0x9090;
	//party member can see
	enable_write(0x081371f8);
	*(uint32_t*)0x081371f8 = ((uint32_t)OnGetInvis_GetAIReaction-(uint32_t)0x081371fc);
	enable_write(0x081777e7);
	*(uint32_t*)0x081777e7 = ((uint32_t)OnApplySanctuary_GetVisibleListElement-(uint32_t)0x081777eb);
	//the only save that matter is the first one
	enable_write(0x081372fb);
	*(uint32_t*)0x081372fb = ((uint32_t)OnGetInvis_DoSanctuarySave-(uint32_t)0x081372ff);
	enable_write(0x08137381);
	*(uint32_t*)0x08137381 = ((uint32_t)OnGetInvis_DoSanctuarySave-(uint32_t)0x08137385);
	//----------------- End Sanctuarry Effect Fix ---------------------------
    
    hook_function(0x0817cc30, (unsigned long)OnApplyEffect_HookProc, d_ret_code_applyeffect, 12);
	hook_function(0x0817cc9c, (unsigned long)OnRemoveEffect_HookProc, d_ret_code_removeeffect, 12);
}
REGISTER_INIT(init);
 
}
}