#include "nwscript_funcs.h"

namespace {

    bool (*IsClassBonusFeat)(CNWClass*, uint16_t) = (bool (*)(CNWClass*, uint16_t))0x080bf524;
    VM_FUNC_NEW(GetIsClassBonusFeat, 439)
    {
        uint32_t class_index = vm_pop_int();
        uint16_t feat = vm_pop_int();
        bool result = false;
        if (class_index < nwn_rules->ru_classes_len)
        {
            result = IsClassBonusFeat(&nwn_rules->ru_classes[class_index], feat);
        }
        vm_push_int(result);
    }
    bool (*IsClassNormalFeat)(CNWClass*, uint16_t) = (bool (*)(CNWClass*, uint16_t))0x080bf570;
    VM_FUNC_NEW(GetIsClassGeneralFeat, 440)
    {
        uint32_t class_index = vm_pop_int();
        uint16_t feat = vm_pop_int();
        bool result = false;
        if (class_index < nwn_rules->ru_classes_len)
        {
            result = IsClassNormalFeat(&nwn_rules->ru_classes[class_index], feat);
        }
        vm_push_int(result);
    }
    bool (*GetIsClassGrantedFeat)(CNWClass*, uint16_t, uint8_t*) = (bool (*)(CNWClass*, uint16_t, uint8_t*))0x080bf5e0;
    VM_FUNC_NEW(GetIsClassGrantedFeat, 441)
    {
        uint32_t class_index = vm_pop_int();
        uint16_t feat = vm_pop_int();
        int result = 0;
        if (class_index < nwn_rules->ru_classes_len)
        {
            uint8_t at_level = 0;
            result = GetIsClassGrantedFeat(&nwn_rules->ru_classes[class_index], feat, &at_level);
            if (result)
            {
                result = at_level;
            }
        }
        vm_push_int(result);
    }
    bool (*GetIsSkillUseable)(CNWClass*, uint16_t) = (bool (*)(CNWClass*, uint16_t))0x080bf440;
    bool (*GetClassSkill)(CNWClass*, uint16_t) = (bool (*)(CNWClass*, uint16_t))0x080bf3e4;
    VM_FUNC_NEW(GetIsClassSkill, 442)
    {
        uint32_t class_index = vm_pop_int();
        uint16_t skill = vm_pop_int();
        int result = 0;
        if (skill < nwn_rules->ru_skills_len)
        {
            CNWClass* cl = &nwn_rules->ru_classes[class_index];
            result = GetIsSkillUseable(cl, skill) && GetClassSkill(cl, skill);
        }
        vm_push_int(result);
    }

}
