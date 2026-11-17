#include "FailureFrameOutput.h"

#ifdef _WIN32

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <system_error>

#include "MaaUtils/SafeWindows.hpp"

#include <sddl.h>
#include <winternl.h>

MAA_NS_BEGIN

namespace
{

[[noreturn]] void throw_win32(const char* operation, DWORD error = GetLastError())
{
    throw std::system_error(static_cast<int>(error), std::system_category(), operation);
}

struct HandleCloser
{
    void operator()(HANDLE handle) const { CloseHandle(handle); }
};

using Handle = std::unique_ptr<void, HandleCloser>;

constexpr DWORD kShareDirectory = FILE_SHARE_READ | FILE_SHARE_WRITE;
constexpr ACCESS_MASK kDirectoryAccess = FILE_TRAVERSE | FILE_READ_ATTRIBUTES;

void check_component(std::wstring_view name)
{
    if (name.empty() || name == L"." || name == L".." || name.find_first_of(L"\\/:") != name.npos || name.find(L'\0') != name.npos
        || name.size() > (std::numeric_limits<USHORT>::max)() / sizeof(wchar_t)) {
        throw_win32("Invalid failure frame output name", ERROR_INVALID_NAME);
    }
}

template <typename Function>
Function nt_function(const char* name)
{
    const auto function = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), name);
    if (!function) {
        throw_win32(name);
    }
    return std::bit_cast<Function>(function);
}

[[noreturn]] void throw_nt_error(const char* operation, NTSTATUS status)
{
    static const auto to_dos_error = nt_function<decltype(&RtlNtStatusToDosError)>("RtlNtStatusToDosError");
    throw_win32(operation, to_dos_error(status));
}

Handle open_relative(
    HANDLE parent,
    std::wstring_view name,
    ACCESS_MASK access,
    ULONG share,
    ULONG disposition,
    ULONG options,
    PSECURITY_DESCRIPTOR security = nullptr)
{
    check_component(name);
    static const auto create_file = nt_function<decltype(&NtCreateFile)>("NtCreateFile");
    const auto length = static_cast<USHORT>(name.size() * sizeof(wchar_t));
    UNICODE_STRING relative_name { length, length, const_cast<wchar_t*>(name.data()) };
    OBJECT_ATTRIBUTES attributes {};
    InitializeObjectAttributes(&attributes, &relative_name, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, parent, security);
    IO_STATUS_BLOCK io_status {};
    HANDLE handle = nullptr;
    const auto status = create_file(
        &handle,
        access | SYNCHRONIZE,
        &attributes,
        &io_status,
        nullptr,
        FILE_ATTRIBUTE_NORMAL,
        share,
        disposition,
        options | FILE_SYNCHRONOUS_IO_NONALERT,
        nullptr,
        0);
    if (status < 0) {
        throw_nt_error("Create failure frame output relative to directory", status);
    }
    return Handle(handle);
}

void check_available_name(HANDLE parent, std::wstring_view name)
{
    try {
        // The final component may be a reparse point. Only open the entry itself.
        open_relative(parent, name, FILE_READ_ATTRIBUTES, kShareDirectory | FILE_SHARE_DELETE, FILE_OPEN, FILE_OPEN_REPARSE_POINT);
    }
    catch (const std::system_error& error) {
        if (error.code().value() == ERROR_FILE_NOT_FOUND) {
            return;
        }
        throw;
    }
    throw std::system_error(std::make_error_code(std::errc::file_exists), "Failure frame directory already exists");
}

Handle open_log_root(const std::filesystem::path& path)
{
    const auto handle =
        CreateFileW(path.c_str(), kDirectoryAccess, kShareDirectory, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw_win32("Open failure frame log root");
    }
    return Handle(handle);
}

Handle create_private_stage(HANDLE parent, const std::wstring& name)
{
    DWORD flags = 0;
    if (!GetVolumeInformationByHandleW(parent, nullptr, 0, nullptr, nullptr, &flags, nullptr, 0)) {
        throw_win32("Query failure frame log volume");
    }
    if ((flags & FILE_PERSISTENT_ACLS) == 0) {
        throw_win32("Failure frame staging requires filesystem ACLs", ERROR_NOT_SUPPORTED);
    }

    HANDLE process_token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &process_token)) {
        throw_win32("Open failure frame writer token");
    }
    const Handle token(process_token);
    alignas(TOKEN_USER) std::array<std::byte, sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE> user_info {};
    DWORD length = 0;
    if (!GetTokenInformation(token.get(), TokenUser, user_info.data(), static_cast<DWORD>(user_info.size()), &length)) {
        throw_win32("Query failure frame writer SID");
    }
    LPWSTR sid_string = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<const TOKEN_USER*>(user_info.data())->User.Sid, &sid_string)) {
        throw_win32("Format failure frame writer SID");
    }
    const std::unique_ptr<wchar_t, decltype(&LocalFree)> sid(sid_string, LocalFree);
    // Grant inheritable access to the writer rather than to the token's default owner group.
    const auto acl = std::wstring(L"O:") + sid.get() + L"D:P(A;OICI;FA;;;" + sid.get() + L")";
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) {
        throw_win32("Build private failure frame directory ACL");
    }
    const std::unique_ptr<void, decltype(&LocalFree)> security(descriptor, LocalFree);
    return open_relative(parent, name, kDirectoryAccess | DELETE, kShareDirectory, FILE_CREATE, FILE_DIRECTORY_FILE, security.get());
}

