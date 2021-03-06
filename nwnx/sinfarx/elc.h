#pragma once

#include <cstdio>
#include <cstdint>
#include <string>

#define ELCERROR_INVALID_PC_FILE		1
#define ELCERROR_INVALID_2DA_FILE		2
#define ELCERROR_INVALID_INITIAL_ABILITIES	3
#define ELCERROR_INVALID_RACE			4
#define ELCERROR_INVALID_CLASS			5
#define ELCERROR_INVALID_FEAT			6
#define ELCERROR_NO_CLASS_PREREQ		7
#define ELCERROR_INVALID_SKILL_RANK		8
#define ELCERROR_INVALID_SKILL			9
#define ELCERROR_NO_FEAT_PREREQ			10
#define ELCERROR_INVALID_HITDIE			11
#define ELCERROR_TOO_MANY_FEAT			12
#define ELCERROR_INVALID_FINAL_CLASSES		13
#define ELCERROR_INVALID_FINAL_SKILLS		14
#define ELCERROR_INVALID_FINAL_FEATS		15
#define ELCERROR_INVALID_CLASS_LEVEL		16
#define ELCERROR_SAVE_BONUS			17

#define ABILITY_STRENGTH	0
#define ABILITY_DEXTERITY	1
#define ABILITY_CONSTITUTION	2
#define ABILITY_INTELLIGENCE	3
#define ABILITY_WISDOM		4
#define ABILITY_CHARISMA	5

#define RACE_HUMAN 6

#define CLASS_TYPE_BARD			1
#define CLASS_TYPE_CLERIC		2
#define CLASS_TYPE_DRUID		3
#define CLASS_TYPE_PALADIN		6
#define CLASS_TYPE_RANGER		7
#define CLASS_TYPE_SORCERER		9
#define CLASS_TYPE_WIZARD		10
#define CLASS_TYPE_DRAGON_DISCIPLE	37
#define CLASS_TYPE_PALE_MASTER 34

#define FEAT_WAR_DOMAIN_POWER		306
#define FEAT_WATER_DOMAIN_POWER		325
#define FEAT_EPIC_GREAT_CHARISMA_1	764
#define FEAT_EPIC_GREAT_CHARISMA_10	773
#define FEAT_EPIC_GREAT_CONSTITUTION_1	774
#define FEAT_EPIC_GREAT_CONSTITUTION_10 783
#define FEAT_EPIC_GREAT_DEXTERITY_1	784
#define FEAT_EPIC_GREAT_DEXTERITY_10	793
#define FEAT_EPIC_GREAT_INTELLIGENCE_1  794
#define FEAT_EPIC_GREAT_INTELLIGENCE_10 803
#define FEAT_EPIC_GREAT_WISDOM_1	804
#define FEAT_EPIC_GREAT_WISDOM_10	813
#define FEAT_EPIC_GREAT_STRENGTH_1	814
#define FEAT_EPIC_GREAT_STRENGTH_10	823
#define FEAT_DRAGON_ABILITIES		962
#define FEAT_DRAGON_HDINCREASE_D6	1042
#define FEAT_DRAGON_HDINCREASE_D8       1043
#define FEAT_DRAGON_HDINCREASE_D10	1044
#define FEAT_EPIC_CHARACTER		1001
