#include "core.h"
#include "script_event.h"

namespace nwnx { namespace dmactions {

	unsigned char d_ret_code_pltoserv_dmaction[0x20];
	int (*playertoserver_dmaction_org)(CNWSMessage*, CNWSPlayer*, uint8_t, int) = (int (*)(CNWSMessage*, CNWSPlayer*, uint8_t, int))&d_ret_code_pltoserv_dmaction;
	int playertoserver_dmaction_hook(CNWSMessage* message, CNWSPlayer* player, uint8_t action_type, int p2)
	{
		nwscript::CScriptArray event_params(new nwscript::CScriptArrayData);
		event_params->push_back(action_type);
		switch (action_type)
		{
			case 0x23: //possess
			case 0x29: //possess full control
			case 0x30: //appear
			case 0x31: //disappear
			case 0x2c: //set door locked
			case 0x2d: //Set door unlocked
			case 0x2e: //enter trigger
			case 0x2f: //exit trigger
			case 0x22: //goto
				event_params->push_back(message->get_object(0));
				break;
			case 0x24: //toggle invulnerable
			case 0x32: //toggle immortal
			case 0x25: //rest
			case 0x20: //heal
			case 0x26: //move to limbo
			case 0x21: //kill
			case 0x2b: //toggle ai
				event_params->push_back(message->get_object(p2 ? 4 : 0));
				event_params->push_back(p2 ? message->get_dword(0) : 0);
				break;
			case 0x11: //set difficulty level
				event_params->push_back(message->get_dword(0));
				break;
			case 0xa: //create creatue
			case 0xb: //create item
			case 0xc: //create trap/trigger?
			case 0xd: //create waypoint
			case 0xe: //create encounter
			case 0xf: //create plc
			case 0x10: //create plc with orientation
				{
					CScriptLocation loc;
					loc.loc_area = message->get_object(0);
					loc.loc_position.x = message->get_float(4);
					loc.loc_position.y = message->get_float(8);
					loc.loc_position.z = message->get_float(12);
					uint32_t resref_index = 16;
					if (action_type == 0x10)
					{
						loc.loc_orientation.x = message->get_float(16);
						loc.loc_orientation.y = message->get_float(20);
						loc.loc_orientation.z = message->get_float(24);
						resref_index = 28;
					}
					else
					{
						loc.loc_orientation.x = 0;
						loc.loc_orientation.y = 0;
						loc.loc_orientation.z = 0;
					}
					std::string resref;
					if (action_type == 0xd || action_type == 0xf)
					{
						resref = message->get_string(resref_index);
					}
					else
					{
						resref = message->get_resref(resref_index).to_str();
					}
					event_params->push_back(resref);
					event_params->push_back(loc);
				}
				break;
			case 0x13: // create trap/trigger?
				event_params->push_back(message->get_resref(8).to_str());
				event_params->push_back(message->get_object(0));
				event_params->push_back(message->get_object(4));
				break;
			case 0x50: //jump to point
			case 0x83: //jump all players to point
			case 0x82: //move creature to point
				{
					CScriptLocation loc;
					loc.loc_area = message->get_object(0);
					loc.loc_position.x = message->get_float(4);
					loc.loc_position.y = message->get_float(8);
					loc.loc_position.z = message->get_float(12);
					loc.loc_orientation.x = 0;
					loc.loc_orientation.y = 0;
					loc.loc_orientation.z = 0;
					event_params->push_back(loc);
					if (action_type == 0x82)
					{
						event_params->push_back(message->get_object(p2 ? 20 : 16));
					}
				}
				break;
			case 0x60: //give/take xp
			case 0x61: //give/take level
			case 0x62: //give gold
			case 0x8b: //display info?
			case 0x63: //set faction id
			case 0x12: //open inventory
			case 0x8c: //align to good
			case 0x8d: //align to evil
			case 0x8e: //align to law
			case 0x8f: //align to chaos
				event_params->push_back(message->get_object(4));
				event_params->push_back(message->get_dword(0));
				break;
			case 0x64: //set faction id by name
				event_params->push_back(message->get_object(0));
				event_params->push_back(message->get_string(4));
				break;
			case 0x84: //modify/set stats
				event_params->push_back(message->get_object(9));
				event_params->push_back(message->get_dword(0));
				event_params->push_back(message->get_float(4));
				event_params->push_back(message->get_byte(8));
				break;
			case 0x80: //give item
			case 0x81: //does nothing?
				event_params->push_back(message->get_object(0));
				event_params->push_back(message->get_resref(4).to_str());
				break;
			case 0x85: //get var
				event_params->push_back(message->get_byte(0));
				event_params->push_back(message->get_object(1));
				event_params->push_back(message->get_string(5));
				break;
			case 0x86: //set var
				{
					uint8_t var_type = message->get_byte(0);
					event_params->push_back(var_type);
					event_params->push_back(message->get_object(1));
					std::string var_name = message->get_string(5);
					event_params->push_back(var_name);
					uint32_t var_value_index = 5+1+var_name.length();
					switch (var_type)
					{
						case VARTYPE_INT:
							event_params->push_back(message->get_dword(var_value_index));
							break;
						case VARTYPE_FLOAT:
							event_params->push_back(message->get_float(var_value_index));
							break;
						case VARTYPE_STRING:
							event_params->push_back(message->get_string(var_value_index));
							break;
						case VARTYPE_OBJECT:
							event_params->push_back(message->get_object(var_value_index));
							break;
					}
				}
				break;
			case 0x87: //set time
				event_params->push_back(message->get_dword(0));
				event_params->push_back(message->get_dword(4));
				event_params->push_back(message->get_dword(8));
				event_params->push_back(message->get_dword(12));
				break;
			case 0x89: //set factions reputation
			case 0x8a: //adjust factiosn reputations
				{
					std::string faction1_name = message->get_string(0);
					uint32_t read_index = faction1_name.length()+1;
					std::string faction2_name = message->get_string(read_index);
					read_index += 1 + faction2_name.length();
					event_params->push_back(faction1_name);
					event_params->push_back(faction2_name);
					event_params->push_back(message->get_dword(read_index));
				}
				break;
		}
		
		if (script_event::run("dmact_ev_all", player->pl_pc_oid, event_params))
		{
			return 0;
		}
		else
		{
			return playertoserver_dmaction_org(message, player, action_type, p2);
		}
	}

	void init()
	{
		core::hook_function(0x0081882E4, (long)playertoserver_dmaction_hook, d_ret_code_pltoserv_dmaction, 12);
	}
	REGISTER_INIT(init);
	
}}