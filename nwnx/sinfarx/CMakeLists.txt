file(GLOB  source_files
	"*.cpp" "*.h"
	"./jsoncpp/src/*.cpp" "jsoncpp/src/*.h" "jsoncpp/include/json/*.h"
	"../../nwn_tools/C2da/*.cpp" "../../nwn_tools/C2da/*.h"
	"../../nwn_tools/GFF/*.cpp" "../../nwn_tools/GFF/*.h"
	"../../nwn_tools/CKey/*.cpp" "../../nwn_tools/CKey/*.h"
	"./nwscript_funcs/*.cpp" "./nwscript_funcs/*.h")
include_directories("../../nwn_tools/C2da")
include_directories("jsoncpp/include")
add_library(sinfarx SHARED ${common_source_files} ${source_files})
set_target_properties(sinfarx PROPERTIES COMPILE_DEFINITIONS "LAG_TEST=1")
target_link_libraries(sinfarx mysqlclient "boost_regex" "cryptopp")
