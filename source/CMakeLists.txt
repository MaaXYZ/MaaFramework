add_subdirectory(MaaUtils)

if (WITH_ADB_CONTROLLER)
    add_subdirectory(MaaAdbControlUnit)
endif(WITH_ADB_CONTROLLER)
if(WITH_THRIFT_CONTROLLER AND NOT MAA_CROSSCOMPILE)
    add_subdirectory(MaaThriftController)
endif(WITH_THRIFT_CONTROLLER AND NOT MAA_CROSSCOMPILE)
if (WITH_DBG_CONTROLLER)
    add_subdirectory(MaaDbgControlUnit)
endif(WITH_DBG_CONTROLLER)

add_subdirectory(MaaFramework)
add_subdirectory(MaaToolKit)

if(WITH_GRPC)
    add_subdirectory(MaaRpc)
endif(WITH_GRPC)

add_subdirectory(binding)
