# 弃用：不能指定为 runtimes 给 COM 互操作程序集使用、缺少 git 链接
string(TIMESTAMP current_year "%Y")

# 启用组件安装
set(CPACK_NUGET_COMPONENT_INSTALL ON)
set(CPACK_GENERATOR "NuGet")

# 设置 NuGet 包的相关参数
set(CPACK_PACKAGE_NAME "Maa.Framework")
set(CPACK_PACKAGE_VERSION "${MAA_HASH_VERSION}")
set(CPACK_NUGET_PACKAGE_DESCRIPTION "A software automation testing framework based on image recognition.")
set(CPACK_NUGET_PACKAGE_AUTHORS "MaaAssistantArknights Team")
set(CPACK_NUGET_PACKAGE_TITLE "MAA Framework")
set(CPACK_NUGET_PACKAGE_LICENSE_EXPRESSION "LGPL-3.0-only")
set(CPACK_NUGET_PACKAGE_ICONURL
    "https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png")
# set(CPACK_NUGET_PACKAGE_ICON "icon.png") set(CPACK_NUGET_PACKAGE_DESCRIPTION_SUMMARY "A software automation testing
# framework based on image recognition.")
set(CPACK_NUGET_PACKAGE_RELEASE_NOTES
    "Release notes are at https://github.com/MaaAssistantArknights/MaaFramework/releases.")
set(CPACK_NUGET_PACKAGE_COPYRIGHT "Copyright 2021-${current_year} © MistEO and Contributors. All rights reserved.")
set(CPACK_NUGET_PACKAGE_TAGS "MAA native C++ computer-vision")

# 包含 CPack 模块
include(CPack)
