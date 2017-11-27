#pragma once

#include <map>
#include <unordered_set>
#include "core.h"

namespace nwnx { namespace creature {
	
	#define PERCEPTION_POINT_STATE_FLAG_LOS  1
	#define PERCEPTION_POINT_STATE_FLAG_USED 128
	struct PERCEPTION_POINT
	{
		float x;
		float y;
	};
	struct PERCEPTION_POINT_CMP
	{
		bool operator() (const PERCEPTION_POINT& p1, const PERCEPTION_POINT& p2) const
		{
			if (p1.x == p2.x)
			{
				return (p1.y < p2.y);
			}
			else
			{
				return (p1.x < p2.x);
			}
		}
	};
	struct CREATURE_EXTRA_ATTACHED
	{
		float attach_x;
		float attach_y;
		float attach_z;
		uint32_t target_id;
		CREATURE_EXTRA_ATTACHED():	
							attach_x(0),
							attach_y(0),
							attach_z(0),
							target_id(OBJECT_INVALID)
		{
			//constructor
		}
	};
	struct CREATURE_EXTRA;
	inline CREATURE_EXTRA* GetCreatureExtra(CNWSCreatureStats* stats)
	{
		return (CREATURE_EXTRA*)stats->cs_age;
	}
	inline CREATURE_EXTRA* GetCreatureExtra(CNWSCreature* creature)
	{
		return (CREATURE_EXTRA*)creature->cre_stats->cs_age;
	}
	struct CREATURE_EXTRA
	{
		timeval lastmove;
		std::vector<int> custom_factions;
		bool bIsEquipingItem;
		uint32_t last_max_hp;
		uint8_t last_con_mod;
		uint32_t last_real_max_hp;
		std::map<PERCEPTION_POINT, unsigned char, PERCEPTION_POINT_CMP> perception_points;
		uint32_t last_circle_kick_real_target;
		CNWSItem* head_as_item;
		CREATURE_EXTRA_ATTACHED* attached_by;
		uint32_t attached_to;
		uint32_t slots_ip;
		signed char extra_ability_bonus[6];
		std::unordered_set<uint16_t> extra_feats;
		std::unordered_map<uint8_t,signed char> extra_skill_bonus;
		int last_dialog_reply_time;
		CREATURE_EXTRA():	bIsEquipingItem(false),
							last_max_hp(100000),
							last_con_mod(0),
							last_real_max_hp(100000),
							last_circle_kick_real_target(OBJECT_INVALID),
							head_as_item(NULL),
							attached_by(NULL),
							attached_to(OBJECT_INVALID),
							slots_ip(0),
							extra_ability_bonus{0,0,0,0,0,0},
							last_dialog_reply_time(0)
		{
			lastmove.tv_sec=0;
			lastmove.tv_usec=0;
		}
		void DetachAttachedBy();
		void DetachAttachedTo();
		~CREATURE_EXTRA();
	};
	
}
}