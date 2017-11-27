#include "xdata.h"
#include <algorithm>
#ifdef XDATA_READ_FROM_MESSAGE
#include "../../nwncx/nwncx.h"
#endif

uint8_t hex_to_dec(char hex_char)
{
	if (hex_char >= '0' && hex_char <= '9') return hex_char-'0';
	if (hex_char >= 'A' && hex_char <= 'F') return 10+(hex_char-'A');
	if (hex_char >= 'a' && hex_char <= 'f') return 10+(hex_char-'a');
	return 0;
}
#ifdef XDATA_READ_FROM_MESSAGE
uint8_t read_pc_part_xdata_byte(const char*& xdata)
{
	return nwncx::read_msg_byte((nwncx::CNWMessage*)xdata);
}
float read_pc_part_xdata_float(const char*& xdata)
{
	return nwncx::read_msg_float((nwncx::CNWMessage*)xdata);
}
#else
uint8_t read_pc_part_xdata_byte(const char*& xdata)
{
	uint8_t val = 0;
	if (*xdata)
	{
		val = hex_to_dec(*xdata) << 4;
		xdata++;
		if (*xdata)
		{
			val |= hex_to_dec(*xdata);
			xdata++;
		}
	}
	return val;
}
float read_pc_part_xdata_float(const char*& xdata)
{
	uint32_t val = 0;
	for (int i=0; i<4; i++)
	{
		val |= (read_pc_part_xdata_byte(xdata) << (i*8));
	}
	return *(float*)&val;
}
#endif
namespace
{
	const char XDATA_CHAR_TO_ASCII_TABLE[] = "abcdefghijklmnopqrstuvwxyz0123456789_-|¦,.:;!@#$%^&*()[]{}+=><~?";
	inline char xdata_char_to_ascii(uint8_t xdata_char)
	{
		return XDATA_CHAR_TO_ASCII_TABLE[xdata_char];
	}
	const char ASCII_TO_XDATA_CHAR_TABLE[] = "?????????????????????????????????\x2c?\x2e\x2f\x30\x32?\x34\x35\x33\x3a\x28\x25\x29?\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x2a\x2b\x3d\x3b\x3c\x3f\x2d??????????????????????????\x36?\x37\x31\x24?\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x38\x26\x39\x3e?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????";
	inline char ascii_to_xdata_char(uint8_t ascii_code)
	{
		if (ascii_code >= 'A' && ascii_code <= 'Z')
		{
			ascii_code += ('a'-'A');
		}
		return ASCII_TO_XDATA_CHAR_TABLE[ascii_code];
	}
}
std::string read_pc_part_xdata_resref(const char*& xdata, uint32_t length)
{
	std::string result;
	result.reserve(length);
	int read_count = 0;
	uint8_t pending_char = 0;
	while (result.length() < length)
	{
		uint8_t read_char = read_pc_part_xdata_byte(xdata);
		switch (read_count % 3)
		{
		case 0: 
			result += xdata_char_to_ascii(read_char & 0x3F);
			pending_char = (read_char & 0xC0) >> 6;
			break;
		case 1:
			result += xdata_char_to_ascii(read_char & 0x3F);
			pending_char |=  (read_char & 0xC0) >> 4;
			break;
		case 2:
			result += xdata_char_to_ascii(read_char & 0x3F);
			if (result.length() < length)
			{
				pending_char |=  (read_char & 0xC0) >> 2;
				result += xdata_char_to_ascii(pending_char);
			}
			break;
		}
		read_count++;
	}
	return result;
}
std::string get_pc_part_xdata_resref(const std::string& value)
{
	const char* value_cstr = value.c_str();
	std::string result;
	uint32_t char_index = 0;
	uint8_t next_free_char = 0xFF;
	while (*value_cstr)
	{
		uint8_t mask_pos = char_index % 3;
		if (mask_pos == 0)
		{
			if (next_free_char != 0xFF)
			{
				value_cstr++;
				if (!*value_cstr) break;
			}
			if (*(value_cstr+1)&&*(value_cstr+2)&&*(value_cstr+3))
			{
				next_free_char = ascii_to_xdata_char(*(value_cstr+3));
			}
			else
			{
				next_free_char = 0;
			}
		}
		result += get_pc_part_xdata_byte(ascii_to_xdata_char(*value_cstr) | ((next_free_char & (3<<(mask_pos*2))) << ((3-mask_pos)*2)));
		char_index++;
		value_cstr++;
	}
	return result;
}
int read_pc_part_xdata_flags(const char*& xdata)
{
	int result = read_pc_part_xdata_byte(xdata);
	if (result & (1<<7))
	{
		result &= ~(1<<7);
		result |= (read_pc_part_xdata_byte(xdata) << 8);
	}
	return result;
}
uint16_t read_pc_part_xdata_colors(const char*& xdata)
{
	uint16_t colors = read_pc_part_xdata_byte(xdata);
	if (colors & 0x80)
	{
		colors &= ~0x80;
		uint16_t extra_colors = read_pc_part_xdata_byte(xdata) << 7;
		colors |= extra_colors;
	}
	return colors;
}
char dec_to_hex(uint8_t decimal)
{
	if (decimal < 10) return '0'+decimal;
	if (decimal < 15) return 'A'+(decimal-10);
	return 'F';
}
std::string get_pc_part_xdata_byte(uint8_t value)
{
	char byte_hex[3];
	byte_hex[0] = dec_to_hex(value >> 4);
	byte_hex[1] = dec_to_hex(value & 0x0F);
	byte_hex[2] = 0;
	return byte_hex;
}
std::string get_pc_part_xdata_float(float value)
{
	uint8_t* bytes_val = (uint8_t*)&value;
	char float_hex[9];
	for (int i=0; i<4; i++)
	{
		float_hex[i*2] = dec_to_hex(bytes_val[i] >> 4);
		float_hex[i*2+1] = dec_to_hex(bytes_val[i] & 0x0F);
	}
	float_hex[8] = 0;
	return float_hex;
}
std::string get_pc_part_xdata_flags(int flags)
{
	if (flags > (1<<7))
	{
		flags |= (1<<7);
		std::string result = get_pc_part_xdata_byte(flags & 0xFF);
		result += get_pc_part_xdata_byte(flags >> 8);
		return result;
	}
	else
	{
		return get_pc_part_xdata_byte(flags);
	}
}
std::string get_pc_part_xdata_colors(uint16_t colors)
{
	uint8_t basic_colors = (colors & 0x00FF);
	uint8_t extra_colors = (colors & 0xFF00) >> 7;
	if (colors >= 0x80)
	{
		if (colors & 0x80)
		{
			extra_colors |= 0x1;
		}
		else
		{
			basic_colors |= 0x80;
		}
	}
	std::string colors_str = get_pc_part_xdata_byte(basic_colors);
	if (extra_colors)
	{
		colors_str += get_pc_part_xdata_byte(extra_colors);
	}
	return colors_str;
}

