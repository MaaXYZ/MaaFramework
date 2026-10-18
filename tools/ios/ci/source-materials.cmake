# Keep the patched sources and build configuration with binary-cache restores.
if(NOT PORT MATCHES "^(openssl|curl|ffmpeg|pugixml|zlib)$")
    return()
endif()
if(NOT IS_DIRECTORY "${SOURCE_PATH}")
    message(FATAL_ERROR "Missing corresponding source for ${PORT}")
endif()
set(maa_materials "${CURRENT_PACKAGES_DIR}/share/${PORT}/source-materials")
file(MAKE_DIRECTORY "${maa_materials}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar czf "${maa_materials}/source.tar.gz" --format=gnutar .
    WORKING_DIRECTORY "${SOURCE_PATH}" COMMAND_ERROR_IS_FATAL ANY)
file(COPY "${CURRENT_PORT_DIR}/" DESTINATION "${maa_materials}/port")
file(COPY "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}.vcpkg_abi_info.txt" DESTINATION "${maa_materials}")
if(PORT STREQUAL "ffmpeg")
    file(GLOB_RECURSE maa_configs LIST_DIRECTORIES false
        "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-*/config.h"
        "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-*/config_components.h"
        "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-*/config.mak")
    foreach(maa_config IN LISTS maa_configs)
        file(RELATIVE_PATH maa_relative "${CURRENT_BUILDTREES_DIR}" "${maa_config}")
        get_filename_component(maa_parent "${maa_relative}" DIRECTORY)
        file(COPY "${maa_config}" DESTINATION "${maa_materials}/configuration/${maa_parent}")
    endforeach()
endif()
