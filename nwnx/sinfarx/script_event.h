#include "nwscript.h"
#include <vector>
#include <memory>

namespace nwnx { namespace script_event {
	typedef std::unique_ptr<nwscript::CScriptVarValue> RESULT;
	RESULT run(const std::string& script, uint32_t object_id, const nwscript::CScriptArray& params);
	RESULT run(const std::string& script, uint32_t object_id, const std::initializer_list<nwscript::CScriptVarValue>& params={});
	void set_result(const nwscript::CScriptVarValue& result);
	nwscript::CScriptArray& get_params();
	const nwscript::CScriptVarValue& get_param(uint32_t index);
	uint32_t get_param_count();
	
}
}