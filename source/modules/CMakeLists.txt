set(MODULE_PREFIX "share/MaaFramework/modules")

add_library(MaaFrameworkModule)
target_sources(MaaFrameworkModule PUBLIC FILE_SET cppms TYPE CXX_MODULES FILES
    $<BUILD_INTERFACE:MaaFramework.cppm>
    $<BUILD_INTERFACE:MaaToolkit.cppm>
    $<BUILD_INTERFACE:MaaAgentClient.cppm>
    $<BUILD_INTERFACE:MaaAgentServer.cppm>
    $<INSTALL_INTERFACE:${MODULE_PREFIX}/MaaFramework.cppm>
    $<INSTALL_INTERFACE:${MODULE_PREFIX}/MaaToolkit.cppm>
    $<INSTALL_INTERFACE:${MODULE_PREFIX}/MaaAgentClient.cppm>
    $<INSTALL_INTERFACE:${MODULE_PREFIX}/MaaAgentServer.cppm>
)
target_compile_features(MaaFrameworkModule INTERFACE cxx_std_20)
target_link_libraries(MaaFrameworkModule PRIVATE MaaInterface)

install(
    TARGETS MaaFrameworkModule
    EXPORT MaaFrameworkTargets
    FILE_SET cppms DESTINATION ${MODULE_PREFIX})