void PC_PART_XDATA::delete_pointers()
{
	if (mapped_textures != NULL)
	{
		delete mapped_textures;
	}
}
void PC_PART_XDATA::clear()
{
	delete_pointers();
	memset(channels_mappping, 0xFF, sizeof(channels_mappping));
	memset(channels_lightness_mod, 0, sizeof(channels_lightness_mod));
	memset(channels_color, 0, sizeof(channels_color));
	memset(rgba_mod, 0, sizeof(RGBA_MOD));
	channels_color_override = 0;
	scaling = 1.0;
	extra_x = 0.0;
	extra_y = 0.0;
	extra_z = 0.0;
	extra_rotate_x = 0.0;
	extra_rotate_y = 0.0;
	extra_rotate_z = 0.0;
	mapped_textures = NULL;
}
void PC_PART_XDATA::copy_data(const PC_PART_XDATA& copy)
{
	memcpy(channels_mappping, copy.channels_mappping, sizeof(channels_mappping));
	memcpy(channels_lightness_mod, copy.channels_lightness_mod, sizeof(channels_lightness_mod));
	memcpy(channels_color, copy.channels_color, sizeof(channels_color));
	memcpy(rgba_mod, copy.rgba_mod, sizeof(RGBA_MOD));
	channels_color_override = copy.channels_color_override;
	scaling = copy.scaling;
	extra_x = copy.extra_x;
	extra_y = copy.extra_y;
	extra_z = copy.extra_z;
	extra_rotate_x = copy.extra_rotate_x;
	extra_rotate_y = copy.extra_rotate_y;
	extra_rotate_z = copy.extra_rotate_z;
	mapped_textures = copy.mapped_textures ? new std::map<std::string, std::string>(*copy.mapped_textures) : NULL;	
}
PC_PART_XDATA::PC_PART_XDATA() : mapped_textures(NULL)
{
	clear();
}
PC_PART_XDATA::PC_PART_XDATA(const PC_PART_XDATA& to_copy)
{
	copy_data(to_copy);	
}
PC_PART_XDATA& PC_PART_XDATA::operator=(const PC_PART_XDATA& other)
{
	if (this != &other)
	{
		delete_pointers();
		copy_data(other);
	}
	return *this;
}
PC_PART_XDATA::~PC_PART_XDATA()
{
	delete_pointers();
}
PC_PART_XDATA pc_part_xdata_empty;
bool PC_PART_XDATA::has_data()
{
	return (memcmp(this, &pc_part_xdata_empty, sizeof(PC_PART_XDATA))!=0);
}
bool PC_PART_XDATA::equals(PC_PART_XDATA& to)
{
	if(
		memcmp(channels_mappping, to.channels_mappping, sizeof(channels_mappping)) ||
		memcmp(channels_lightness_mod, to.channels_lightness_mod, sizeof(channels_lightness_mod)) ||
		memcmp(channels_color, to.channels_color, sizeof(channels_color)) ||
		memcmp(rgba_mod, to.rgba_mod, sizeof(RGBA_MOD)) ||
		channels_color_override != to.channels_color_override ||
		scaling != to.scaling ||
		extra_x != to.extra_x ||
		extra_y != to.extra_y ||
		extra_z != to.extra_z ||
		extra_rotate_x != extra_rotate_x ||
		extra_rotate_y != extra_rotate_y ||
		extra_rotate_z != extra_rotate_z)
	{
		return false;
	}
	if (mapped_textures == NULL)
	{
		if (to.mapped_textures != NULL) return false;
	}
	else
	{
		if (to.mapped_textures == NULL ||
			mapped_textures->size() != to.mapped_textures->size())
		{
			return false;
		}
		for (auto iter=mapped_textures->begin(); iter!=mapped_textures->end(); iter++)
		{
			auto to_iter = to.mapped_textures->find(iter->first);
			if (to_iter == to.mapped_textures->end() ||
				iter->second != to_iter->second)
			{
				return false;
			}
		}
	}
	return true;
}
bool PC_PART_XDATA::has_rgba_mod()
{
	for (uint32_t i=0; i<sizeof(RGBA_MOD); i++)
	{
		if (rgba_mod[i] != 0)
		{
			return true;
		}
	}
	return false;
}
bool PC_PART_XDATA::has_texture_data()
{
	return (memcmp(channels_lightness_mod, pc_part_xdata_empty.channels_lightness_mod, sizeof(channels_lightness_mod)) !=0 ||
			memcmp(channels_mappping, pc_part_xdata_empty.channels_mappping, sizeof(channels_mappping)) != 0 );
}

