#include <boost/test/unit_test.hpp>
#include <time.h>
#include "../../nwncx/nwncx.h"
#include "../../nwnx/sinfarx/xdata.h"

void test_pc_parts_compatibility(PC_PARTS_XDATA pc_parts_xdata, const std::string& expected)
{
	std::string pc_parts_xdata_str = pc_parts_xdata.get_pc_parts_xdata();
	BOOST_CHECK_EQUAL(expected, pc_parts_xdata_str);
	const char* xdata = pc_parts_xdata_str.c_str();
	uint8_t pc_parts_flags = read_pc_part_xdata_byte(xdata);
	PC_PARTS_XDATA pc_parts_xdata_result;
	pc_parts_xdata_result.read_pc_parts_xdata(pc_parts_flags, xdata);
	BOOST_CHECK(pc_parts_xdata.equals(pc_parts_xdata_result));
}

BOOST_AUTO_TEST_CASE(ReadAndWriteCompatibility_ColorMap)
{
	PC_PARTS_XDATA pc_parts_xdata;
	test_pc_parts_compatibility(pc_parts_xdata, "");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_ROBE].channels_mappping[nwncx::NwnPalette_Cloth1] = nwncx::NwnPalette_Hair;
	test_pc_parts_compatibility(pc_parts_xdata, "04011010");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_NECK].channels_mappping[nwncx::NwnPalette_Tattoo1] = nwncx::NwnPalette_Tattoo2;
	test_pc_parts_compatibility(pc_parts_xdata, "1401101001800290");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_NECK].channels_mappping[nwncx::NwnPalette_Metal1] = nwncx::NwnPalette_Metal2;
	test_pc_parts_compatibility(pc_parts_xdata, "1401101001840239");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_NECK].channels_mappping[nwncx::NwnPalette_Cloth2] = nwncx::NwnPalette_Tattoo1;
	test_pc_parts_compatibility(pc_parts_xdata, "1401101001A4023890");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_BELT].channels_mappping[nwncx::NwnPalette_Cloth2] = nwncx::NwnPalette_Tattoo1;
	test_pc_parts_compatibility(pc_parts_xdata, "1C01101001208001A4023890");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_PELVIS].channels_mappping[nwncx::NwnPalette_Leather1] = nwncx::NwnPalette_Skin;
	test_pc_parts_compatibility(pc_parts_xdata, "3C01101001208001A402389001014000");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_TORSO].channels_mappping[nwncx::NwnPalette_Hair] = nwncx::NwnPalette_Tattoo2;
	test_pc_parts_compatibility(pc_parts_xdata, "3C01101001208001A402389003014000010290");
}

BOOST_AUTO_TEST_CASE(ReadAndWriteCompatibility_ColorLightMod)
{
	PC_PARTS_XDATA pc_parts_xdata;

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_TORSO].channels_lightness_mod[nwncx::NwnPalette_Hair] = (uint8_t)0xF8;
	test_pc_parts_compatibility(pc_parts_xdata, "20020202F8");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_TORSO].channels_lightness_mod[nwncx::NwnPalette_Skin] = (uint8_t)0xAB;
	test_pc_parts_compatibility(pc_parts_xdata, "20020203ABF8");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_TORSO].channels_mappping[nwncx::NwnPalette_Skin] = nwncx::NwnPalette_Tattoo1;
	test_pc_parts_compatibility(pc_parts_xdata, "200203018003ABF8");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_PELVIS].channels_lightness_mod[nwncx::NwnPalette_Skin] = nwncx::NwnPalette_Tattoo1;
	test_pc_parts_compatibility(pc_parts_xdata, "200302010803018003ABF8");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_BELT].channels_lightness_mod[nwncx::NwnPalette_Skin] = (uint8_t)0x01;
	test_pc_parts_compatibility(pc_parts_xdata, "280201010302010803018003ABF8");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_BELT].channels_lightness_mod[nwncx::NwnPalette_Leather2] = (uint8_t)0x10;
	test_pc_parts_compatibility(pc_parts_xdata, "2802810101100302010803018003ABF8");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_BELT].channels_lightness_mod[nwncx::NwnPalette_Tattoo1] = (uint8_t)0x10;
	test_pc_parts_compatibility(pc_parts_xdata, "280281030110100302010803018003ABF8");
}

