#pragma once

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <map>
#include <memory>

#define ITEM_APPR_ARMOR_MODEL_RFOOT			0
#define ITEM_APPR_ARMOR_MODEL_LFOOT			1
#define ITEM_APPR_ARMOR_MODEL_RSHIN			2
#define ITEM_APPR_ARMOR_MODEL_LSHIN 		3
#define ITEM_APPR_ARMOR_MODEL_LTHIGH		4
#define ITEM_APPR_ARMOR_MODEL_RTHIGH		5
#define ITEM_APPR_ARMOR_MODEL_PELVIS		6
#define ITEM_APPR_ARMOR_MODEL_TORSO			7
#define ITEM_APPR_ARMOR_MODEL_BELT			8
#define ITEM_APPR_ARMOR_MODEL_NECK			9
#define ITEM_APPR_ARMOR_MODEL_RFOREARM		10
#define ITEM_APPR_ARMOR_MODEL_LFOREARM		11
#define ITEM_APPR_ARMOR_MODEL_RBICEP		12
#define ITEM_APPR_ARMOR_MODEL_LBICEP		13
#define ITEM_APPR_ARMOR_MODEL_RSHOULDER		14
#define ITEM_APPR_ARMOR_MODEL_LSHOULDER		15
#define ITEM_APPR_ARMOR_MODEL_RHAND			16
#define ITEM_APPR_ARMOR_MODEL_LHAND			17
#define ITEM_APPR_ARMOR_MODEL_ROBE			18
#define ITEM_APPR_ARMOR_NUM_MODELS			19

uint8_t hex_to_dec(char hex_char);
uint8_t read_pc_part_xdata_byte(const char*& xdata);
int read_pc_part_xdata_flags(const char*& xdata);
uint16_t read_pc_part_xdata_colors(const char*& xdata);
float read_pc_part_xdata_float(const char*& xdata);
std::string get_pc_part_xdata_byte(uint8_t value);
std::string get_pc_part_xdata_flags(int flags);
std::string get_pc_part_xdata_colors(uint16_t colors);
std::string get_pc_part_xdata_float(float value);

//item parts
#define NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PARTS			0x00000001
#define NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART1			0x00000002
#define NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART2			0x00000004
#define NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART3			0x00000008

//armor parts
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ROBE			0x00000004
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_BELT			0x00000008
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_NECK			0x00000010
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_PELVIS_TORSO	0x00000020
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ARMS			0x00000040
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_LEGS			0x00000080
//sub to NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_PELVIS_TORSO
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_PELVIS	0x01
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_TORSO	0x02
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_HELM	0x04
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_CLOAK	0x08
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_WINGS	0x10
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_TAIL	0x20
//sub to NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ARMS
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LSHOULDER	0x01
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RSHOULDER	0x02
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LBICEP		0x04
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RBICEP		0x08
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LFOREARM		0x10
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RFOREARM		0x20
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LHAND		0x40
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RHAND		0x80
//sub to NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_LEGS
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LTHIGH		0x01
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RTHIGH		0x02
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LSHIN		0x04
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RSHIN		0x08
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LFOOT		0x10
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RFOOT		0x20
//armor part xdata
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORMAP 		0x01
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_LIGHTMOD 		0x02
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_SCALE 			0x04
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEX 			0x08
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEY 			0x10
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEZ 			0x20
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORSOVERRIDE 	0x40
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEX 		0x0100
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEY 		0x0200
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEZ 		0x0400
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_RGBA_OVERRIDE 		0x0800
#define NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_TEXTURE_MAPPING 		0x1000
//for array size..
#define NUM_PLT_COLOR_CHANNELS 10

typedef int8_t RGBA_MOD[15];

class PC_PART_XDATA
{
public:
	typedef std::map<std::string, std::string> MAPPED_TEXTURES;
private:
	void copy_data(const PC_PART_XDATA& copy);
	void delete_pointers();
	MAPPED_TEXTURES* mapped_textures;
public:
	void clear();
	PC_PART_XDATA();
	PC_PART_XDATA(const PC_PART_XDATA& to_copy);
	PC_PART_XDATA& operator=(const PC_PART_XDATA& other);
	virtual ~PC_PART_XDATA();
	bool equals(PC_PART_XDATA& to);
	
