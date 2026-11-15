cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(MaaToolkitVBoxConfigTests LANGUAGES CXX)
    enable_testing()
endif()

if(NOT TARGET Boost::headers)
    find_package(Boost CONFIG REQUIRED)
endif()

add_executable(MaaToolkitVBoxConfigTest vbox_config_test.cpp)
target_compile_features(MaaToolkitVBoxConfigTest PRIVATE cxx_std_20)
target_include_directories(MaaToolkitVBoxConfigTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../source/MaaToolkit"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/include")
target_link_libraries(MaaToolkitVBoxConfigTest PRIVATE Boost::headers)
if(WIN32)
    target_link_libraries(MaaToolkitVBoxConfigTest PRIVATE ws2_32)
endif()
set_target_properties(MaaToolkitVBoxConfigTest PROPERTIES FOLDER Testing)

foreach(case formatting namespaces filtering addresses bracketed_addresses snapshots deduplication invalid_xml missing_file unicode_path oversized_file discovery_layouts discovery_boundaries serial_aliases candidate_aliases)
    add_test(NAME toolkit_vbox_${case} COMMAND MaaToolkitVBoxConfigTest ${case})
endforeach()
