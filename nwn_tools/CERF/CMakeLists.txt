file(GLOB  cerf_source_files "*.cpp" "*.h")
file(GLOB  ckey_source_files "../CKey/*.cpp" "../CKey/*.h")
include_directories(".")

file(GLOB test_source_files "test/*.cpp" "test/*.h")
add_executable(CERF ${test_source_files} ${cerf_source_files} ${ckey_source_files})