	bool has_data();
	bool has_rgba_mod();
	bool has_texture_data();
	
	std::shared_ptr<MAPPED_TEXTURES::iterator> get_mapped_textures_iterator();
	std::string get_mapped_texture_from(std::shared_ptr<MAPPED_TEXTURES::iterator>& iter);
	std::string get_mapped_texture_to(std::shared_ptr<MAPPED_TEXTURES::iterator>& iter);
	bool increment_mapped_textures_iterator(std::shared_ptr<MAPPED_TEXTURES::iterator>& iter);
	
	void map_texture(const std::string& from, const std::string& to);
	void unmap_texture(const std::string& from);
	std::string get_mapped_texture(std::string texture);
	bool has_mapped_textures();
	
	void read_pc_part_xdata(const char*& xdata);
	std::string get_pc_part_xdata();
	
	int8_t get_rgba_mod(uint8_t rgba);
	void set_rgba_mod(uint8_t rgba, int8_t val);

	uint8_t channels_mappping[NUM_PLT_COLOR_CHANNELS];
	uint8_t channels_color[NUM_PLT_COLOR_CHANNELS];
	uint16_t channels_color_override;
	int8_t channels_lightness_mod[NUM_PLT_COLOR_CHANNELS];
	float scaling;
	float extra_x;
	float extra_y;
	float extra_z;
	float extra_rotate_x;
	float extra_rotate_y;
	float extra_rotate_z;
	RGBA_MOD rgba_mod;
};

template<typename PART_DATA> struct ITEM_PARTS
{
	void read_item_parts_xdata(const char* xdata)
	{
		uint8_t item_parts_with_xdata = read_pc_part_xdata_byte(xdata);
		if (item_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PARTS)
		{
			parts[0].read_pc_part_xdata(xdata);
		}
		if (item_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART1)
		{
			parts[1].read_pc_part_xdata(xdata);
		}
		if (item_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART2)
		{
			parts[2].read_pc_part_xdata(xdata);
		}
		if (item_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART3)
		{
			parts[3].read_pc_part_xdata(xdata);
		}
	}
	std::string get_item_parts_xdata()
	{
		uint8_t item_parts_with_xdata_flags = 0;
		std::string parts_xdata = parts[0].get_pc_part_xdata();
		if (!parts_xdata.empty()) item_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PARTS;
		std::string part1_xdata = parts[1].get_pc_part_xdata();
		if (!part1_xdata.empty()) item_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART1;
		std::string part2_xdata = parts[2].get_pc_part_xdata();
		if (!part2_xdata.empty()) item_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART2;
		std::string part3_xdata = parts[3].get_pc_part_xdata();
		if (!part3_xdata.empty()) item_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ITEM_PART3;

		std::string result = get_pc_part_xdata_byte(item_parts_with_xdata_flags);
		if (item_parts_with_xdata_flags)
		{
			result += parts_xdata;
			result += part1_xdata;
			result += part2_xdata;
			result += part3_xdata;
		}
		return result;
	}
	bool equals(ITEM_PARTS<PART_DATA>& to)
	{
		for (int i=0; i<get_num_parts(); i++)
		{
			if (!parts[i].equals(to.parts[i])) return false;
		}
		return true;
	}
	PART_DATA& operator[](uint8_t index){return parts[index];}
	inline int get_num_parts() {return sizeof(parts)/sizeof(PART_DATA);}
	PART_DATA parts[4];
};

