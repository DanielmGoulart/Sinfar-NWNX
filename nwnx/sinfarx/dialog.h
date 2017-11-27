#pragma once

#include "nwscript.h"
#include "creature.h"

namespace nwnx { namespace dialog {
	struct DIALOG_EXTRA: public CNWSDialog
	{
		std::unordered_map<int, std::string> internal_custom_tokens;	
	};
	
	extern DIALOG_EXTRA* sending_exostring_from_dialog;
}
}