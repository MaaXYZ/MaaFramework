file(GLOB_RECURSE MaaUtils *.h *.hpp *.cpp)

add_library(MaaUtils SHARED ${MaaUtilsSrc})
set_target_properties(MaaUtils PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(MaaUtils
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../..
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
# target_link_libraries(MaaUtils HeaderOnlyLibraries ${OpenCV_LIBS})
target_compile_definitions(MaaUtils PRIVATE MAA_UTILS_EXPORTS)
