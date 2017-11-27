#include "elc.h"
#include "cpp_utils.h"
#include "cached2da.h"
#include "GFF/quick_read_gff.h"
#include "nwscript.h"

using namespace nwnx::cpp_utils;
using namespace nwnx::cached2da;
using namespace nwnx::nwscript;

#define DBG(X) __FILE__":%d DBG("#X")\n", __LINE__

using namespace std;

namespace {
	
uint32_t GFF_GetListSize(FILE* hGFF, GFF_HEADER* Header, GFF_FIELD* ListField);
void GFF_ReadFieldValueFromStructInList(FILE* hGFF, GFF_HEADER* Header, GFF_FIELD* ListField, const char* LabelName, uint32_t* Buffer, uint32_t nListSize);

inline std::string format_elc_error_message(const std::string& base_message, const char* file, int line_number)
{
	char formatted_message[1000];
	snprintf(formatted_message, 1000, "[%s:%d] %s", file, line_number, base_message.c_str());
	return std::string(formatted_message);
}

inline char* strtolower(char* str)
{
	for (; *str; str++)
		*str = (char)tolower(*str);
	return str;
}

class elc_error : public exception
{
public:
	elc_error(int code, const std::string& msg):error_message(msg),error_code(code){}
	~elc_error() throw() {}
	const char* what() const noexcept {return error_message.c_str();}
	std::string error_message;
	int error_code;
};

class CFile
{
public:
	CFile(const std::string& filepath, const std::string& mode)
	{
		file = fopen(filepath.c_str(), mode.c_str());
	}
	~CFile()
	{
		if (file)
		{
			fclose(file);
		}
	}
	FILE* file;
};

int EnforceLegalCharacter(char* szCharacterFile)
{
	int nReturn = 0;

	CFile char_file (szCharacterFile, "rb");
	if (char_file.file == NULL) return ELCERROR_INVALID_2DA_FILE;
	C2da* feats2da = get_cached_2da("feat");
	if (feats2da->GetRowCount() == 0) return ELCERROR_INVALID_2DA_FILE;
	C2da* classes2da = get_cached_2da("classes");
	C2da* skills2da = get_cached_2da("skills");
	C2da* racialtypes2da = get_cached_2da("racialtypes");

	GFF_HEADER Header;
	my_fread(&Header, sizeof(GFF_HEADER), 1, char_file.file);

	uint32_t nNbFeats = feats2da->GetRowCount();
	uint32_t nNbClasses = classes2da->GetRowCount();
	uint32_t nNbSkills = skills2da->GetRowCount();
	uint32_t nNbRaces = racialtypes2da->GetRowCount();

	GFF_STRUCT Struct;
	uint32_t nStructIndex;
	GFF_FIELD Field;
	uint32_t nField;
	uint32_t nFieldIndex;
	GFF_LABEL Label;

	uint32_t nPCRace = nNbRaces;
	uint32_t Abilities[6] = {0, 0, 0, 0, 0, 0};
	uint32_t* PCSkills = NULL;
	uint32_t nPCNbFeats = 0;
	uint32_t* PCFeats = NULL;
	uint32_t nPCNbClasses = 0;
	uint32_t PCClasses[3];
	uint32_t PCClassesLevel[3] = {0, 0, 0};
	GFF_FIELD FieldLvlStatList;

	C2da* ClassesABTable[3] = {NULL, NULL, NULL};
	C2da* ClassesSpellGainTable[3] = {NULL, NULL, NULL};
	C2da* ClassesFeatsTable[3] = {NULL, NULL, NULL};
	C2da* ClassesBonusFeatsTable[3] = {NULL, NULL, NULL};
	C2da* ClassesPreReqTables[3] = {NULL, NULL, NULL};
	C2da* ClassesSkillsTable[3] = {NULL, NULL, NULL};
	uint32_t ClassesBaseSkillPoints[3] = {0, 0, 0};
	uint32_t ClassesHitDie[3] = {0, 0, 0};
	uint32_t ClassesEpicLevel[3] = {100, 100, 100};
	C2da* RaceFeatsTable = NULL;

	uint32_t* PreviousLevelFeats = NULL;
	uint32_t nPreviousLevelNbFeats = 0;
	uint32_t* ThisLevelFeats = NULL;
	uint32_t nPreviousLevelSkillPoints = 0;
	uint32_t* PreviousLevelSkills = new uint32_t[nNbSkills];
	memset(PreviousLevelSkills, 0, 4*nNbSkills);
	uint32_t* ThisLevelSkills = NULL;
	uint32_t PreviousLevelClassesLevel[3] = {0, 0, 0};

	int nRaceStrAdjust;
	int nRaceDexAdjust;
	int nRaceIntAdjust;
	int nRaceConAdjust;
	int nRaceWisAdjust;
	int nRaceChaAdjust;

	uint32_t nNbLevel;

	int nNbAbilityPoints;

try {
	my_fread(&Struct, sizeof(GFF_STRUCT), 1, char_file.file);
	for (nField=0; nField<Struct.FieldCount; nField++)
	{
		fseek(char_file.file, Header.FieldIndicesOffset+Struct.DataOrDataOffset+(nField*4), SEEK_SET);
		my_fread(&nFieldIndex, 4, 1, char_file.file);
		fseek(char_file.file, Header.FieldOffset+(nFieldIndex*sizeof(GFF_FIELD)), SEEK_SET);
		my_fread(&Field, sizeof(GFF_FIELD), 1, char_file.file);
		fseek(char_file.file, Header.LabelOffset+(Field.LabelIndex*sizeof(GFF_LABEL)), SEEK_SET);
		my_fread(&Label, sizeof(GFF_LABEL), 1, char_file.file);

		if (nPCRace == nNbRaces && strcmp(Label.Name, "Race") == 0)
		{
			nPCRace = Field.DataOrDataOffset;
		}/*
		else if (strcmp(Label.Name, "willbonus") == 0 ||
			strcmp(Label.Name, "fortbonus") == 0 ||
			strcmp(Label.Name, "refbonus") == 0 ||
			strcmp(Label.Name, "movementrate") == 0)
		{
			if (Field.DataOrDataOffset != 0){throw elc_error(ELCERROR_SAVE_BONUS, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}*/
		else if (Abilities[ABILITY_STRENGTH] == 0 && strcmp(Label.Name, "Str") == 0)
		{
			Abilities[ABILITY_STRENGTH] = Field.DataOrDataOffset;
		}
		else if (Abilities[ABILITY_DEXTERITY] == 0 && strcmp(Label.Name, "Dex") == 0)
		{
			Abilities[ABILITY_DEXTERITY] = Field.DataOrDataOffset;
		}
		else if (Abilities[ABILITY_INTELLIGENCE] == 0 && strcmp(Label.Name, "Int") == 0)
		{
			Abilities[ABILITY_INTELLIGENCE] = Field.DataOrDataOffset;
		}
		else if (Abilities[ABILITY_WISDOM] == 0 && strcmp(Label.Name, "Wis") == 0)
		{
			Abilities[ABILITY_WISDOM] = Field.DataOrDataOffset;
		}
		else if (Abilities[ABILITY_CONSTITUTION] == 0 && strcmp(Label.Name, "Con") == 0)
		{
			Abilities[ABILITY_CONSTITUTION] = Field.DataOrDataOffset;
		}
		else if (Abilities[ABILITY_CHARISMA] == 0 && strcmp(Label.Name, "Cha") == 0)
		{
			Abilities[ABILITY_CHARISMA] = Field.DataOrDataOffset;
		}
		else if (nPCNbClasses == 0 && strcmp(Label.Name, "ClassList") == 0)
		{
			nPCNbClasses = GFF_GetListSize(char_file.file, &Header, &Field);
			if (nPCNbClasses == 0 ||nPCNbClasses > 3) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			GFF_ReadFieldValueFromStructInList(char_file.file, &Header, &Field, "Class", PCClasses, nPCNbClasses);
			GFF_ReadFieldValueFromStructInList(char_file.file, &Header, &Field, "ClassLevel", PCClassesLevel, nPCNbClasses);
		}
		else if (strcmp(Label.Name, "LvlStatList") == 0)
		{
			FieldLvlStatList = Field;
		}
		else if (nPCNbFeats == 0 && strcmp(Label.Name, "FeatList") == 0)
		{
			nPCNbFeats = GFF_GetListSize(char_file.file, &Header, &Field);
			PCFeats = new uint32_t[nPCNbFeats];
			PreviousLevelFeats = new uint32_t[nNbFeats];
			GFF_ReadFieldValueFromStructInList(char_file.file, &Header, &Field, "Feat", PCFeats, nPCNbFeats);
		}
		else if (PCSkills == NULL && strcmp(Label.Name, "SkillList") == 0)
		{
			if (GFF_GetListSize(char_file.file, &Header, &Field) != nNbSkills) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			PCSkills = new uint32_t[nNbSkills];
			GFF_ReadFieldValueFromStructInList(char_file.file, &Header, &Field, "Rank", PCSkills, nNbSkills);
		}
	}

	//check if the lvl up stat have been found
	if (FieldLvlStatList.DataOrDataOffset == 0) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}

	//check race
	if (nPCRace >= nNbRaces) {throw elc_error(ELCERROR_INVALID_RACE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
	if (racialtypes2da->GetInt("PlayerRace", nPCRace) != 1)
	{
		throw elc_error(ELCERROR_INVALID_RACE, format_elc_error_message("", __FILE__, __LINE__).c_str());
	}

	//Get the initial abilities (final abilities - feat (FEAT_EPIC_GREAT_*) - LvlStatAbility - Rdd abilities)

	//rdd abilities ... and check if classes are valid
	for (uint32_t nClassIndex=0; nClassIndex<nPCNbClasses; nClassIndex++)
	{
		//class not in the 2da
		if (PCClasses[nClassIndex] >= nNbClasses) {throw elc_error(ELCERROR_INVALID_CLASS, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		//not a player class
		if (classes2da->GetInt("PlayerClass", PCClasses[nClassIndex]) == 0) {throw elc_error(ELCERROR_INVALID_CLASS, format_elc_error_message("", __FILE__, __LINE__).c_str());};
		//invalid class level
		int nMaxLevel = classes2da->GetInt("MaxLevel", PCClasses[nClassIndex]);
		if (nMaxLevel != 0 && PCClassesLevel[nClassIndex] > (uint32_t)nMaxLevel) {throw elc_error(ELCERROR_INVALID_CLASS_LEVEL, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		//rdd abilities
		if (PCClasses[nClassIndex] == CLASS_TYPE_DRAGON_DISCIPLE)
		{
			if (PCClassesLevel[nClassIndex] >= 2)
			{
				Abilities[ABILITY_STRENGTH] -= 2;
				if (PCClassesLevel[nClassIndex] >= 4)
				{
					Abilities[ABILITY_STRENGTH] -= 2;
					if (PCClassesLevel[nClassIndex] >= 7)
					{
						Abilities[ABILITY_CONSTITUTION] -= 2;
						if (PCClassesLevel[nClassIndex] >= 9)
						{
							Abilities[ABILITY_INTELLIGENCE] -= 2;
							if (PCClassesLevel[nClassIndex] >= 10)
							{
								Abilities[ABILITY_CHARISMA] -= 2;
								Abilities[ABILITY_STRENGTH] -= 2;//4;
							}
						}
					}
				}
			}
		}
	}

	//lvl abilities bonus
	nNbLevel = GFF_GetListSize(char_file.file, &Header, &FieldLvlStatList);
	for (uint32_t nLevel=1; nLevel<=nNbLevel; nLevel++)
	{
		fseek(char_file.file, Header.ListIndicesOffset+FieldLvlStatList.DataOrDataOffset+(4*nLevel), SEEK_SET);
		my_fread(&nStructIndex, 4, 1, char_file.file);
		fseek(char_file.file, Header.StructOffset+(sizeof(GFF_STRUCT)*nStructIndex), SEEK_SET);
		my_fread(&Struct, sizeof(GFF_STRUCT), 1, char_file.file);
		if (Struct.FieldCount <= 1) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		for (nField=0; nField<Struct.FieldCount; nField++)
		{
			fseek(char_file.file, Header.FieldIndicesOffset+Struct.DataOrDataOffset+(nField*4), SEEK_SET);
			my_fread(&nFieldIndex, 4, 1, char_file.file);
			fseek(char_file.file, Header.FieldOffset+(sizeof(GFF_FIELD)*nFieldIndex), SEEK_SET);
			my_fread(&Field, sizeof(GFF_FIELD), 1, char_file.file);
			fseek(char_file.file, Header.LabelOffset+(sizeof(GFF_LABEL)*Field.LabelIndex), SEEK_SET);
			my_fread(&Label, sizeof(GFF_LABEL), 1, char_file.file);
			if (strcmp(Label.Name, "LvlStatAbility") == 0)
			{
				Abilities[Field.DataOrDataOffset]--;
			}
		}
	}

	//great ability feats
	for (uint32_t nFeatIndex=0; nFeatIndex<nPCNbFeats; nFeatIndex++)
	{
		if (PCFeats[nFeatIndex] >= FEAT_EPIC_GREAT_STRENGTH_1 && PCFeats[nFeatIndex] <= FEAT_EPIC_GREAT_STRENGTH_10)
			Abilities[ABILITY_STRENGTH]--;
		else if (PCFeats[nFeatIndex] >= FEAT_EPIC_GREAT_DEXTERITY_1 && PCFeats[nFeatIndex] <= FEAT_EPIC_GREAT_DEXTERITY_10)
			Abilities[ABILITY_DEXTERITY]--;
		else if (PCFeats[nFeatIndex] >= FEAT_EPIC_GREAT_INTELLIGENCE_1 && PCFeats[nFeatIndex] <= FEAT_EPIC_GREAT_INTELLIGENCE_10)
			Abilities[ABILITY_INTELLIGENCE]--;
		else if (PCFeats[nFeatIndex] >= FEAT_EPIC_GREAT_CONSTITUTION_1 && PCFeats[nFeatIndex] <= FEAT_EPIC_GREAT_CONSTITUTION_10)
			Abilities[ABILITY_CONSTITUTION]--;
		else if (PCFeats[nFeatIndex] >= FEAT_EPIC_GREAT_WISDOM_1 && PCFeats[nFeatIndex] <= FEAT_EPIC_GREAT_WISDOM_10)
			Abilities[ABILITY_WISDOM]--;
		else if (PCFeats[nFeatIndex] >= FEAT_EPIC_GREAT_CHARISMA_1 && PCFeats[nFeatIndex] <= FEAT_EPIC_GREAT_CHARISMA_10)
			Abilities[ABILITY_CHARISMA]--;
	}

	//Check the initial abilities
	nNbAbilityPoints = 30;
	for (uint32_t nAbility=0; nAbility<6; nAbility++)
	{
		if (Abilities[nAbility] < 8 || Abilities[nAbility] > 18) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}

		if (Abilities[nAbility] <= 14)
			nNbAbilityPoints -= (Abilities[nAbility]-8);
		else
		{
			switch (Abilities[nAbility])
			{
				case 15: nNbAbilityPoints -= 8;break;
				case 16: nNbAbilityPoints -= 10;break;
				case 17: nNbAbilityPoints -= 13; break;
				case 18: nNbAbilityPoints -= 16; break;
			}
		}
	}
	if (nNbAbilityPoints != 0) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
	if (classes2da->GetInt("SpellCaster", PCClasses[0]) == 1)
	{
		std::string primarty_ability = classes2da->GetString("PrimaryAbil", PCClasses[0]);
		if (primarty_ability == "STR")
		{
			if (Abilities[ABILITY_STRENGTH] < 11) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}
		else if (primarty_ability == "DEX")
		{
			if (Abilities[ABILITY_DEXTERITY] < 11) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}
		else if (primarty_ability == "INT")
		{
			if (Abilities[ABILITY_INTELLIGENCE] < 11) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}
		else if (primarty_ability == "CON")
		{
			if (Abilities[ABILITY_CONSTITUTION] < 11) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}
		else if (primarty_ability == "WIS")
		{
			if (Abilities[ABILITY_WISDOM] < 11) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}
		else if (primarty_ability == "CHA")
		{
			if (Abilities[ABILITY_CHARISMA] < 11) {throw elc_error(ELCERROR_INVALID_INITIAL_ABILITIES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		}
	}

	//Read stats tables of each class
	for (uint32_t nClassIndex=0; nClassIndex<nPCNbClasses; nClassIndex++)
	{
		std::string cls_ab_2da = classes2da->GetString("AttackBonusTable", PCClasses[nClassIndex]);
		if (cls_ab_2da.empty()) {throw elc_error(ELCERROR_INVALID_2DA_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		ClassesABTable[nClassIndex] = get_cached_2da(cls_ab_2da);
		//spell gain table
		std::string cls_spellgain_table = classes2da->GetString("SpellGainTable", PCClasses[nClassIndex]);
		if (!cls_spellgain_table.empty())
			ClassesSpellGainTable[nClassIndex] = get_cached_2da(cls_spellgain_table);
		//feats table
		std::string cls_feats_table = classes2da->GetString("FeatsTable", PCClasses[nClassIndex]);
		if (cls_feats_table.empty()) {throw elc_error(ELCERROR_INVALID_2DA_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		ClassesFeatsTable[nClassIndex] = get_cached_2da(cls_feats_table);
		//skills table
		std::string cls_skills_table = classes2da->GetString("SkillsTable", PCClasses[nClassIndex]);
		if (cls_skills_table.empty()) {throw elc_error(ELCERROR_INVALID_2DA_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		ClassesSkillsTable[nClassIndex] = get_cached_2da(cls_skills_table);
		//bonus feats table
		std::string cls_bonusfeats_table = classes2da->GetString("BonusFeatsTable", PCClasses[nClassIndex]);
		if (cls_bonusfeats_table.empty()) {throw elc_error(ELCERROR_INVALID_2DA_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		ClassesBonusFeatsTable[nClassIndex] = get_cached_2da(cls_bonusfeats_table);
		//prereq table
		std::string cls_prereq_table = classes2da->GetString("PreReqTable", PCClasses[nClassIndex]);
		if (!cls_prereq_table.empty())
			ClassesPreReqTables[nClassIndex] = get_cached_2da(cls_prereq_table);
		//base skill points
		ClassesBaseSkillPoints[nClassIndex] = classes2da->GetInt("SkillPointBase", PCClasses[nClassIndex]);
		//hit die
		ClassesHitDie[nClassIndex] = classes2da->GetInt("HitDie", PCClasses[nClassIndex]);
		//epic level
		ClassesEpicLevel[nClassIndex] = classes2da->GetInt("EpicLevel", PCClasses[nClassIndex]);
	}
	//read race feat table
	std::string race_feats_table = racialtypes2da->GetString("FeatsTable", nPCRace);
	if (!race_feats_table.empty())
		RaceFeatsTable = get_cached_2da(race_feats_table);

	//Add the race bonus to the initial abilities
	//str
	nRaceStrAdjust = racialtypes2da->GetInt("StrAdjust", nPCRace);
	Abilities[ABILITY_STRENGTH] += nRaceStrAdjust;
	//dex
	nRaceDexAdjust = racialtypes2da->GetInt("DexAdjust", nPCRace);
	Abilities[ABILITY_DEXTERITY] += nRaceDexAdjust;
	//int
	nRaceIntAdjust = racialtypes2da->GetInt("IntAdjust", nPCRace);
	Abilities[ABILITY_INTELLIGENCE] += nRaceIntAdjust;
	//con
	nRaceConAdjust = racialtypes2da->GetInt("ConAdjust", nPCRace);
	Abilities[ABILITY_CONSTITUTION] += nRaceConAdjust;
	//wis
	nRaceWisAdjust = racialtypes2da->GetInt("WisAdjust", nPCRace);
	Abilities[ABILITY_WISDOM] += nRaceWisAdjust;
	//cha
	nRaceChaAdjust = racialtypes2da->GetInt("ChaAdjust", nPCRace);
	Abilities[ABILITY_CHARISMA] += nRaceChaAdjust;

	//Check all level
	for (uint32_t nLevel=1; nLevel<=nNbLevel; nLevel++)
	{
		uint32_t nThisLevelNbFeats = 0;
		uint32_t nSkill;
		uint32_t nExtraSkillPoints;
		uint32_t nThisLevelSkillPoints = 0;
		uint32_t nThisLevelClass = 0;
		uint32_t nThisLevelHitDie = 0;
		uint32_t nClassIndex;
		uint8_t bAbility = false;
		uint8_t bHitDie = false;

		fseek(char_file.file, Header.ListIndicesOffset+FieldLvlStatList.DataOrDataOffset+(4*nLevel), SEEK_SET);
		my_fread(&nStructIndex, 4, 1, char_file.file);
		fseek(char_file.file, Header.StructOffset+(sizeof(GFF_STRUCT)*nStructIndex), SEEK_SET);
		my_fread(&Struct, sizeof(GFF_STRUCT), 1, char_file.file);

		//Read level stats
		for (nField=0; nField<Struct.FieldCount; nField++)
		{
			fseek(char_file.file, Header.FieldIndicesOffset+Struct.DataOrDataOffset+(nField*4), SEEK_SET);
			my_fread(&nFieldIndex, 4, 1, char_file.file);
			fseek(char_file.file, Header.FieldOffset+(sizeof(GFF_FIELD)*nFieldIndex), SEEK_SET);
			my_fread(&Field, sizeof(GFF_FIELD), 1, char_file.file);
			fseek(char_file.file, Header.LabelOffset+(sizeof(GFF_LABEL)*Field.LabelIndex), SEEK_SET);
			my_fread(&Label, sizeof(GFF_LABEL), 1, char_file.file);

			if (!bAbility && strcmp(Label.Name, "LvlStatAbility") == 0)
			{
				if (nLevel % 4 != 0) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
				Abilities[Field.DataOrDataOffset]++;
				bAbility = true;
			}
			else if (!bHitDie && strcmp(Label.Name, "LvlStatHitDie") == 0)
			{
				nThisLevelHitDie = Field.DataOrDataOffset;
				bHitDie = true;
			}
			else if (strcmp(Label.Name, "LvlStatClass") == 0)
			{
				nThisLevelClass = Field.DataOrDataOffset;
			}
			else if (strcmp(Label.Name, "SkillPoints") == 0)
			{
				nThisLevelSkillPoints = Field.DataOrDataOffset;
			}
			else if (strcmp(Label.Name, "SkillList") == 0)
			{
				if (ThisLevelSkills != NULL)
				{
					delete[] ThisLevelSkills;
					ThisLevelSkills = NULL;
				}
				if (GFF_GetListSize(char_file.file, &Header, &Field) != nNbSkills) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
				ThisLevelSkills = new uint32_t[nNbSkills];
				GFF_ReadFieldValueFromStructInList(char_file.file, &Header, &Field, "Rank", ThisLevelSkills, nNbSkills);
			}
			else if (strcmp(Label.Name, "FeatList") == 0)
			{
				if (ThisLevelFeats != NULL)
				{
					delete[] ThisLevelFeats;
					ThisLevelFeats = NULL;
				}
				nThisLevelNbFeats = GFF_GetListSize(char_file.file, &Header, &Field);
				if (nThisLevelNbFeats > 0)
				{
					ThisLevelFeats = new uint32_t[nThisLevelNbFeats];
					GFF_ReadFieldValueFromStructInList(char_file.file, &Header, &Field, "Feat", ThisLevelFeats, nThisLevelNbFeats);
				}
			}
		}

		//Check class
		for (nClassIndex=0; nClassIndex<nPCNbClasses; nClassIndex++)
		{
			if (PCClasses[nClassIndex] == nThisLevelClass)
			{
				if (PreviousLevelClassesLevel[nClassIndex] == 0)
				{
					//Check class prerequisite
					if (ClassesPreReqTables[nClassIndex] != NULL)
					{
						C2da* cls_prereq_table = ClassesPreReqTables[nClassIndex];
						uint32_t nNbPreReq = cls_prereq_table->GetRowCount();
						uint8_t nHasPreReqRace=1;
						uint8_t nHasPreReqFeatOr=1;
						uint8_t nHasPreReqClassOr=1;
						uint32_t nFeatIndex;
						uint32_t nClassIndex2;
						for (uint32_t nPreReq=0; nPreReq<nNbPreReq; nPreReq++)
						{
							std::string prereq_type = cls_prereq_table->GetString("ReqType", nPreReq);
							if (prereq_type == "FEAT")
							{
								uint32_t nPreReqFeat = cls_prereq_table->GetInt("ReqParam1", nPreReq);
								for (nFeatIndex=0; nFeatIndex<nPreviousLevelNbFeats; nFeatIndex++)
								{
									if (PreviousLevelFeats[nFeatIndex] == nPreReqFeat) break;
								}
								if (nFeatIndex == nPreviousLevelNbFeats) {throw elc_error(ELCERROR_NO_CLASS_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							}
							else if (prereq_type == "FEATOR")
							{
								if (nHasPreReqFeatOr != 2)
								{
									uint32_t nPreReqFeatOr = cls_prereq_table->GetInt("ReqParam1", nPreReq);
									for (nFeatIndex=0; nFeatIndex<nPreviousLevelNbFeats; nFeatIndex++)
									{
										if (PreviousLevelFeats[nFeatIndex] == nPreReqFeatOr)
										{
											nHasPreReqFeatOr=2;
											break;
										}
									}
									if (nFeatIndex == nPreviousLevelNbFeats) nHasPreReqFeatOr=0;
								}
							}
							else if (prereq_type == "SKILL")
							{
								uint32_t nPreReqSkill = cls_prereq_table->GetInt("ReqParam1", nPreReq);
								uint32_t nPreReqSkillRank = cls_prereq_table->GetInt("ReqParam2", nPreReq);
								if (PreviousLevelSkills[nPreReqSkill] < nPreReqSkillRank) {throw elc_error(ELCERROR_NO_CLASS_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							}
							else if (prereq_type == "RACE")
							{
								if (nHasPreReqRace != 2)
								{
									uint32_t nPreReqRace = cls_prereq_table->GetInt("ReqParam1", nPreReq);
									if (nPCRace == nPreReqRace) nHasPreReqRace=2; else nHasPreReqRace=0;
								}
							}
							else if (prereq_type == "BAB")
							{
								uint32_t nPreReqBAB = cls_prereq_table->GetInt("ReqParam1", nPreReq);
								uint32_t nThisLevelBAB = 0;
								for (uint32_t nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
								{
									if (PreviousLevelClassesLevel[nClassIndex2] > 0)
										nThisLevelBAB += ClassesABTable[nClassIndex2]->GetInt("BAB", PreviousLevelClassesLevel[nClassIndex2]-1);
								}
								if (nThisLevelBAB < nPreReqBAB) {throw elc_error(ELCERROR_NO_CLASS_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							}
							else if (prereq_type == "CLASSOR")
							{
								if (nHasPreReqClassOr != 2)
								{
									uint32_t nPreReqClass = cls_prereq_table->GetInt("ReqParam1", nPreReq);
									for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
									{
										if (PCClasses[nClassIndex2] == nPreReqClass && PreviousLevelClassesLevel[nClassIndex2] > 0)
										{
											nHasPreReqClassOr = 2;
											break;
										}
									}
									if (nClassIndex2 == nPCNbClasses) nHasPreReqClassOr = 0;
								}
							}
							else if (prereq_type == "ARCSPELL")
							{
								uint32_t nPreReqLevel = cls_prereq_table->GetInt("ReqParam1", nPreReq);
								for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
								{
									if (PCClasses[nClassIndex2] == CLASS_TYPE_BARD ||
										PCClasses[nClassIndex2] == CLASS_TYPE_SORCERER ||
										PCClasses[nClassIndex2] == CLASS_TYPE_WIZARD)
									{
										if (PreviousLevelClassesLevel[nClassIndex2] >= nPreReqLevel) break;
									}
								}
								if (nClassIndex2 == nPCNbClasses) {throw elc_error(ELCERROR_NO_CLASS_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							}
							else if (prereq_type == "SPELL")
							{
								uint32_t nPreReqLevel = cls_prereq_table->GetInt("ReqParam1", nPreReq);
								for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
								{
									if (ClassesSpellGainTable[nClassIndex2] != NULL)
									{
										if (PreviousLevelClassesLevel[nClassIndex2] >= nPreReqLevel) break;
									}
								}
								if (nClassIndex2 == nPCNbClasses) {throw elc_error(ELCERROR_NO_CLASS_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());};
							}
						}
						if (!(nHasPreReqFeatOr && nHasPreReqClassOr && nHasPreReqRace)) {throw elc_error(ELCERROR_NO_CLASS_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
					}
				}
				PreviousLevelClassesLevel[nClassIndex]++;
				break;
			}
		}
		if (nClassIndex == nPCNbClasses) {throw elc_error(ELCERROR_INVALID_CLASS, format_elc_error_message("", __FILE__, __LINE__).c_str());}

		//check if the class is epic at an invalid level
		if (nLevel < 21)
		{
			if (ClassesEpicLevel[nClassIndex] != 0xFFFFFFFF)
			{
				if (PreviousLevelClassesLevel[nClassIndex] > ClassesEpicLevel[nClassIndex]) {throw elc_error(ELCERROR_INVALID_CLASS_LEVEL, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
		}

		//Check skills
		nExtraSkillPoints = ClassesBaseSkillPoints[nClassIndex];
		int nIntMod = ((int)Abilities[ABILITY_INTELLIGENCE]-10);
		if (nIntMod % 2 < 0 && nIntMod % 2 != 0)
			nIntMod = (nIntMod/2)-1;
		else
			nIntMod /= 2;
		nExtraSkillPoints += nIntMod;
		if (nExtraSkillPoints < 1) nExtraSkillPoints = 1;
		if (nPCRace == RACE_HUMAN) nExtraSkillPoints++;
		if (nLevel == 1) nExtraSkillPoints *= 4; else nExtraSkillPoints += nPreviousLevelSkillPoints;
		for (nSkill=0; nSkill<nNbSkills; nSkill++)
		{
			if (ThisLevelSkills[nSkill] > 0)
			{
				PreviousLevelSkills[nSkill] += ThisLevelSkills[nSkill];
				uint32_t nClassSkillIndex;
				for (nClassSkillIndex=0; nClassSkillIndex<ClassesSkillsTable[nClassIndex]->GetRowCount(); nClassSkillIndex++)
				{
					if ((uint32_t)ClassesSkillsTable[nClassIndex]->GetInt("SkillIndex", nClassSkillIndex) == nSkill)
					{
						if (ClassesSkillsTable[nClassIndex]->GetInt("ClassSkill", nClassSkillIndex) == 1)
						{
							if (PreviousLevelSkills[nSkill] > nLevel+3) {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							else if (nExtraSkillPoints < ThisLevelSkills[nSkill]) {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							else
							{
								nExtraSkillPoints -= ThisLevelSkills[nSkill];
								break;
							}
						}
						else if (ClassesSkillsTable[nClassIndex]->GetInt("ClassSkill", nClassSkillIndex) == 0)
						{
							if (PreviousLevelSkills[nSkill] > (nLevel+3)/2) {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							else if (nExtraSkillPoints < ThisLevelSkills[nSkill]*2) {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
							else
							{
								nExtraSkillPoints -= ThisLevelSkills[nSkill]*2;
								break;
							}
						}
						else {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
					}
				}
				if (nClassSkillIndex == ClassesSkillsTable[nClassIndex]->GetRowCount()) {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
		}
		if (nExtraSkillPoints != nThisLevelSkillPoints) {throw elc_error(ELCERROR_INVALID_SKILL_RANK, format_elc_error_message("", __FILE__, __LINE__).c_str());}
		nPreviousLevelSkillPoints = nThisLevelSkillPoints;

		//put all feat gained this level in the PreviousLevelFeats array and adjust abilities
		uint32_t nFeatIndex;
		for (nFeatIndex=0; nFeatIndex<nThisLevelNbFeats; nFeatIndex++)
		{
			PreviousLevelFeats[nPreviousLevelNbFeats] = ThisLevelFeats[nFeatIndex];
			nPreviousLevelNbFeats++;
			if (nPreviousLevelNbFeats >= nNbFeats) {throw elc_error(ELCERROR_INVALID_PC_FILE, format_elc_error_message("", __FILE__, __LINE__).c_str());}

			if (ThisLevelFeats[nFeatIndex] >= FEAT_EPIC_GREAT_STRENGTH_1 && ThisLevelFeats[nFeatIndex] <= FEAT_EPIC_GREAT_STRENGTH_10)
				Abilities[ABILITY_STRENGTH]++;
			else if (ThisLevelFeats[nFeatIndex] >= FEAT_EPIC_GREAT_DEXTERITY_1 && ThisLevelFeats[nFeatIndex] <= FEAT_EPIC_GREAT_DEXTERITY_10)
				Abilities[ABILITY_DEXTERITY]++;
			else if (ThisLevelFeats[nFeatIndex] >= FEAT_EPIC_GREAT_INTELLIGENCE_1 && ThisLevelFeats[nFeatIndex] <= FEAT_EPIC_GREAT_INTELLIGENCE_10)
				Abilities[ABILITY_INTELLIGENCE]++;
			else if (ThisLevelFeats[nFeatIndex] >= FEAT_EPIC_GREAT_CONSTITUTION_1 && ThisLevelFeats[nFeatIndex] <= FEAT_EPIC_GREAT_CONSTITUTION_10)
				Abilities[ABILITY_CONSTITUTION]++;
			else if (ThisLevelFeats[nFeatIndex] >= FEAT_EPIC_GREAT_WISDOM_1 && ThisLevelFeats[nFeatIndex] <= FEAT_EPIC_GREAT_WISDOM_10)
				Abilities[ABILITY_WISDOM]++;
			else if (ThisLevelFeats[nFeatIndex] >= FEAT_EPIC_GREAT_CHARISMA_1 && ThisLevelFeats[nFeatIndex] <= FEAT_EPIC_GREAT_CHARISMA_10)
				Abilities[ABILITY_CHARISMA]++;
		}

		//Check feats
		uint32_t nNbNormalFeat = 0;
		if (nLevel==1 || nLevel%3==0) nNbNormalFeat = 1;
		if (nPCRace == RACE_HUMAN && nLevel == 1) nNbNormalFeat++;
		uint32_t nNbBonusFeat = ClassesBonusFeatsTable[nClassIndex]->GetInt("Bonus", PreviousLevelClassesLevel[nClassIndex]-1);
		uint32_t nNbNormalOrBonusFeat = 0;
		uint32_t nNbClericDomain = 0;
		if (nThisLevelClass == CLASS_TYPE_CLERIC && PreviousLevelClassesLevel[nClassIndex] == 1) nNbClericDomain = 2;
		for (nFeatIndex=0; nFeatIndex<nThisLevelNbFeats; nFeatIndex++)
		{
			if (ThisLevelFeats[nFeatIndex] >= nNbFeats) {throw elc_error(ELCERROR_INVALID_FEAT, format_elc_error_message("", __FILE__, __LINE__).c_str());} //this feat is not in the 2da

			if (ThisLevelFeats[nFeatIndex] == FEAT_EPIC_CHARACTER) continue; //exception

			int bNextFeat = false;
			//check if its a racial feat
			if (nLevel == 1)
			{
				for (uint32_t nRaceFeatIndex=0; nRaceFeatIndex<RaceFeatsTable->GetRowCount(); nRaceFeatIndex++)
				{
					if ((uint32_t)RaceFeatsTable->GetInt("FeatIndex", nRaceFeatIndex) == ThisLevelFeats[nFeatIndex]) {bNextFeat = true; break;}
				}
				if (bNextFeat) continue;
			}
			//check if its a cleric domain
			if (ThisLevelFeats[nFeatIndex] >= FEAT_WAR_DOMAIN_POWER && ThisLevelFeats[nFeatIndex] <= FEAT_WATER_DOMAIN_POWER)
			{
				if (nNbClericDomain == 0) {throw elc_error(ELCERROR_TOO_MANY_FEAT, format_elc_error_message("", __FILE__, __LINE__).c_str());}
				nNbClericDomain--;
				continue;
			}
			//check if its a class feat
			int bIsAClassFeat = false;
			for (uint32_t nClassFeatIndex=0; nClassFeatIndex<ClassesFeatsTable[nClassIndex]->GetRowCount(); nClassFeatIndex++)
			{
				if ((uint32_t)ClassesFeatsTable[nClassIndex]->GetInt("FeatIndex", nClassFeatIndex) == ThisLevelFeats[nFeatIndex])
				{
					if (ClassesFeatsTable[nClassIndex]->GetInt("List", nClassFeatIndex) == 0 && ClassesFeatsTable[nClassIndex]->GetInt("GrantedOnLevel", nClassFeatIndex) <= (int)PreviousLevelClassesLevel[nClassIndex])
					{
						if (nNbNormalFeat == 0){throw elc_error(ELCERROR_TOO_MANY_FEAT, format_elc_error_message("", __FILE__, __LINE__).c_str());}
						nNbNormalFeat--;
						bIsAClassFeat = true;
					}
					else if (ClassesFeatsTable[nClassIndex]->GetInt("List", nClassFeatIndex) == 1 && ClassesFeatsTable[nClassIndex]->GetInt("GrantedOnLevel", nClassFeatIndex) <= (int)PreviousLevelClassesLevel[nClassIndex])
					{
						nNbNormalOrBonusFeat++;
						bIsAClassFeat = true;
					}
					else if (ClassesFeatsTable[nClassIndex]->GetInt("List", nClassFeatIndex) == 2 && ClassesFeatsTable[nClassIndex]->GetInt("GrantedOnLevel", nClassFeatIndex) <= (int)PreviousLevelClassesLevel[nClassIndex])
					{
						if (nNbBonusFeat == 0){throw elc_error(ELCERROR_TOO_MANY_FEAT, format_elc_error_message("", __FILE__, __LINE__).c_str());}
						nNbBonusFeat--;
						bIsAClassFeat = true;
					}
					else if ((uint32_t)ClassesFeatsTable[nClassIndex]->GetInt("List", nClassFeatIndex) == 3 && (uint32_t)ClassesFeatsTable[nClassIndex]->GetInt("GrantedOnLevel", nClassFeatIndex) == PreviousLevelClassesLevel[nClassIndex])
					{
						bNextFeat = true;
					}
					break;
				}
			}
			if (bNextFeat) continue;
			//check for pre requisite
			uint32_t nClassIndex2;
			uint32_t nFeatIndex2;
			uint8_t nOrReqFeat = 1;
			if (!bIsAClassFeat)
			{
				if (!feats2da->GetInt("ALLCLASSESCANUSE", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
				if (nNbNormalFeat == 0){throw elc_error(ELCERROR_TOO_MANY_FEAT, format_elc_error_message("", __FILE__, __LINE__).c_str());}
				nNbNormalFeat--;
			}
			if (!feats2da->GetString("MINATTACKBONUS", ThisLevelFeats[nFeatIndex]).empty())
			{
				uint32_t nBAB = 0;
				for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
				{
					if (PreviousLevelClassesLevel[nClassIndex2] > 0)
						nBAB += ClassesABTable[nClassIndex2]->GetInt("BAB", PreviousLevelClassesLevel[nClassIndex2]-1);
				}
				if (nBAB < (uint32_t)feats2da->GetInt("MINATTACKBONUS", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MINSTR", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (Abilities[ABILITY_STRENGTH] < (uint32_t)feats2da->GetInt("MINSTR", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MINDEX", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (Abilities[ABILITY_DEXTERITY] < (uint32_t)feats2da->GetInt("MINDEX", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MININT", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (Abilities[ABILITY_INTELLIGENCE] < (uint32_t)feats2da->GetInt("MININT", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MINWIS", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (Abilities[ABILITY_WISDOM] < (uint32_t)feats2da->GetInt("MINWIS", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MINCHA", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (Abilities[ABILITY_CHARISMA] < (uint32_t)feats2da->GetInt("MINCHA", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MINSPELLLVL", ThisLevelFeats[nFeatIndex]).empty())
			{
				uint32_t nPaleMasterBonus = 0;
				for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
				{
					if (PCClasses[nClassIndex2] == CLASS_TYPE_PALE_MASTER && PreviousLevelClassesLevel[nClassIndex2] > 0)
					{
						nPaleMasterBonus = (PreviousLevelClassesLevel[nClassIndex2]-1)/2 + 1;
					}
				}
				for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
				{
					if ((ClassesSpellGainTable[nClassIndex2]) != NULL && PreviousLevelClassesLevel[nClassIndex2] > 0)
					{
						if (ClassesSpellGainTable[nClassIndex2]->GetInt("NumSpellLevels", PreviousLevelClassesLevel[nClassIndex2]-1+nPaleMasterBonus)-1 >= feats2da->GetInt("MINSPELLLVL", ThisLevelFeats[nFeatIndex])) break;
					}
				}
				if (nClassIndex2 == nPCNbClasses) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("PREREQFEAT1", ThisLevelFeats[nFeatIndex]).empty())
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("PREREQFEAT1", ThisLevelFeats[nFeatIndex])) break;
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("PREREQFEAT2", ThisLevelFeats[nFeatIndex]).empty())
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("PREREQFEAT2", ThisLevelFeats[nFeatIndex])) break;
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("OrReqFeat0", ThisLevelFeats[nFeatIndex]).empty())
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("OrReqFeat0", ThisLevelFeats[nFeatIndex]))
					{
						nOrReqFeat = 2;
						break;
					}
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) nOrReqFeat = 0;
			}
			if (!feats2da->GetString("OrReqFeat1", ThisLevelFeats[nFeatIndex]).empty() && nOrReqFeat != 2)
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("OrReqFeat1", ThisLevelFeats[nFeatIndex]))
					{
						nOrReqFeat = 2;
						break;
					}
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) nOrReqFeat = 0;
			}
			if (!feats2da->GetString("OrReqFeat2", ThisLevelFeats[nFeatIndex]).empty() && nOrReqFeat != 2)
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("OrReqFeat2", ThisLevelFeats[nFeatIndex]))
					{
						nOrReqFeat = 2;
						break;
					}
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) nOrReqFeat = 0;
			}
			if (!feats2da->GetString("OrReqFeat3", ThisLevelFeats[nFeatIndex]).empty() && nOrReqFeat != 2)
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("OrReqFeat3", ThisLevelFeats[nFeatIndex]))
					{
						nOrReqFeat = 2;
						break;
					}
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) nOrReqFeat = 0;
			}
			if (!feats2da->GetString("OrReqFeat4", ThisLevelFeats[nFeatIndex]).empty() && nOrReqFeat != 2)
			{
				for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
				{
					if (PreviousLevelFeats[nFeatIndex2] == (uint32_t)feats2da->GetInt("OrReqFeat4", ThisLevelFeats[nFeatIndex]))
					{
						nOrReqFeat = 2;
						break;
					}
				}
				if (nFeatIndex2 == nPreviousLevelNbFeats) nOrReqFeat = 0;
			}
			if (nOrReqFeat == 0) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			if (!feats2da->GetString("REQSKILL", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (PreviousLevelSkills[feats2da->GetInt("REQSKILL", ThisLevelFeats[nFeatIndex])] < (uint32_t)feats2da->GetInt("ReqSkillMinRanks", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("REQSKILL2", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (PreviousLevelSkills[feats2da->GetInt("REQSKILL2", ThisLevelFeats[nFeatIndex])] < (uint32_t)feats2da->GetInt("ReqSkillMinRanks2", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MinLevelClass", ThisLevelFeats[nFeatIndex]).empty())
			{
				uint32_t min_level = feats2da->GetInt("MinLevel", ThisLevelFeats[nFeatIndex]);
				if (min_level == 0) min_level = 1;
				for (nClassIndex2=0; nClassIndex2<nPCNbClasses; nClassIndex2++)
				{
					if (PCClasses[nClassIndex2] == (uint32_t)feats2da->GetInt("MinLevelClass", ThisLevelFeats[nFeatIndex]))
					{
						if (PreviousLevelClassesLevel[nClassIndex2] < min_level) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
						break;
					}
				}
				if (nClassIndex2 == nPCNbClasses) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			if (!feats2da->GetString("MaxLevel", ThisLevelFeats[nFeatIndex]).empty())
			{
				if (nLevel > (uint32_t)feats2da->GetInt("MaxLevel", ThisLevelFeats[nFeatIndex])) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			/*
			if (Feat.MinFortSave != MY2DAVALUE_NULL)
			{
			}
			*/
			if (feats2da->GetInt("PreReqEpic", ThisLevelFeats[nFeatIndex]))
			{
				if (nLevel < 21) {throw elc_error(ELCERROR_NO_FEAT_PREREQ, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
		}
		if (nNbNormalOrBonusFeat > nNbNormalFeat+nNbBonusFeat){throw elc_error(ELCERROR_TOO_MANY_FEAT, format_elc_error_message("", __FILE__, __LINE__).c_str());}

		//check HitDie
		if (nThisLevelClass == CLASS_TYPE_DRAGON_DISCIPLE)
		{
			if (PreviousLevelClassesLevel[nClassIndex] >= 11)
			{
				if (nThisLevelHitDie > 12) {throw elc_error(ELCERROR_INVALID_HITDIE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			else if (PreviousLevelClassesLevel[nClassIndex] >= 6)
			{
				if (nThisLevelHitDie > 10) {throw elc_error(ELCERROR_INVALID_HITDIE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			else if (PreviousLevelClassesLevel[nClassIndex] >= 4)
			{
				if (nThisLevelHitDie > 8) {throw elc_error(ELCERROR_INVALID_HITDIE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			else
			{
				if (nThisLevelHitDie > ClassesHitDie[nClassIndex]) {throw elc_error(ELCERROR_INVALID_HITDIE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
			}
			//apply dragon abilities bonus
			if (PreviousLevelClassesLevel[nClassIndex] == 2)
				Abilities[ABILITY_STRENGTH] += 2;
			else if (PreviousLevelClassesLevel[nClassIndex] == 4)
				Abilities[ABILITY_STRENGTH] += 2;
			else if (PreviousLevelClassesLevel[nClassIndex] == 7)
				Abilities[ABILITY_CONSTITUTION] += 2;
			else if (PreviousLevelClassesLevel[nClassIndex] == 9)
				Abilities[ABILITY_INTELLIGENCE] += 2;
			else if (PreviousLevelClassesLevel[nClassIndex] == 10)
			{
				Abilities[ABILITY_STRENGTH] += 2;//4;
				Abilities[ABILITY_CHARISMA] += 2;
			}
		}
		else if (nThisLevelHitDie > ClassesHitDie[nClassIndex]) {throw elc_error(ELCERROR_INVALID_HITDIE, format_elc_error_message("", __FILE__, __LINE__).c_str());}
	}//end checking this level

	//check if the final feat, skills and classes match with the level stat
	for (uint16_t nClassIndex=0; nClassIndex<nPCNbClasses; nClassIndex++)
	{
		if (PCClassesLevel[nClassIndex] != PreviousLevelClassesLevel[nClassIndex]) {throw elc_error(ELCERROR_INVALID_FINAL_CLASSES, format_elc_error_message("", __FILE__, __LINE__).c_str());}
	}
	for (uint32_t nFeatIndex=0; nFeatIndex<nPCNbFeats; nFeatIndex++)
	{
		//the final feats may not be in order so we need to search each feats
		uint32_t nFeatIndex2;
		for (nFeatIndex2=0; nFeatIndex2<nPreviousLevelNbFeats; nFeatIndex2++)
		{
			if (PCFeats[nFeatIndex] == PreviousLevelFeats[nFeatIndex2])
			{
				PreviousLevelFeats[nFeatIndex2] = (uint32_t)-1;//remove this feat from the array by making it invalid
				break;
			}
		}
		if (nFeatIndex2 == nPreviousLevelNbFeats) {throw elc_error(ELCERROR_INVALID_FINAL_FEATS, format_elc_error_message("", __FILE__, __LINE__).c_str());}
	}
	for (uint32_t nSkill=0; nSkill<nNbSkills; nSkill++)
	{
		if (PCSkills[nSkill] != PreviousLevelSkills[nSkill]) {throw elc_error(ELCERROR_INVALID_FINAL_SKILLS, format_elc_error_message("", __FILE__, __LINE__).c_str());}
	}

} catch (elc_error& e) {
	nReturn = e.error_code;
	fprintf(stderr, "%s:%d:%s\n", szCharacterFile, e.error_code, e.error_message.c_str());
}

	if (ThisLevelFeats != NULL) delete[] ThisLevelFeats;
	if (ThisLevelSkills != NULL) delete[] ThisLevelSkills;
	if (PreviousLevelSkills != NULL) delete[] PreviousLevelSkills;
	if (PreviousLevelFeats != NULL) delete[] PreviousLevelFeats;

	if (PCSkills != NULL) delete[] PCSkills;
	if (PCFeats != NULL) delete[] PCFeats;

	return nReturn;
}

uint32_t GFF_GetListSize(FILE* fGFF, GFF_HEADER* Header, GFF_FIELD* ListField)
{
	uint32_t nListSize;
	fseek(fGFF, Header->ListIndicesOffset+ListField->DataOrDataOffset, SEEK_SET);
	my_fread(&nListSize, 4, 1, fGFF);
	return nListSize;
}

void GFF_ReadFieldValueFromStructInList(FILE* fGFF, GFF_HEADER* Header, GFF_FIELD* ListField, const char* LabelName, uint32_t* Buffer, uint32_t nListSize)
{
	uint32_t nStructIndex;
	GFF_STRUCT Struct;
	uint32_t nFieldIndex;
	GFF_FIELD Field;
	GFF_LABEL Label;
	for (uint32_t nStruct=1; nStruct<=nListSize; nStruct++)
	{
		fseek(fGFF, Header->ListIndicesOffset+ListField->DataOrDataOffset+(nStruct*4), SEEK_SET);
		my_fread(&nStructIndex, 4, 1, fGFF);
		fseek(fGFF, Header->StructOffset+(sizeof(GFF_STRUCT)*nStructIndex), SEEK_SET);
		my_fread(&Struct, sizeof(GFF_STRUCT), 1, fGFF);
		if (Struct.FieldCount > 1)
		{

			for (uint32_t nField=0; nField<Struct.FieldCount; nField++)
			{
				fseek(fGFF, Header->FieldIndicesOffset+Struct.DataOrDataOffset+(nField*4), SEEK_SET);
				my_fread(&nFieldIndex, 4, 1, fGFF);
				fseek(fGFF, Header->FieldOffset+(sizeof(GFF_FIELD)*nFieldIndex), SEEK_SET);
				my_fread(&Field, sizeof(GFF_FIELD), 1, fGFF);
				fseek(fGFF, Header->LabelOffset+(sizeof(GFF_LABEL)*Field.LabelIndex), SEEK_SET);
				my_fread(&Label, sizeof(GFF_LABEL), 1, fGFF);
				if (strcmp(Label.Name, LabelName) == 0)
				{
					Buffer[nStruct-1] = Field.DataOrDataOffset;
					continue;
				}
			}

		}
		else
		{
			fseek(fGFF, Header->FieldOffset+(sizeof(GFF_FIELD)*Struct.DataOrDataOffset), SEEK_SET);
			my_fread(&Field, sizeof(GFF_FIELD), 1, fGFF);
			fseek(fGFF, Header->LabelOffset+(sizeof(GFF_LABEL)*Field.LabelIndex), SEEK_SET);
			my_fread(&Label, sizeof(GFF_LABEL), 1, fGFF);
			if (strcmp(Label.Name, LabelName) == 0)
			{
				Buffer[nStruct-1] = Field.DataOrDataOffset;
			}
		}
	}
}

VM_FUNC_NEW(EnforceLegalCharacter, 14)
{
	CExoString file_path = vm_pop_string();
	int result = 0;
	if (file_path.text)
	{
		result = EnforceLegalCharacter(file_path.text);
	}
	vm_push_int(result);
}

}
