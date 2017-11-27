cmake_minimum_required (VERSION 2.6)
project (nwncx_sinfar)
file(GLOB source_files "nwncx_core/*" "nwncx_sinfar/*" "../nwncx/*" "../nwncx/sinfar/*" "../nwncx/ini/*"
	"../nwn_tools/CKey/*.h" "../nwn_tools/CKey/*.cpp"
	"../nwn_tools/CERF/*.h" "../nwn_tools/CERF/*.cpp"
	"../nwnx/sinfarx/xdata.*")
include_directories("/sinfar/libs/includes")
add_library(nwncx_sinfar SHARED ${source_files})
set_target_properties(nwncx_sinfar PROPERTIES COMPILE_FLAGS "-m32 -O3 -s -fvisibility=hidden -fvisibility-inlines-hidden -std=c++14 -Wno-nonnull -Wno-unused-result -D_LINUX -DXDATA_READ_FROM_MESSAGE")
set_target_properties(nwncx_sinfar PROPERTIES PREFIX "")
set_target_properties(nwncx_sinfar PROPERTIES LINK_FLAGS "-m32 -Wl,--no-undefined -static-libgcc -static-libstdc++ -pthread -ldl")
add_custom_command(TARGET nwncx_sinfar POST_BUILD COMMAND "strip" "-s" "-X" "-x" "nwncx_sinfar.so")