std::shared_ptr<PC_PART_XDATA::MAPPED_TEXTURES::iterator> PC_PART_XDATA::get_mapped_textures_iterator()
{
	return std::shared_ptr<MAPPED_TEXTURES::iterator>(mapped_textures ? new MAPPED_TEXTURES::iterator(mapped_textures->begin()) : NULL);
}
std::string PC_PART_XDATA::get_mapped_texture_from(std::shared_ptr<MAPPED_TEXTURES::iterator>& iter)
{
	return (iter ? (*iter)->first : "");
}
std::string PC_PART_XDATA::get_mapped_texture_to(std::shared_ptr<MAPPED_TEXTURES::iterator>& iter)
{
	return (iter ? (*iter)->second : "");	
}
bool PC_PART_XDATA::increment_mapped_textures_iterator(std::shared_ptr<MAPPED_TEXTURES::iterator>& iter)
{
	if (iter && mapped_textures)
	{
		(*iter)++;
		return ((*iter) != mapped_textures->end());
	}
	return false;
}
	
void PC_PART_XDATA::map_texture(const std::string& from, const std::string& to)
{
	if (from.empty() || to.empty()) return;
	if (mapped_textures == NULL)
	{
		mapped_textures = new std::map<std::string, std::string>;
	}
	(*mapped_textures)[from] = to;
}
void PC_PART_XDATA::unmap_texture(const std::string& from)
{
	if (mapped_textures != NULL)
	{
		mapped_textures->erase(from);
	}
}
bool PC_PART_XDATA::has_mapped_textures()
{
	return (mapped_textures != NULL && mapped_textures->size() > 0);
}
std::string PC_PART_XDATA::get_mapped_texture(std::string texture)
{
	if (mapped_textures)
	{
		std::transform(texture.begin(), texture.end(), texture.begin(), ::tolower);
		auto iter = mapped_textures->find(texture);
		if (iter != mapped_textures->end())
		{
			return iter->second;	
		}
	}
	return texture;
}

