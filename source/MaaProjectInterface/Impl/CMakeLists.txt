file(GLOB_RECURSE project_interface_src *.h *.hpp *.cpp)
file(GLOB_RECURSE project_interface_header ${MAA_PRIVATE_INC}/ProjectInterface/*)

add_library(ProjectInterface STATIC ${project_interface_src} ${project_interface_header})

target_include_directories(ProjectInterface
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${MAA_PRIVATE_INC} ${MAA_PUBLIC_INC})

target_link_libraries(ProjectInterface MaaUtils HeaderOnlyLibraries Boost::system)

add_dependencies(ProjectInterface MaaUtils)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${project_interface_src})
