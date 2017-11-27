#include "core.h"

namespace nwnx { namespace nwncx {

struct MyCLastUpdateObjectExtra
{
	MyCLastUpdateObjectExtra() : name_update(0xFFFFFFFF), xchest(0), xpelvis(0), lhand_xdata("00"), rhand_xdata("00")
	{
	}
	~MyCLastUpdateObjectExtra(){}
	int name_update;
	std::string body_xdata;
	std::string armor_xdata;
	uint16_t xchest;
	uint16_t xpelvis;
	std::string lhand_xdata;
	std::string rhand_xdata;
};
struct MyCLastUpdateObject
{
	CLastUpdateObject nw;
	MyCLastUpdateObjectExtra sf;
};

extern const int latest_nwncx_protocol_version;
extern CNWSPlayer* sending_obj_update_player;
extern uint16_t sending_obj_update_nwncx_version;
extern uint16_t sending_client_area_nwncx_version;
uint16_t get_nwncx_version(CNWSPlayer* player);

}
}