int8_t PC_PART_XDATA::get_rgba_mod(uint8_t rgba)
{
	if (rgba >= sizeof(RGBA_MOD)) return 0;
	return rgba_mod[rgba];
}
void PC_PART_XDATA::set_rgba_mod(uint8_t rgba, int8_t val)
{
	if (rgba >= sizeof(RGBA_MOD)) return;
	rgba_mod[rgba] = val;
}

uint8_t num_xdata_flags(uint8_t flag_index)
{
	uint8_t result = 0;
	if (flag_index >= 15)
	{
		result++;
	}
	if (flag_index >= 7)
	{
		result++;
	}
	return result;
}

void PC_PART_XDATA::read_pc_part_xdata(const char*& xdata)
{
	int pc_part_xdata_flags = read_pc_part_xdata_flags(xdata);
	uint8_t current_map_value = 0xFF;
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORMAP)
	{
		uint16_t colors = read_pc_part_xdata_colors(xdata);
		for (int i=0; i<NUM_PLT_COLOR_CHANNELS; i++)
		{
			if (colors & (1<<i))
			{
				if (current_map_value == 0xFF)
				{
					current_map_value = read_pc_part_xdata_byte(xdata);
					channels_mappping[i] = (current_map_value >> 4);
				}
				else
				{
					channels_mappping[i] = (current_map_value & 0x0F);
					current_map_value = 0xFF;
				}
			}
		}
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_LIGHTMOD)
	{
		uint16_t colors = read_pc_part_xdata_colors(xdata);
		for (int i=0; i<NUM_PLT_COLOR_CHANNELS; i++)
		{
			if (colors & (1<<i))
			{
				channels_lightness_mod[i] = read_pc_part_xdata_byte(xdata);
			}
		}
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORSOVERRIDE)
	{
		channels_color_override = read_pc_part_xdata_colors(xdata);
		for (int i=0; i<NUM_PLT_COLOR_CHANNELS; i++)
		{
			if (channels_color_override & (1<<i))
			{
				channels_color[i] = read_pc_part_xdata_byte(xdata);
			}
		}
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_SCALE)
	{
		scaling = read_pc_part_xdata_float(xdata);
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEX)
	{
		extra_x = read_pc_part_xdata_float(xdata);
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEY)
	{
		extra_y = read_pc_part_xdata_float(xdata);
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEZ)
	{
		extra_z = read_pc_part_xdata_float(xdata);
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEX)
	{
		extra_rotate_x = read_pc_part_xdata_float(xdata);		
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEY)
	{
		extra_rotate_y = read_pc_part_xdata_float(xdata);		
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEZ)
	{
		extra_rotate_z = read_pc_part_xdata_float(xdata);		
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_RGBA_OVERRIDE)
	{
		int rgba_override_flags = read_pc_part_xdata_flags(xdata);
		for (uint32_t i=0; i<sizeof(RGBA_MOD); i++)
		{
			if (rgba_override_flags & (1<<(i+num_xdata_flags(i)))) 
			{
				rgba_mod[i] = read_pc_part_xdata_byte(xdata);
			}
		}
	}
	if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_TEXTURE_MAPPING)
	{
		uint8_t lengths = read_pc_part_xdata_byte(xdata);
		while (lengths)
		{
			uint8_t from_length = (lengths & 0xF) + 1;
			uint8_t to_length = ((lengths & 0xF0) >> 4) + 1;
			std::string mapped_texture = read_pc_part_xdata_resref(xdata, from_length + to_length);
			map_texture(mapped_texture.substr(0, from_length), mapped_texture.substr(from_length));
			lengths = read_pc_part_xdata_byte(xdata);
		}
	}
}