void write_file(HANDLE parent, std::wstring_view name, std::span<const std::byte> bytes)
{
    auto file = open_relative(parent, name, FILE_WRITE_DATA, 0, FILE_CREATE, FILE_NON_DIRECTORY_FILE);
    while (!bytes.empty()) {
        const auto chunk = static_cast<DWORD>(std::min(bytes.size(), static_cast<size_t>(MAXDWORD)));
        DWORD written = 0;
        if (!WriteFile(file.get(), bytes.data(), chunk, &written, nullptr)) {
            throw_win32("Write failure frame output");
        }
        if (written == 0) {
            throw_win32("Write failure frame output", ERROR_WRITE_FAULT);
        }
        bytes = bytes.subspan(written);
    }
    if (!CloseHandle(file.release())) {
        throw_win32("Close failure frame output");
    }
}

} // namespace

struct FailureFrameOutput::Impl
{
    std::wstring final_name;
    Handle on_error;
    Handle stage;
    Handle frames;
};

FailureFrameOutput::FailureFrameOutput(const std::filesystem::path& directory, const std::filesystem::path& temporary)
    : impl_(std::make_unique<Impl>())
{
    const auto parent = directory.parent_path();
    if (parent.filename() != L"on_error" || temporary.parent_path() != parent) {
        throw_win32("Invalid failure frame staging directory", ERROR_INVALID_NAME);
    }
    impl_->final_name = directory.filename().native();
    const auto log_root = open_log_root(parent.parent_path());
    impl_->on_error =
        open_relative(log_root.get(), parent.filename().native(), kDirectoryAccess, kShareDirectory, FILE_OPEN_IF, FILE_DIRECTORY_FILE);
    const auto staging_name = temporary.filename().native();
    check_available_name(impl_->on_error.get(), impl_->final_name);
    check_available_name(impl_->on_error.get(), staging_name);
    impl_->stage = create_private_stage(impl_->on_error.get(), staging_name);
    impl_->frames = open_relative(impl_->stage.get(), L"frames", kDirectoryAccess, kShareDirectory, FILE_CREATE, FILE_DIRECTORY_FILE);
}

FailureFrameOutput::~FailureFrameOutput() = default;

void FailureFrameOutput::write_frame(const std::filesystem::path& name, std::span<const std::byte> bytes)
{
    write_file(impl_->frames.get(), name.native(), bytes);
}

void FailureFrameOutput::write_manifest(std::string_view data)
{
    write_file(impl_->stage.get(), L"frames.json", std::as_bytes(std::span(data)));
}

void FailureFrameOutput::publish()
{
    // Windows cannot rename a directory while its children are open.
    if (impl_->frames && !CloseHandle(impl_->frames.release())) {
        throw_win32("Close failure frame directory");
    }
    const auto& name = impl_->final_name;
    const auto size = sizeof(FILE_RENAME_INFO) + name.size() * sizeof(wchar_t);
    const auto buffer = std::make_unique<std::byte[]>(size);
    FILE_RENAME_INFO info {};
    info.ReplaceIfExists = FALSE;
    info.RootDirectory = impl_->on_error.get();
    info.FileNameLength = static_cast<DWORD>(name.size() * sizeof(wchar_t));
    std::memcpy(buffer.get(), &info, sizeof(info));
    std::memcpy(buffer.get() + offsetof(FILE_RENAME_INFO, FileName), name.c_str(), (name.size() + 1) * sizeof(wchar_t));
    // Use the native rename API for a destination relative to the retained directory handle.
    using NtSetInformationFileFn = NTSTATUS(NTAPI*)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG);
    constexpr ULONG kFileRenameInformation = 10;
    static const auto set_information = nt_function<NtSetInformationFileFn>("NtSetInformationFile");
    IO_STATUS_BLOCK io_status {};
    const auto status = set_information(impl_->stage.get(), &io_status, buffer.get(), static_cast<ULONG>(size), kFileRenameInformation);
    if (status < 0) {
        throw_nt_error("Publish failure frame directory", status);
    }
}

MAA_NS_END

#endif
