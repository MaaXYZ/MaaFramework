#pragma once
#include <string>
#include <string_view>

namespace maa::ios::wifi
{
/// Windows 内部保存接口：以当前进程用户 SID 的受保护 DACL 创建临时文件，
/// 写入并刷新后在同目录替换目标。temporary 与 path 的同目录关系由调用方保证。
bool write_private_record(
    const std::string& temporary,
    const std::string& path,
    std::string_view text,
    std::string& err,
    bool replace = true);
/// 为指定目录设置受保护 DACL，授予当前进程用户 SID 完全访问及子项继承权限。
/// 仅调整 DACL，不修改所有者；外层 save_record 仅对新建叶子目录调用。
bool protect_record_directory(const std::string& path, std::string& err);
} // namespace maa::ios::wifi
