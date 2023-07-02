file(GLOB_RECURSE MaaControlUnitSrc *.h *.hpp *.cpp)

add_library(MaaControlUnit SHARED ${MaaControlUnitSrc})
set_target_properties(MaaControlUnit PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(MaaControlUnit
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../..
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_link_libraries(MaaControlUnit HeaderOnlyLibraries ${OpenCV_LIBS})
target_compile_definitions(MaaControlUnit PRIVATE MAA_DLL_EXPORTS)
if (MAA_DEBUG_EXPORTS)
    target_compile_definitions(MaaControlUnit PRIVATE MAA_DEBUG_DLL_EXPORTS)
endif ()
