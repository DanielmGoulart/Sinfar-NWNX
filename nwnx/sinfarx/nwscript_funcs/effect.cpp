#include "nwscript_funcs.h"

namespace
{

void (*RemoveEffect)(CNWSObject*, CGameEffect*) = (void (*)(CNWSObject*, CGameEffect*))0x081cedb8;
VM_FUNC_NEW(InstantlyRemoveEffects, 240)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	int duration_type = vm_pop_int();
	int effect_type = vm_pop_int();
	int spell_id = vm_pop_int();
	int int1 = vm_pop_int();
	int int1value = vm_pop_int();
	int int2 = vm_pop_int();
	int int2value = vm_pop_int();
	uint32_t creator = vm_pop_object();
	std::string tag = vm_pop_string();
	if (object)
	{
		for (uint32_t nEffect=0; nEffect<object->obj_effects_len; nEffect++)
		{
			CGameEffect* effect = *(object->obj_effects+nEffect);
			if ((duration_type == -1 || (effect->eff_dursubtype&7) == duration_type) &&
				(effect_type == -1 || effect->eff_type == effect_type) &&
				(creator == OBJECT_INVALID || effect->eff_creator == creator) &&
				(spell_id == -1 || (int)effect->eff_spellid == spell_id) &&
				((int1 == -1 || (int1 >= 0 && int1 < (int)effect->eff_num_integers && int1value == *(effect->eff_integers+int1)))) &&
				((int2 == -1 || (int2 >= 0 && int2 < (int)effect->eff_num_integers && int2value == *(effect->eff_integers+int2)))) &&
				(tag == "" || (effect->extra && effect->extra->tag == tag)))
			{
				RemoveEffect(object, effect);
				nEffect--;
			}
		}
	}
}

CGameEffect* pop_original_effect()
{
	CGameEffect* real_effect = last_copied_effect;
	vm_pop_effect();
    return real_effect;
}
VM_FUNC_NEW(GetEffectTag, 342)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
	CExoString result;
	if (effect->extra)
	{
		result = CExoString(effect->extra->tag);
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(SetEffectTag, 343)
{
	CGameEffect* effect = pop_original_effect();
	std::string tag = vm_pop_string();
	if (!effect->extra && !tag.empty())
	{
		effect->extra = new CGameEffect_EXTRA;
	}
	effect->extra->tag = tag;
}
VM_FUNC_NEW(GetTrueEffectCount, 296)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	vm_push_int(object ? object->obj_effects_len : 0);
}
VM_FUNC_NEW(GetTrueEffectByIndex, 297)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	uint32_t index = vm_pop_int();
	if (object && index < object->obj_effects_len)
	{
		vm_push_effect(object->obj_effects[index]);
	}
	else
	{
		vm_push_effect(NULL);
	}
}
VM_FUNC_NEW(RemoveTrueEffectByIndex, 298)
{
	CNWSObject* object = GetObjectById(vm_pop_object());
	uint32_t index = vm_pop_int();
	if (object && index < object->obj_effects_len)
	{
		RemoveEffect(object, object->obj_effects[index]);
	}
}
VM_FUNC_NEW(GetEffectDuration, 300, 299)
{
    std::unique_ptr<CGameEffect> effect(vm_pop_effect());
    vm_push_float(effect->eff_duration);
}
VM_FUNC_NEW(GetEffectDurationRemaining, 302, 301)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
    int64_t expire = (effect->eff_expire_day * 2880000LL) + effect->eff_expire_time;
    vm_push_float((float)(expire - get_world_time(NULL, NULL)) / 1000.0);
}
VM_FUNC_NEW(GetEffectId, 303)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
	vm_push_int(effect->eff_id);
}
VM_FUNC_NEW(SetEffectId, 304)
{
    CGameEffect* effect = pop_original_effect();
	effect->eff_id = vm_pop_int(); //TODO: also set the 64 bit part of the ID, using the current one
}
VM_FUNC_NEW(GetEffectTrueType, 305)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
	vm_push_int(effect->eff_type);
}
VM_FUNC_NEW(SetEffectTrueType, 306)
{
    CGameEffect* effect = pop_original_effect();
	effect->eff_type = vm_pop_int();
}
int (*GetEffectInteger)(CGameEffect*, int) = (int (*)(CGameEffect*, int))0x0817f5f8;
VM_FUNC_NEW(GetEffectInteger, 308, 307)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
	vm_push_int(GetEffectInteger(effect.get(), vm_pop_int()));
}
VM_FUNC_NEW(SetEffectInteger, 310, 309)
{
    CGameEffect* effect = pop_original_effect();
    uint32_t index = vm_pop_int();
    int val = vm_pop_int();
	if (effect->eff_integers && index < effect->eff_num_integers)
	{
		effect->eff_integers[index] = val;
	}
}
VM_FUNC_NEW(GetEffectSpellId, 311)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
    vm_push_int(effect->eff_spellid);
}
VM_FUNC_NEW(SetEffectSpellId, 313, 312)
{
    CGameEffect* effect = pop_original_effect();
    effect->eff_spellid  = vm_pop_int();
}
VM_FUNC_NEW(GetEffectCreator, 314)
{
	std::unique_ptr<CGameEffect> effect(vm_pop_effect());
    vm_push_object(effect->eff_creator);
}
void (*SetEffectCreator)(CGameEffect*, uint32_t) = (void (*)(CGameEffect*, uint32_t))0x0817eecc;
VM_FUNC_NEW(SetEffectCreator, 316, 315)
{
    CGameEffect* effect = pop_original_effect();
    SetEffectCreator(effect, vm_pop_object());
}
void setup_new_effect(CGameEffect& effect)
{
	effect.eff_dursubtype = 8;
	SetEffectCreator(&effect, virtual_machine->vm_cmd->cmd_self_id);
}
void (*SetEffectInteger)(CGameEffect*, int, int) = (void (*)(CGameEffect*, int, int))0x817f60c;
VM_FUNC_NEW(EffectBonusFeat, 317)
{
	int feat = vm_pop_int();
	CGameEffect effect(true);
    setup_new_effect(effect);
    effect.eff_type = 83;
	SetEffectInteger(&effect, 0, feat);
	vm_push_effect(&effect);
}
VM_FUNC_NEW(EffectIcon, 318)
{
	int icon = vm_pop_int();
	CGameEffect effect(true);
	setup_new_effect(effect);
    effect.eff_type = 67;
	SetEffectInteger(&effect, 0, icon);
	vm_push_effect(&effect);
}
VM_FUNC_NEW(EffectSetAIState, 319)
{
	int ai_state = vm_pop_int();
	CGameEffect effect(true);
    setup_new_effect(effect);
    effect.eff_type = 23;
	SetEffectInteger(&effect, 0, ai_state);
	vm_push_effect(&effect);
}
	
}