BOOST_AUTO_TEST_CASE(ReadAndWriteCompatibility_ArmsLegs)
{
	PC_PARTS_XDATA pc_parts_xdata;

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_RFOREARM].channels_lightness_mod[nwncx::NwnPalette_Cloth2] = (uint8_t)0xBB;
	test_pc_parts_compatibility(pc_parts_xdata, "40200220BB");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_RFOREARM].channels_mappping[nwncx::NwnPalette_Cloth2] = nwncx::NwnPalette_Tattoo2;
	test_pc_parts_compatibility(pc_parts_xdata, "402003209020BB");

	pc_parts_xdata[ITEM_APPR_ARMOR_MODEL_LSHIN].channels_mappping[nwncx::NwnPalette_Tattoo1] = nwncx::NwnPalette_Hair;
	test_pc_parts_compatibility(pc_parts_xdata, "C02003209020BB0401800210");
}

BOOST_AUTO_TEST_CASE(ReadAndWriteCompatibility_TextureMapping)
{
	PC_PARTS_XDATA pc_parts_xdata;
	pc_parts_xdata[0].map_texture("asdf", "asdf2");
	pc_parts_xdata[0].map_texture("08aagu87125%?&*!", "08aagu87125%?&*!");
	pc_parts_xdata[0].map_texture("123456789", "!&*****b");
	pc_parts_xdata[0].map_texture("aaaaaaaaaa", "aaa____---__a");
	//pc_parts_xdata[0].map_texture("08aagu87125%?&*!", "08aagu87125%?&*!");
	test_pc_parts_compatibility(pc_parts_xdata, "80208010FF1A22004614A21B1CDF3FF2B31A22004614A21B1CDF3FF2B3789BDC5D9F20A1E32CF2F333F301C90000000000000000000064A46465A5242400434052034052031C00");
}

BOOST_AUTO_TEST_CASE(TextureMapping_CaseInsensitive)
{
	PC_PARTS_XDATA pc_parts_xdata1;
	pc_parts_xdata1[0].map_texture("aSdf", "asdF2");
	PC_PARTS_XDATA pc_parts_xdata2;
	pc_parts_xdata2[0].map_texture("asdF", "asdf2");
	BOOST_CHECK_EQUAL(pc_parts_xdata1.get_pc_parts_xdata(), pc_parts_xdata2.get_pc_parts_xdata());
}

BOOST_AUTO_TEST_CASE(ReadAndWriteCompatibility_SuperTest)
{
	srand ((unsigned int)time(NULL));
	for (int i=0; i<1000; i++)
	{
		PC_PARTS_XDATA pc_parts_xdata;
		for (int pc_part_index=0; pc_part_index<pc_parts_xdata.get_num_parts(); pc_part_index++)
		{
			for (int color_index=0; color_index<NUM_PLT_COLOR_CHANNELS; color_index++)
			{
				if (rand() % 2 == 0)
				{
					pc_parts_xdata[pc_part_index].channels_mappping[color_index] = (rand() % NUM_PLT_COLOR_CHANNELS);
				}
				if (rand() % 2 == 0)
				{
					pc_parts_xdata[pc_part_index].channels_lightness_mod[color_index] = (int8_t)(rand() % 255);
				}
				if (rand() % 2 == 0)
				{
					pc_parts_xdata[pc_part_index].channels_color_override |= (1<<color_index);
					pc_parts_xdata[pc_part_index].channels_color[color_index] = (uint8_t)(rand() % 255);
				}
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].scaling = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].extra_x = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].extra_y = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].extra_z = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].extra_rotate_x = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].extra_rotate_y = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				pc_parts_xdata[pc_part_index].extra_rotate_z = ((float)(rand() % 1000)) / ((float)(rand() % 1000));
			}
			if (rand() % 2 == 0)
			{
				for (int i=0; i<sizeof(RGBA_MOD); i++)
				{
					if (rand() % 2 == 0)
					{
						pc_parts_xdata[pc_part_index].rgba_mod[i] = (uint8_t)(rand() % 255);
					}
				}
			}
		}
		std::string pc_parts_xdata_str = pc_parts_xdata.get_pc_parts_xdata();
		const char* xdata = pc_parts_xdata_str.c_str();
		uint8_t pc_parts_flags = read_pc_part_xdata_byte(xdata);
		PC_PARTS_XDATA pc_parts_xdata_result;
		pc_parts_xdata_result.read_pc_parts_xdata(pc_parts_flags, xdata);
		BOOST_CHECK(pc_parts_xdata.equals(pc_parts_xdata_result));
	}
}