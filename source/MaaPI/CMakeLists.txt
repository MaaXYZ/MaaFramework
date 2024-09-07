file(GLOB_RECURSE maa_pi_src Impl/*.h Impl/*.hpp Impl/*.cpp API/*.cpp)
file(GLOB_RECURSE maa_pi_header ${MAA_PUBLIC_INC}/MaaPi/*)

add_library(MaaPI SHARED ${maa_pi_src} ${maa_pi_header})

target_compile_definitions(MaaPI PRIVATE MAA_PI_EXPORTS)

target_include_directories(MaaPI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(MaaPI MaaFramework MaaToolkit MaaUtils HeaderOnlyLibraries Boost::system)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${maa_pi_src})

add_subdirectory(CLI)
