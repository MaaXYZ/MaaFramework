#include "wifi/PairRecordWindows.h"
#include "i18n/Translation.h"
#include <aclapi.h>
#include <filesystem>
#include <limits>
#include <sddl.h>
#include <vector>
#include <windows.h>

namespace maa::ios::wifi
{
namespace
{
/// RAII 管理安全描述符的 LocalAlloc 内存，令牌句柄和临时 SID 字符串在初始化时释放。
/// DACL 仅含当前进程用户 SID 的完全访问 ACE，启用保护以阻止继承父目录的 ACE。
struct PrivateDescriptor
{
    PSECURITY_DESCRIPTOR data = nullptr;
    PrivateDescriptor() = default;
    PrivateDescriptor(const PrivateDescriptor&) = delete;
    PrivateDescriptor& operator=(const PrivateDescriptor&) = delete;

    ~PrivateDescriptor()
    {
        if (data) {
            LocalFree(data);
        }
    }

    bool initialize(std::string& err)
    {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return fail(err);
        }
        DWORD size = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        std::vector<unsigned char> buffer(size);
        const BOOL ok = size && GetTokenInformation(token, TokenUser, buffer.data(), size, &size);
        const DWORD code = GetLastError();
        CloseHandle(token);
        if (!ok) {
            return fail(err, code);
        }
        LPWSTR sid = nullptr;
        if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &sid)) {
            return fail(err);
        }
        // P 表示受保护 DACL；OI/CI 允许该 ACE 由文件及子目录继承，FA 为完全访问。
        const std::wstring sddl = std::wstring(L"D:P(A;OICI;FA;;;") + sid + L")";
        LocalFree(sid);
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &data, nullptr)) {
            return fail(err);
        }
        return true;
    }

    static bool fail(std::string& err, DWORD code = GetLastError())
    {
        err = std::string(MAA_IOS_TR("Failed to set record file permissions")) + ": Windows error " + std::to_string(code);
        return false;
    }
};
} // namespace

bool protect_record_directory(const std::string& path, std::string& err)
{
    PrivateDescriptor descriptor;
    if (!descriptor.initialize(err)) {
        return false;
    }
    PACL acl = nullptr;
    BOOL present = FALSE, defaulted = FALSE;
    if (!GetSecurityDescriptorDacl(descriptor.data, &present, &acl, &defaulted) || !present) {
        return PrivateDescriptor::fail(err);
    }
    auto wide = std::filesystem::path(path).wstring();
    const DWORD code = SetNamedSecurityInfoW(
        wide.data(),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        nullptr,
        nullptr,
        acl,
        nullptr);
    if (code != ERROR_SUCCESS) {
        SetLastError(code);
        return PrivateDescriptor::fail(err);
    }
    return true;
}

bool write_private_record(const std::string& temporary, const std::string& path, std::string_view text, std::string& err, bool replace)
{
    if (text.size() > std::numeric_limits<DWORD>::max()) {
        err = MAA_IOS_TR("Incomplete record file write");
        return false;
    }
    PrivateDescriptor descriptor;
    if (!descriptor.initialize(err)) {
        return false;
    }
    // 创建时即应用 DACL，句柄不可继承；CREATE_NEW 避免覆盖已有同名临时文件。
    SECURITY_ATTRIBUTES attributes { sizeof(attributes), descriptor.data, FALSE };
    const auto temp = std::filesystem::path(temporary).wstring();
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, &attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        err = MAA_IOS_TR("Cannot open temporary file ") + temporary + ": Windows error " + std::to_string(GetLastError());
        return false;
    }
    DWORD written = 0;
    const BOOL saved = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) && written == text.size()
                       && FlushFileBuffers(file);
    const BOOL closed = CloseHandle(file);
    if (!saved || !closed) {
        DeleteFileW(temp.c_str());
        err = MAA_IOS_TR("Incomplete record file write");
        return false;
    }
    // 调用方提供同目录临时文件，替换沿用它的安全描述符，不经过跨卷复制。
    // 写入、刷新或替换失败时尝试删除临时文件，保留错误供调用方处理。
    if (!MoveFileExW(
            temp.c_str(),
            std::filesystem::path(path).c_str(),
            (replace ? MOVEFILE_REPLACE_EXISTING : 0) | MOVEFILE_WRITE_THROUGH)) {
        const DWORD code = GetLastError();
        DeleteFileW(temp.c_str());
        err = !replace && (code == ERROR_FILE_EXISTS || code == ERROR_ALREADY_EXISTS)
                  ? MAA_IOS_TR("Pairing record already exists; Wi-Fi pairing will not replace it")
                  : std::string(MAA_IOS_TR("Rename failed")) + ": Windows error " + std::to_string(code);
        return false;
    }
    return true;
}
} // namespace maa::ios::wifi
