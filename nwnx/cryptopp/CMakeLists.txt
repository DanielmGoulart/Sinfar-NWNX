file(GLOB  source_files "*.cpp" "*.h")
add_library(cryptopp STATIC ${source_files})
#set_target_properties(cryptopp PROPERTIES COMPILE_FLAGS " -fopenmp")