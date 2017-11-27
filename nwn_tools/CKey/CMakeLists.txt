file(GLOB  ckey_source_files "*.cpp" "*.h")
include_directories(".")

file(GLOB  test_source_files "test/*.cpp" "test/*.h")
add_executable(CKey ${test_source_files} ${ckey_source_files})

file(GLOB  extract_key_source_files "extract_key_src/*.cpp" "extract_key_src/*.h")
add_executable(extract_key ${extract_key_source_files} ${ckey_source_files})