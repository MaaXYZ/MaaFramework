#include "FailureFrameOutput.h"

#ifndef _WIN32

#include <algorithm>
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <stdio.h>
#include <sys/acl.h>
#include <sys/mount.h>
#elif defined(__linux__)
#include <linux/fs.h>
#include <sys/syscall.h>
#endif

MAA_NS_BEGIN

namespace
{
constexpr int kDirectoryFlags = O_RDONLY | O_DIRECTORY | O_CLOEXEC;
constexpr char kErrorDirectory[] = "on_error";
constexpr char kExportDirectory[] = "export";
constexpr mode_t kNonOwnerPermissions = S_IRWXG | S_IRWXO;

[[noreturn]] void throw_posix_error(const char* operation, int error = errno)
{
    throw std::system_error(error, std::generic_category(), operation);
}

class FileDescriptor
{
public:
    FileDescriptor(int fd, const char* operation)
        : fd_(fd)
    {
        if (fd_ < 0) {
            throw_posix_error(operation);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    ~FileDescriptor()
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    int get() const { return fd_; }

    void close_checked()
    {
        // A failed close may have released the descriptor. Retrying could close a reused descriptor.
        if (::close(std::exchange(fd_, -1)) < 0) {
            throw_posix_error("close failure frame file");
        }
    }

private:
    int fd_;
};

void check_leaf(const std::filesystem::path& name)
{
    if (name.empty() || name.has_parent_path() || name == "." || name == ".." || name.native().find('\0') != std::string::npos) {
        throw std::invalid_argument("invalid failure frame output name");
    }
}

FileDescriptor open_error_directory(const std::filesystem::path& directory, const std::filesystem::path& temporary)
{
    const auto error_directory = directory.parent_path();
    if (error_directory.filename() != kErrorDirectory || temporary.parent_path() != error_directory) {
        throw std::invalid_argument("failure frame paths must share the on_error directory");
    }
    const auto log_directory = error_directory.parent_path();
    // The configured log directory is the trust anchor and may be a symlink.
    const FileDescriptor log_dir(::open(log_directory.empty() ? "." : log_directory.c_str(), kDirectoryFlags), "open log directory");
    if (::mkdirat(log_dir.get(), kErrorDirectory, 0700) < 0 && errno != EEXIST) {
        throw_posix_error("create on_error directory");
    }
    return FileDescriptor(::openat(log_dir.get(), kErrorDirectory, kDirectoryFlags | O_NOFOLLOW), "open on_error directory");
}

FileDescriptor create_private_directory(int parent, const char* name)
{
    if (::mkdirat(parent, name, 0700) < 0) {
        throw_posix_error("create failure frame directory");
    }
    return FileDescriptor(::openat(parent, name, kDirectoryFlags | O_NOFOLLOW), "open failure frame directory");
}

#if defined(__APPLE__)
bool has_no_extended_acl(int directory)
{
    const acl_t acl = ::acl_get_fd(directory);
    if (acl) {
        ::acl_free(acl);
        return false;
    }
    return errno == ENOENT;
}
#endif

FileDescriptor create_export_directory(int staging)
{
    struct stat status;
    if (::fstat(staging, &status) < 0) {
        throw_posix_error("inspect failure frame staging directory");
    }
    if (status.st_uid != ::geteuid() || (status.st_mode & kNonOwnerPermissions) != 0) {
        throw_posix_error("failure frame staging directory is not private", EACCES);
    }
#if defined(__APPLE__)
    // Inherited ACLs and mounts that ignore ownership can override the POSIX mode.
    struct statfs volume;
    if (::fstatfs(staging, &volume) < 0) {
        throw_posix_error("inspect failure frame staging volume");
    }
    if ((volume.f_flags & MNT_IGNORE_OWNERSHIP) != 0 || !has_no_extended_acl(staging)) {
        throw_posix_error("failure frame staging directory is not private", EACCES);
    }
#endif
    // The wrapper may move in a shared parent. Its private contents are the rename source.
    return create_private_directory(staging, kExportDirectory);
}

void check_staging_parent(int parent)
{
    struct stat status;
    if (::fstat(parent, &status) < 0) {
        throw_posix_error("inspect on_error directory");
    }
    // mkdirat cannot return a handle. Prevent another user replacing its entry before openat.
    if (status.st_uid != ::geteuid() || ((status.st_mode & (S_IWGRP | S_IWOTH)) != 0 && (status.st_mode & S_ISVTX) == 0)) {
        throw_posix_error("on_error directory permits staging replacement", EACCES);
    }
#if defined(__APPLE__)
    if (!has_no_extended_acl(parent)) {
        throw_posix_error("on_error directory has an extended ACL", EACCES);
    }
#endif
}

void write_file(int directory, const char* name, std::span<const std::byte> bytes)
{
    const int flags = O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC;
    FileDescriptor file(::openat(directory, name, flags, 0600), "create failure frame file");
    while (!bytes.empty()) {
        const auto count = std::min(bytes.size(), static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
        const auto written = ::write(file.get(), bytes.data(), count);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw_posix_error("write failure frame file");
        }
        if (written == 0) {
            throw_posix_error("write failure frame file", EIO);
        }
        bytes = bytes.subspan(static_cast<size_t>(written));
    }
    file.close_checked();
}

void publish_directory(int staging, int parent, const char* name)
{
#if defined(__APPLE__)
    const auto result = ::renameatx_np(staging, kExportDirectory, parent, name, RENAME_EXCL);
#elif defined(__linux__) && defined(SYS_renameat2) && defined(RENAME_NOREPLACE)
    // Android exposes the libc renameat2 entry point starting at API 30.
    const auto result = ::syscall(SYS_renameat2, staging, kExportDirectory, parent, name, RENAME_NOREPLACE);
#else
    (void)staging;
    (void)parent;
    (void)name;
    const int result = -1;
    errno = ENOTSUP;
#endif
    if (result < 0) {
        throw_posix_error("publish failure frame directory");
    }
}

void remove_staging_directory(int parent, const char* name, int staging)
{
    // POSIX cannot unlink by directory fd. Only clean up if other users cannot replace the checked entry.
    struct stat parent_status;
    if (::fstat(parent, &parent_status) < 0 || parent_status.st_uid != ::geteuid()
        || ((parent_status.st_mode & (S_IWGRP | S_IWOTH)) != 0 && (parent_status.st_mode & S_ISVTX) == 0)) {
        return;
    }
#if defined(__APPLE__)
    if (!has_no_extended_acl(parent)) {
        return;
    }
#endif
    struct stat entry;
    struct stat opened;
    if (::fstatat(parent, name, &entry, AT_SYMLINK_NOFOLLOW) == 0 && ::fstat(staging, &opened) == 0 && entry.st_dev == opened.st_dev
        && entry.st_ino == opened.st_ino) {
        ::unlinkat(parent, name, AT_REMOVEDIR);
    }
}
} // namespace

struct FailureFrameOutput::Impl
{
    Impl(const std::filesystem::path& directory, const std::filesystem::path& temporary)
        : final_name(directory.filename())
        , temporary_name(temporary.filename())
        , parent(open_error_directory(directory, temporary))
        , staging(create_staging_directory())
        , output(create_export_directory(staging.get()))
        , frames(create_private_directory(output.get(), "frames"))
    {
    }