std::string PC_PART_XDATA::get_pc_part_xdata()
{
	int pc_part_xdata_flags = 0;
	uint16_t colors_map_flag = 0;
	uint16_t colors_light_flag = 0;
	std::string colors_map_data;
	std::string colors_light_data;
	std::string channels_colors_override_data;
	uint8_t current_map_value = 0xFF;
	for (int i=0; i<NUM_PLT_COLOR_CHANNELS; i++)
	{
		if (channels_mappping[i]!=0xFF)
		{
			pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORMAP;
			colors_map_flag |= (1<<i);
			if (current_map_value == 0xFF)
			{
				current_map_value = channels_mappping[i] << 4;
			}
			else
			{
				current_map_value |= channels_mappping[i];
				colors_map_data += get_pc_part_xdata_byte(current_map_value);
				current_map_value = 0xFF;
			}
		}
	}
	if (current_map_value != 0xFF)
	{
		colors_map_data += get_pc_part_xdata_byte(current_map_value);
	}
	for (int i=0; i<NUM_PLT_COLOR_CHANNELS; i++)
	{
		if (channels_lightness_mod[i]!=0)
		{
			pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_LIGHTMOD;
			colors_light_flag |= (1<<i);
			colors_light_data += get_pc_part_xdata_byte(channels_lightness_mod[i]);
		}
	}
	if (channels_color_override)
	{
		channels_colors_override_data = get_pc_part_xdata_colors(channels_color_override);
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_COLORSOVERRIDE;
		for (int i=0; i<NUM_PLT_COLOR_CHANNELS; i++)
		{
			if (channels_color_override & (1<<i))
			{
				channels_colors_override_data += get_pc_part_xdata_byte(channels_color[i]);
			}
		}
	}
	if (scaling != 1.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_SCALE;
	}
	if (extra_x != 0.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEX;
	}
	if (extra_y != 0.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEY;
	}
	if (extra_z != 0.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEZ;
	}
	if (extra_rotate_x != 0.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEX;				
	}
	if (extra_rotate_y != 0.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEY;				
	}
	if (extra_rotate_z != 0.0)
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEZ;				
	}
	if (has_rgba_mod())
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_RGBA_OVERRIDE;
	}
	if (has_mapped_textures())
	{
		pc_part_xdata_flags |= NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_TEXTURE_MAPPING;
	}
	std::string result;
	if (pc_part_xdata_flags)
	{
		result += get_pc_part_xdata_flags(pc_part_xdata_flags);
		if (colors_map_flag)
		{
			result += get_pc_part_xdata_colors(colors_map_flag);
			result += colors_map_data;
		}
		if (colors_light_flag)
		{
			result += get_pc_part_xdata_colors(colors_light_flag);
			result += colors_light_data;
		}
		if (channels_color_override)
		{
			result += channels_colors_override_data;
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_SCALE)
		{
			result += get_pc_part_xdata_float(scaling);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEX)
		{
			result += get_pc_part_xdata_float(extra_x);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEY)
		{
			result += get_pc_part_xdata_float(extra_y);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_MOVEZ)
		{
			result += get_pc_part_xdata_float(extra_z);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEX)
		{
			result += get_pc_part_xdata_float(extra_rotate_x);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEY)
		{
			result += get_pc_part_xdata_float(extra_rotate_y);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_XDATA_ROTATEZ)
		{
			result += get_pc_part_xdata_float(extra_rotate_z);
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_RGBA_OVERRIDE)
		{
			int rgba_override_flags = 0;
			std::string rgba_override_data;
			for (uint32_t i=0; i<sizeof(RGBA_MOD); i++)
			{
				if (rgba_mod[i] != 0)
				{
					rgba_override_flags |= (1<<(i+num_xdata_flags(i)));
					rgba_override_data += get_pc_part_xdata_byte(rgba_mod[i]);
				}
			}
			result += get_pc_part_xdata_flags(rgba_override_flags);
			result += rgba_override_data;
		}
		if (pc_part_xdata_flags & NWNCX_SINFAR_CREATURE_EXTRA_ARMOR_TEXTURE_MAPPING)
		{
			for (auto iter=mapped_textures->begin(); iter!=mapped_textures->end(); iter++)
			{
				result += get_pc_part_xdata_byte((iter->first.length()-1) | ((iter->second.length()-1) << 4));
				result += get_pc_part_xdata_resref(iter->first + iter->second);
			}
			result += get_pc_part_xdata_byte(0);
		}
	}
	return result;
}
