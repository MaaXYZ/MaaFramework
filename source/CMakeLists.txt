file(GLOB_RECURSE maa_src *.cpp *.h *.hpp ../include/*.h)
file(GLOB_RECURSE maa_modules_src modules/*.cpp modules/*.h modules/*.hpp)
list(REMOVE_ITEM maa_src ${maa_modules_src})

add_subdirectory(modules/MaaUtils)
add_subdirectory(modules/MaaControlUnit)
add_subdirectory(modules/MaaThriftController)

add_library(MaaFramework SHARED ${maa_src})

file(GLOB MaaFramework_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h)
set(MaaFramework_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_include_directories(MaaFramework PUBLIC ../include PRIVATE . modules/include ${MaaThriftController_INCLUDE_DIRS})
set_target_properties(MaaFramework PROPERTIES
    PUBLIC_HEADER "${MaaFramework_PUBLIC_HEADERS}"
)
target_compile_definitions(MaaFramework PRIVATE MAA_DLL_EXPORTS)

if(APPLE)
  set_target_properties(MaaFramework PROPERTIES
    INSTALL_RPATH "@loader_path;${MAC_CXX_EXPERIMENTAL_PATH}")
elseif(UNIX)
  set_target_properties(MaaFramework PROPERTIES
    INSTALL_RPATH "$ORIGIN")
endif()

target_link_libraries(MaaFramework MaaControlUnit MaaUtils)
target_link_libraries(MaaFramework ${OpenCV_LIBS} MaaDerpLearning ONNXRuntime::ONNXRuntime HeaderOnlyLibraries MaaThriftController) #asio::asio cpr::cpr

# clang 15之后有ranges
# if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
#     find_package(range-v3 REQUIRED)
#     target_link_libraries(MaaFramework range-v3::range-v3)
# endif ()

install(TARGETS MaaFramework
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    ARCHIVE DESTINATION lib
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${maa_src})