    FileDescriptor create_staging_directory() const
    {
        check_leaf(final_name);
        check_leaf(temporary_name);
        check_staging_parent(parent.get());
        struct stat existing;
        if (::fstatat(parent.get(), final_name.c_str(), &existing, AT_SYMLINK_NOFOLLOW) == 0) {
            throw_posix_error("failure frame destination already exists", EEXIST);
        }
        if (errno != ENOENT) {
            throw_posix_error("inspect failure frame destination");
        }
        return create_private_directory(parent.get(), temporary_name.c_str());
    }

    std::filesystem::path final_name;
    std::filesystem::path temporary_name;
    FileDescriptor parent;
    FileDescriptor staging;
    FileDescriptor output;
    FileDescriptor frames;
};

FailureFrameOutput::FailureFrameOutput(const std::filesystem::path& directory, const std::filesystem::path& temporary)
    : impl_(std::make_unique<Impl>(directory, temporary))
{
}

FailureFrameOutput::~FailureFrameOutput() = default;

void FailureFrameOutput::write_frame(const std::filesystem::path& name, std::span<const std::byte> bytes)
{
    check_leaf(name);
    write_file(impl_->frames.get(), name.c_str(), bytes);
}

void FailureFrameOutput::write_manifest(std::string_view data)
{
    write_file(impl_->output.get(), "frames.json", std::as_bytes(std::span(data.data(), data.size())));
}

void FailureFrameOutput::publish()
{
    publish_directory(impl_->staging.get(), impl_->parent.get(), impl_->final_name.c_str());
    remove_staging_directory(impl_->parent.get(), impl_->temporary_name.c_str(), impl_->staging.get());
}

MAA_NS_END

#endif