template<typename PART_DATA> struct PC_PARTS
{
	void read_pc_parts_xdata(uint8_t pc_parts_with_xdata, const char* xdata)
	{
		if (pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ROBE)
		{
			parts[ITEM_APPR_ARMOR_MODEL_ROBE].read_pc_part_xdata(xdata);
		}
		if (pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_BELT)
		{
			parts[ITEM_APPR_ARMOR_MODEL_BELT].read_pc_part_xdata(xdata);
		}
		if (pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_NECK)
		{
			parts[ITEM_APPR_ARMOR_MODEL_NECK].read_pc_part_xdata(xdata);
		}
		if (pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_PELVIS_TORSO)
		{
			uint8_t sub_pc_parts_with_xdata = read_pc_part_xdata_byte(xdata);
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_PELVIS)
			{
				parts[ITEM_APPR_ARMOR_MODEL_PELVIS].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_TORSO)
			{
				parts[ITEM_APPR_ARMOR_MODEL_TORSO].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_HELM)
			{
				parts[19].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_CLOAK)
			{
				parts[20].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_WINGS)
			{
				parts[21].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_TAIL)
			{
				parts[22].read_pc_part_xdata(xdata);
			}
		}
		if (pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ARMS)
		{
			uint8_t sub_pc_parts_with_xdata = read_pc_part_xdata_byte(xdata);
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LSHOULDER)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LSHOULDER].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RSHOULDER)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RSHOULDER].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LBICEP)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LBICEP].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RBICEP)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RBICEP].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LFOREARM)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LFOREARM].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RFOREARM)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RFOREARM].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LHAND)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LHAND].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RHAND)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RHAND].read_pc_part_xdata(xdata);
			}
		}
		if (pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_LEGS)
		{
			uint8_t sub_pc_parts_with_xdata = read_pc_part_xdata_byte(xdata);
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LTHIGH)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LTHIGH].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RTHIGH)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RTHIGH].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LSHIN)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LSHIN].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RSHIN)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RSHIN].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LFOOT)
			{
				parts[ITEM_APPR_ARMOR_MODEL_LFOOT].read_pc_part_xdata(xdata);
			}
			if (sub_pc_parts_with_xdata & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RFOOT)
			{
				parts[ITEM_APPR_ARMOR_MODEL_RFOOT].read_pc_part_xdata(xdata);
			}
		}
	}
	std::string get_pc_parts_xdata()
	{
		uint8_t pc_parts_with_xdata_flags = 0;
		uint8_t trunk_parts_with_xdata_flags = 0;
		uint8_t arms_parts_with_xdata_flags = 0;
		uint8_t legs_parts_with_xdata_flags = 0;
		std::string robe_xdata = parts[ITEM_APPR_ARMOR_MODEL_ROBE].get_pc_part_xdata();
		if (!robe_xdata.empty()) pc_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ROBE;
		std::string belt_xdata = parts[ITEM_APPR_ARMOR_MODEL_BELT].get_pc_part_xdata();
		if (!belt_xdata.empty()) pc_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_BELT;
		std::string neck_xdata = parts[ITEM_APPR_ARMOR_MODEL_NECK].get_pc_part_xdata();
		if (!neck_xdata.empty()) pc_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_NECK;
	
		std::string pelvis_xdata = parts[ITEM_APPR_ARMOR_MODEL_PELVIS].get_pc_part_xdata();
		if (!pelvis_xdata.empty()) trunk_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_PELVIS;
		std::string torso_xdata = parts[ITEM_APPR_ARMOR_MODEL_TORSO].get_pc_part_xdata();
		if (!torso_xdata.empty()) trunk_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_TORSO;
		std::string helm_xdata = parts[19].get_pc_part_xdata();
		if (!helm_xdata.empty()) trunk_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_HELM;
		std::string cloak_xdata = parts[20].get_pc_part_xdata();
		if (!cloak_xdata.empty()) trunk_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_CLOAK;
		std::string wings_xdata = parts[21].get_pc_part_xdata();
		if (!wings_xdata.empty()) trunk_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_WINGS;
		std::string tail_xdata = parts[22].get_pc_part_xdata();
		if (!tail_xdata.empty()) trunk_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_TRUNK_TAIL;
		if (trunk_parts_with_xdata_flags) pc_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_PELVIS_TORSO;

		std::string lshoulder_xdata = parts[ITEM_APPR_ARMOR_MODEL_LSHOULDER].get_pc_part_xdata();
		if (!lshoulder_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LSHOULDER;
		std::string rshoulder_xdata = parts[ITEM_APPR_ARMOR_MODEL_RSHOULDER].get_pc_part_xdata();
		if (!rshoulder_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RSHOULDER;
		std::string lbicep_xdata = parts[ITEM_APPR_ARMOR_MODEL_LBICEP].get_pc_part_xdata();
		if (!lbicep_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LBICEP;
		std::string rbicep_xdata = parts[ITEM_APPR_ARMOR_MODEL_RBICEP].get_pc_part_xdata();
		if (!rbicep_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RBICEP;
		std::string lforearm_xdata = parts[ITEM_APPR_ARMOR_MODEL_LFOREARM].get_pc_part_xdata();
		if (!lforearm_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LFOREARM;
		std::string rforearm_xdata = parts[ITEM_APPR_ARMOR_MODEL_RFOREARM].get_pc_part_xdata();
		if (!rforearm_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RFOREARM;
		std::string lhand_xdata = parts[ITEM_APPR_ARMOR_MODEL_LHAND].get_pc_part_xdata();
		if (!lhand_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_LHAND;
		std::string rhand_xdata = parts[ITEM_APPR_ARMOR_MODEL_RHAND].get_pc_part_xdata();
		if (!rhand_xdata.empty()) arms_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_ARMS_RHAND;
		if (arms_parts_with_xdata_flags) pc_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_ARMS;

		std::string lthigh_xdata = parts[ITEM_APPR_ARMOR_MODEL_LTHIGH].get_pc_part_xdata();
		if (!lthigh_xdata.empty()) legs_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LTHIGH;
		std::string rthigh_xdata = parts[ITEM_APPR_ARMOR_MODEL_RTHIGH].get_pc_part_xdata();
		if (!rthigh_xdata.empty()) legs_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RTHIGH;
		std::string lshin_xdata = parts[ITEM_APPR_ARMOR_MODEL_LSHIN].get_pc_part_xdata();
		if (!lshin_xdata.empty()) legs_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LSHIN;
		std::string rshin_xdata = parts[ITEM_APPR_ARMOR_MODEL_RSHIN].get_pc_part_xdata();
		if (!rshin_xdata.empty()) legs_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RSHIN;
		std::string lfoot_xdata = parts[ITEM_APPR_ARMOR_MODEL_LFOOT].get_pc_part_xdata();
		if (!lfoot_xdata.empty()) legs_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_LFOOT;
		std::string rfoot_xdata = parts[ITEM_APPR_ARMOR_MODEL_RFOOT].get_pc_part_xdata();
		if (!rfoot_xdata.empty()) legs_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_SUB_LEGS_RFOOT;
		if (legs_parts_with_xdata_flags) pc_parts_with_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_LEGS;

		std::string result;
		if (pc_parts_with_xdata_flags)
		{
			result = get_pc_part_xdata_byte(pc_parts_with_xdata_flags);
			result += robe_xdata;
			result += belt_xdata;
			result += neck_xdata;
			if (trunk_parts_with_xdata_flags)
			{
				result += get_pc_part_xdata_byte(trunk_parts_with_xdata_flags);
				result += pelvis_xdata;
				result += torso_xdata;
				result += helm_xdata;
				result += cloak_xdata;
				result += wings_xdata;
				result += tail_xdata;
			}
			if (arms_parts_with_xdata_flags)
			{
				result += get_pc_part_xdata_byte(arms_parts_with_xdata_flags);
				result += lshoulder_xdata;
				result += rshoulder_xdata;
				result += lbicep_xdata;
				result += rbicep_xdata;
				result += lforearm_xdata;
				result += rforearm_xdata;
				result += lhand_xdata;
				result += rhand_xdata;
			}
			if (legs_parts_with_xdata_flags)
			{
				result += get_pc_part_xdata_byte(legs_parts_with_xdata_flags);
				result += lthigh_xdata;
				result += rthigh_xdata;
				result += lshin_xdata;
				result += rshin_xdata;
				result += lfoot_xdata;
				result += rfoot_xdata;
			}
		}
		return result;
	}
	bool equals(PC_PARTS<PART_DATA>& to)
	{
		for (int i=0; i<get_num_parts(); i++)
		{
			if (!parts[i].equals(to.parts[i])) return false;
		}
		return true;
	}
	PART_DATA& operator[](uint8_t index){return parts[index];}
	inline int get_num_parts() {return sizeof(parts)/sizeof(PART_DATA);}
	PART_DATA parts[23];
};

typedef ITEM_PARTS<PC_PART_XDATA> ITEM_PARTS_XDATA;
typedef PC_PARTS<PC_PART_XDATA> PC_PARTS_XDATA;