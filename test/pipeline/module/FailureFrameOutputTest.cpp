#include "FailureFrameOutputTest.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "Task/FailureFrameOutput.h"

MAA_SUPPRESS_BOOST_WARNINGS_BEGIN
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
MAA_SUPPRESS_BOOST_WARNINGS_END

namespace
{
namespace fs = std::filesystem;
using MAA_NS::FailureFrameOutput;

constexpr std::string_view kFrame { "\x89PNG\r\n\0", 7 };
constexpr std::string_view kManifest = "{\"frames\":[]}";

class ScratchDirectory
{
public:
    ScratchDirectory()
        : path(fs::temp_directory_path() / ("maa-failure-frames-" + boost::uuids::to_string(boost::uuids::random_generator()())))
    {
        if (!fs::create_directory(path)) {
            throw std::runtime_error("test scratch directory already exists");
        }
    }

    ~ScratchDirectory()
    {
        std::error_code error;
        fs::remove_all(path, error);
    }

    const fs::path path;
};

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_failure(auto operation, const char* message)
{
    try {
        operation();
    }
    catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

void write_sentinel(const fs::path& path, std::string_view contents)
{
    std::ofstream file(path, std::ios::binary);
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    file.close();
}

std::string read_file(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    file.exceptions(std::ios::failbit | std::ios::badbit);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

void write_archive(FailureFrameOutput& output)
{
    output.write_frame("0000.png", std::as_bytes(std::span(kFrame)));
    output.write_manifest(kManifest);
}

void require_archive(const fs::path& archive)
{
    require(read_file(archive / "frames/0000.png") == kFrame, "published frame bytes differ");
    require(read_file(archive / "frames.json") == kManifest, "published manifest bytes differ");
}

fs::path staged_archive(const fs::path& partial)
{
#ifdef _WIN32
    return partial;
#else
    return partial / "export";
#endif
}

void test_exclusive_files(const fs::path& on_error)
{
    const auto final = on_error / "exclusive";
    const auto partial = on_error / "exclusive.partial";
    FailureFrameOutput output(final, partial);
    const auto staging = staged_archive(partial);

    const auto png = staging / "frames/0000.png";
    const auto manifest = staging / "frames.json";
    constexpr std::string_view existing_png { "png sentinel\r\n\0", 15 };
    constexpr std::string_view existing_manifest = "{\"fixture\":true}\r\n";
    write_sentinel(png, existing_png);
    write_sentinel(manifest, existing_manifest);

    require_failure([&] { output.write_frame("0000.png", std::as_bytes(std::span(kFrame))); }, "existing frame was accepted");
    require_failure([&] { output.write_manifest(kManifest); }, "existing manifest was accepted");
    require(read_file(png) == existing_png, "existing frame bytes changed");
    require(read_file(manifest) == existing_manifest, "existing manifest bytes changed");
    require(!fs::exists(final), "failed staging files became a published archive");
}

void require_name_collision(const fs::path& final, const fs::path& partial)
{
    try {
        FailureFrameOutput output(final, partial);
    }
    catch (const std::system_error& error) {
        require(error.code() == std::errc::file_exists, "occupied export name did not report a collision");
        return;
    }
    throw std::runtime_error("occupied export name was accepted");
}

void test_occupied_names(const fs::path& on_error)
{
    const auto final = on_error / "occupied";
    const auto partial = on_error / "occupied.partial";
    for (const auto& occupied : { final, partial }) {
        require(fs::create_directory(occupied), "could not occupy an export name");
        require_name_collision(final, partial);
        require(fs::is_empty(occupied), "occupied export directory was changed");
        require(!fs::exists(occupied == final ? partial : final), "collision created an export directory");
        fs::remove(occupied);
    }
}

void test_late_final_collision(const fs::path& on_error)
{
    const auto final = on_error / "collision";
    const auto partial = on_error / "collision.partial";
    FailureFrameOutput output(final, partial);
    write_archive(output);

    // An ordinary POSIX rename can replace an empty destination directory.
    require(fs::create_directory(final), "could not create late destination");
    require_failure([&] { output.publish(); }, "late destination was accepted");
    require(fs::is_empty(final), "late destination was changed");
    require_archive(staged_archive(partial));
}

void test_staging_move(const fs::path& on_error)
{
    const auto final = on_error / "move";
    const auto partial = on_error / "move.partial";
    const auto moved = on_error / "displaced";
    FailureFrameOutput output(final, partial);

#ifdef _WIN32
    std::error_code error;
    fs::rename(partial, moved, error);
    require(error && fs::is_directory(partial) && !fs::exists(moved), "held staging directory could be moved");
#else
    fs::rename(partial, moved);
    require(fs::create_directory(partial), "could not create a substitute staging directory");
    write_sentinel(partial / "keep.txt", "replacement");
#endif

    write_archive(output);
    output.publish();
    require_archive(final);

#ifndef _WIN32
    require(!fs::exists(staged_archive(moved)), "held archive was not moved to the final name");
    require(read_file(partial / "keep.txt") == "replacement", "substitute staging directory was changed");
    require(!fs::exists(staged_archive(partial)), "writes appeared in the substitute staging directory");
#endif
}

void test_parent_move(const fs::path& scratch)
{
    const auto log_root = scratch / "parent-move";
    fs::create_directory(log_root);
    const auto on_error = log_root / "on_error";
    const auto moved = log_root / "displaced";
    FailureFrameOutput output(on_error / "archive", on_error / "archive.partial");
#ifdef _WIN32
    std::error_code error;
    fs::rename(on_error, moved, error);
    require(error && fs::is_directory(on_error) && !fs::exists(moved), "held on_error directory could be moved");
    write_archive(output);
    output.publish();
    require_archive(on_error / "archive");
#else
    fs::rename(on_error, moved);
    fs::create_directory(on_error);
    write_sentinel(on_error / "keep.txt", "replacement");
    write_archive(output);
    output.publish();
    require_archive(moved / "archive");
    require(read_file(on_error / "keep.txt") == "replacement", "substitute on_error directory was changed");
    require(!fs::exists(on_error / "archive") && !fs::exists(on_error / "archive.partial"), "export reached the substitute parent");
#endif
}

#ifndef _WIN32
void test_shared_parent_permissions(const fs::path& scratch)
{
    const auto log_root = scratch / "shared-parent";
    fs::create_directory(log_root);
    const auto on_error = log_root / "on_error";
    fs::create_directory(on_error);
    fs::permissions(on_error, fs::perms::all);
    require_failure(
        [&] { FailureFrameOutput output(on_error / "archive", on_error / "archive.partial"); },
        "non-sticky shared parent was accepted");
    require(fs::is_empty(on_error), "unsafe shared parent was changed");

    fs::permissions(on_error, fs::perms::sticky_bit, fs::perm_options::add);
    FailureFrameOutput output(on_error / "archive", on_error / "archive.partial");
    write_archive(output);
    output.publish();
    require_archive(on_error / "archive");
}

void test_log_root_symlinks(const fs::path& scratch)
{
    const auto trusted_target = scratch / "trusted-target";
    const auto configured_root = scratch / "configured-root";
    fs::create_directory(trusted_target);
    fs::create_directory_symlink(trusted_target, configured_root);
    const auto trusted = configured_root / "on_error";
    {
        FailureFrameOutput output(trusted / "archive", trusted / "archive.partial");
        write_archive(output);
        output.publish();
    }
    require_archive(trusted_target / "on_error/archive");

    const auto untrusted_root = scratch / "untrusted-root";
    const auto substitute = scratch / "substitute";
    fs::create_directory(untrusted_root);
    fs::create_directory(substitute);
    const auto untrusted = untrusted_root / "on_error";
    fs::create_directory_symlink(substitute, untrusted);
    write_sentinel(substitute / "keep.txt", "replacement");
    require_failure(
        [&] { FailureFrameOutput output(untrusted / "archive", untrusted / "archive.partial"); },
        "on_error symlink was accepted");
    require(
        !fs::exists(substitute / "archive") && !fs::exists(substitute / "archive.partial"),
        "export reached the on_error symlink target");

    const auto final_link = trusted / "link";
    const auto partial = trusted / "link.partial";
    fs::create_directory_symlink(substitute, final_link);
    require_name_collision(final_link, partial);
    require(fs::is_symlink(final_link) && !fs::exists(partial), "collision changed the existing final symlink");
    require(read_file(substitute / "keep.txt") == "replacement", "symlink target was changed");
}
#endif
} // namespace

bool test_failure_frame_output()
{
    try {
        const ScratchDirectory scratch;
        const auto on_error = scratch.path / "on_error";
        test_exclusive_files(on_error);
        test_occupied_names(on_error);
        test_late_final_collision(on_error);
        test_staging_move(on_error);
        test_parent_move(scratch.path);
#ifndef _WIN32
        test_shared_parent_permissions(scratch.path);
        test_log_root_symlinks(scratch.path);
#endif
        std::cout << "Failure frame output: exclusive writes, publication, and staging identity passed\n";
        return true;
    }
    catch (const std::exception& error) {
        std::cerr << "Failure frame output: " << error.what() << '\n';
        return false;
    }
}
