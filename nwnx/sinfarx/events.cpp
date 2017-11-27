#include "script_event.h"

namespace nwnx { namespace events {

int (*HandlePlayerToServerParty_Org)(CNWSMessage*, CNWSPlayer*, uint8_t);
int HandlePlayerToServerParty_Hook(CNWSMessage* message, CNWSPlayer* player, uint8_t type)
{
    if (type != 2)
    {
        if (script_event::run("pc_ev_party", player->pl_pc_oid, {type, message->get_object()}))
        {
            return true;
        }
    }
    return HandlePlayerToServerParty_Org(message, player, type);
}

int (*HandlePlayerToServerPVPListOperations_Org)(CNWSMessage*, CNWSPlayer*, uint8_t);
int HandlePlayerToServerPVPListOperations_Hook(CNWSMessage* message, CNWSPlayer* player, uint8_t type)
{
    if (script_event::run("pc_ev_setlike", player->pl_pc_oid, {type, message->get_object()}))
    {
        return true;
    }
    return HandlePlayerToServerPVPListOperations_Org(message, player, type);
}

int (*GetCanFireTrapOnObject)(CNWSTrigger*, uint32_t, int) = (int (*)(CNWSTrigger*, uint32_t, int))0x081f24d4;
int GetCanFireTrapOnObject_Hook(CNWSTrigger* trigger, uint32_t target_id, int type)
{
    int nReturn = GetCanFireTrapOnObject(trigger, target_id, type);
    if (nReturn)
    {
        if (script_event::run("pc_ev_firetrap", trigger->obj.obj_id, {type, target_id}))
        {
            return false;
        }
    }
    return nReturn;
}

int (*CanEquip)(CNWSCreature*, CNWSItem*, uint32_t*, int, int, int, CNWSPlayer*) = (int (*)(CNWSCreature*, CNWSItem*, uint32_t*, int, int, int, CNWSPlayer*))0x80ff978;
int OnEquipCanEquip_Hook(CNWSCreature* creature, CNWSItem* item, uint32_t* p3, int p4, int p5, int p6, CNWSPlayer* p7)
{
    if (creature->obj.obj_id > OBJECT_INVALID) //player char
    {
        if (script_event::run("ilr_canequip", creature->obj.obj_id, {item->obj.obj_id}))
        {
            return false;
        }
    }
    return CanEquip(creature, item, p3, p4, p5, p6, p7);
}

int (*AddTrapAction_Org)(CNWSCreature*, uint32_t, char, uint32_t, Vector, uint32_t);
int AddTrapAction_Hook(CNWSCreature* creature, uint32_t target_id, char type, uint32_t p4, Vector position, uint32_t p6)
{
    if (script_event::run("pc_ev_trap", creature->obj.obj_id, {type, target_id, position}))
    {
        return true;
    }
    return AddTrapAction_Org(creature, target_id, type, p4, position, p6);
}

int (*PickPocket_Org)(CNWSCreature*, CNWSAction*);
int PickPocket_Hook(CNWSCreature* creature, CNWSAction* action)
{
    if (script_event::run("pc_ev_pickpocket", creature->obj.obj_id, {action->field_34}))
    {
        return CNWSObject_ACTION_COMPLETE;
    }
    return PickPocket_Org(creature, action);
}
int (*Attack_Org)(CNWSCreature* creature, uint32_t, int, int, int, int);
int Attack_Hook(CNWSCreature* creature, uint32_t target_id, int i1, int i2, int i3, int event_subID)
{
    if (script_event::run("pc_ev_attack", creature->obj.obj_id, {target_id, event_subID}))
    {
        return true;
    }
    return Attack_Org(creature, target_id, i1, i2, i3, event_subID);
}

int (*UseItem_Org)(CNWSCreature*, uint32_t, int, int, Vector, uint32_t, int);
int UseItem_Hook(CNWSCreature* creature, uint32_t item_id, int event_subID, int p4, Vector position, uint32_t target_id, int p7)
{
    if (script_event::run("pc_ev_useitem", creature->obj.obj_id, {item_id, target_id, position, event_subID}))
    {
        return true;
    }
    return UseItem_Org(creature, item_id, event_subID, p4, position, target_id, p7);
}

int (*ExamineItem_Org)(CNWSMessage*, CNWSPlayer*, uint32_t);
int ExamineItem_Hook(CNWSMessage* message, CNWSPlayer* player, uint32_t target_id)
{
    if (script_event::run("pc_ev_examine", player->pl_oid, {target_id}))
    {
        return true;
    }
    return ExamineItem_Org(message, player, target_id);
}

int (*ExamineCreature_Org)(CNWSMessage*, CNWSPlayer*, uint32_t);
int ExamineCreature_Hook(CNWSMessage* message, CNWSPlayer* player, uint32_t target_id)
{
    if (script_event::run("pc_ev_examine", player->pl_oid, {target_id}))
    {
        return true;
    }
    return ExamineCreature_Org(message, player, target_id);
}

int (*ExaminePlaceable_Org)(CNWSMessage*, CNWSPlayer*, uint32_t);
int ExaminePlaceable_Hook(CNWSMessage* message, CNWSPlayer* player, uint32_t target_id)
{
    if (script_event::run("pc_ev_examine", player->pl_oid, {target_id}))
    {
        return true;
    }
    return ExaminePlaceable_Org(message, player, target_id);
}

int (*ExamineDoor_Org)(CNWSMessage*, CNWSPlayer*, uint32_t);
int ExamineDoor_Hook(CNWSMessage* message, CNWSPlayer* player, uint32_t target_id)
{
    if (script_event::run("pc_ev_examine", player->pl_oid, {target_id}))
    {
        return true;
    }
    return ExamineDoor_Org(message, player, target_id);
}

int (*UseSkill_Org)(CNWSCreature*, uint8_t, uint8_t, uint32_t, Vector, uint32_t, uint32_t, int);
int UseSkill_Hook(CNWSCreature* creature, uint8_t nSkill, uint8_t nSubSkill, uint32_t nTargetObjID, Vector vTarget, uint32_t nAreaID, uint32_t nItemObjID, int arg_24)
{
    if (script_event::run("pc_ev_useskill", creature->obj.obj_id, {nSkill, nTargetObjID, vTarget, nItemObjID}))
    {
        return true;
    }
    return UseSkill_Org(creature, nSkill, nSubSkill, nTargetObjID, vTarget, nAreaID, nItemObjID, arg_24);
}

int (*UseFeat_Org)(CNWSCreature*, uint16_t, uint16_t, uint32_t, uint32_t, Vector*);
int UseFeat_Hook(CNWSCreature* creature, uint16_t nFeat, uint16_t nSubFeat, uint32_t nTargetObjID, uint32_t nAreaID, Vector* pvTarget)
{
    Vector* target_pos;
    if (pvTarget)
    {
        target_pos = pvTarget;
    }
    else
    {
        CNWSObject* target = core::GetObjectById(nTargetObjID);
        if (target)
        {
            target_pos = &target->obj_position;
        }
        else
        {
            target_pos = new Vector;
            fprintf(stderr, "UseFeat_Hook: invalid target?!?\n");
        }
    }
    if (script_event::run("pc_ev_usefeat", creature->obj.obj_id, {nFeat, nTargetObjID, *target_pos}))
    {
        return true;
    }
    return UseFeat_Org(creature, nFeat, nSubFeat, nTargetObjID, nAreaID, pvTarget);
}

int (*ToggleMode_Org)(CNWSCreature*, uint8_t);
int ToggleMode_Hook(CNWSCreature* creature, uint8_t nMode)
{
    if (script_event::run("pc_ev_togglemode", creature->obj.obj_id, {nMode}))
    {
        return true;
    }
    return ToggleMode_Org(creature, nMode);
}

int (*CastSpell_Org)(CNWSCreature*, uint32_t, int, int, int, int, Vector, uint32_t, int, int, int, uint8_t, int, int, int, uint8_t);
int CastSpell_Hook(CNWSCreature* creature,
                            uint32_t nSpell,
                            int nClassIndex,
                            int nDomainLevel,
                            int nMetaMagic,
                            int a6,
                            Vector vTarget,
                            uint32_t oTarget,
                            int bTargeted,
                            int a12,
                            int a13,
                            uint8_t nProjectilePathType,
                            int bInstantSpell,
                            int a16,
                            int a17,
                            uint8_t a18)
{
    if (nMetaMagic > 0)
    {
        uint16_t METAMAGIC_FEATS[] = {11, 12, 25, 29, 33, 37};
        for (uint8_t i=0; i<6; i++)
        {
            if (nMetaMagic & 1<<i && !core::has_feat(creature->cre_stats, METAMAGIC_FEATS[i]))
            {
                if (creature->obj.obj_id > OBJECT_INVALID && !bInstantSpell)
                {
                    //fprintf(stderr, "%x does not have the feat for metamagic!\n", creature->obj.obj_id);
                    nMetaMagic &= ~(1<<i);
                }
            }
        }
    }
    
    if (script_event::run("pc_ev_castspell", creature->obj.obj_id, {
        oTarget, vTarget, nSpell, nMetaMagic, nClassIndex, bInstantSpell}))
    {
        return true;
    }
    return CastSpell_Org(creature, nSpell, nClassIndex, nDomainLevel, nMetaMagic, a6, vTarget, oTarget, bTargeted, a12, a13, 
        nProjectilePathType, bInstantSpell, a16, a17, a18);
}

void (*TogglePause_Org)(CServerExoAppInternal* server_internal, uint8_t, int);
void TogglePause_Hook(CServerExoAppInternal* server_internal, uint8_t p2, int state)
{
    if (script_event::run("pc_ev_togglepaus", 0, {state}))
    {
        return;
    }
    return TogglePause_Org(server_internal, p2, state);
}

int (*CreatePlaceable)(CNWSPlaceable* placeable, uint32_t object_id) = (int (*)(CNWSPlaceable*, uint32_t))0x81db8f0;
int OnCreateBodyBagObject(CNWSPlaceable* bodybag, uint32_t object_id)
{
	int ret = CreatePlaceable(bodybag, object_id);
	core::run_script("mod_ev_newbdybag", bodybag->obj.obj_id);
	return ret;
}

void init()
{
    core::hook_func(0x810a3f4, (long)PickPocket_Hook, (void*&)PickPocket_Org, 12);
    core::hook_func(0x81188d4, (long)Attack_Hook, (void*&)Attack_Org, 9);
    core::hook_func(0x81159bc, (long)UseItem_Hook, (void*&)UseItem_Org, 12);
    core::hook_func(0x8073f9c, (long)ExamineItem_Hook, (void*&)ExamineItem_Org, 12);
    core::hook_func(0x8073958, (long)ExamineCreature_Hook, (void*&)ExamineCreature_Org, 12);
    core::hook_func(0x8074668, (long)ExaminePlaceable_Hook, (void*&)ExaminePlaceable_Org, 12);
    core::hook_func(0x8074c50, (long)ExamineDoor_Hook, (void*&)ExamineDoor_Org, 12);
    core::hook_func(0x812b1d8, (long)UseSkill_Hook, (void*&)UseSkill_Org, 12);
    core::hook_func(0x812a004, (long)UseFeat_Hook, (void*&)UseFeat_Org, 12);
    core::hook_func(0x812bcb4, (long)ToggleMode_Hook, (void*&)ToggleMode_Org, 10);
    core::hook_func(0x811610c, (long)CastSpell_Hook, (void*&)CastSpell_Org, 12);
    core::hook_func(0x80adb18, (long)TogglePause_Hook, (void*&)TogglePause_Org, 9);
    core::hook_func(0x081186c8, (long)AddTrapAction_Hook, (void*&)AddTrapAction_Org, 12);
    core::hook_func(0x08197774, (long)HandlePlayerToServerParty_Hook, (void*&)HandlePlayerToServerParty_Org, 12);
    core::hook_func(0x0819ab58, (long)HandlePlayerToServerPVPListOperations_Hook, (void*&)HandlePlayerToServerPVPListOperations_Org, 12);
    core::hook_call(0x081f2765, (long)GetCanFireTrapOnObject_Hook);
    core::hook_call(0x08117148, (long)OnEquipCanEquip_Hook);
    
    //to cleanup body bags
    core::hook_call(0x081d3bdc, (long)OnCreateBodyBagObject);
}
REGISTER_INIT(init);
	
}
}