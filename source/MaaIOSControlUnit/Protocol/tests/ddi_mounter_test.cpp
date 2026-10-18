#include "ddi/Mounter.h"

#include <iostream>
#include <stdexcept>

namespace
{
using namespace maa::ios;
using V = plist::Value;

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}

// iOS 11.4 signature, pinned by classic_catalog's Git blob digest.
const std::vector<uint8_t> classic_signature {
    0x72, 0xc0, 0xa3, 0x4f, 0x3d, 0xfb, 0x8e, 0xcf, 0x5c, 0xc6, 0x49, 0xb9, 0x3d, 0x8e, 0x51, 0xe5, 0x49, 0xa6, 0x9e, 0x30, 0xc7, 0x63,
    0x8a, 0xa9, 0xbe, 0xbf, 0x4a, 0xce, 0x84, 0x5f, 0xc5, 0xb6, 0x2d, 0x2e, 0x88, 0x98, 0xa4, 0xee, 0xab, 0x78, 0x7c, 0xd7, 0xff, 0xaa,
    0x7c, 0xea, 0x90, 0x93, 0x14, 0xd9, 0x62, 0x0f, 0xbb, 0xda, 0xf6, 0x5d, 0x9d, 0x7a, 0xfd, 0xac, 0xf0, 0xfe, 0x87, 0x56, 0x31, 0x2c,
    0xb6, 0x96, 0xdf, 0x21, 0x7c, 0x65, 0x06, 0x39, 0xf1, 0xbd, 0x57, 0x58, 0xa1, 0x3b, 0x55, 0xda, 0x36, 0xac, 0x48, 0xd7, 0xfc, 0x53,
    0x9e, 0x90, 0x9e, 0x2b, 0x45, 0x5c, 0x49, 0x54, 0x95, 0xbc, 0x29, 0x3c, 0xc1, 0xfc, 0xef, 0x7e, 0x0d, 0xf5, 0xbe, 0x05, 0x15, 0x61,
    0xb0, 0x9d, 0x65, 0x03, 0x2d, 0x6b, 0xdd, 0x51, 0x32, 0x70, 0xfe, 0x59, 0x42, 0x6e, 0xc8, 0x5e, 0x0c, 0x9e,
};

struct ImageMounter
{
    std::vector<uint8_t> signature = classic_signature;
    bool mounted = true;
    std::vector<std::string> commands;

    ddi::PlistService service()
    {
        ddi::PlistService result;
        result.request = [this](const V& request, auto, std::string&) -> std::optional<V> {
            const auto command = request.find("Command")->as_string_or();
            commands.push_back(command);
            auto reply = V::Dict();
            if (command == "LookupImage") {
                reply.set("ImagePresent", V::Bool(mounted));
                reply.set("ImageSignature", V::OfData(mounted ? signature : std::vector<uint8_t> { }));
            }
            else if (command == "CopyDevices") {
                auto entries = V::Array();
                if (mounted) {
                    auto entry = V::Dict();
                    entry.set("DiskImageType", V::Str("Developer"));
                    entry.set("IsMounted", V::Bool(true));
                    entry.set("MountPath", V::Str("/Developer"));
                    entries.push(std::move(entry));
                }
                reply.set("EntryList", std::move(entries));
            }
            else {
                throw std::runtime_error("Unexpected image-mounter command");
            }
            return reply;
        };
        return result;
    }
};

void test_classic_reuse_without_download()
{
    ImageMounter mock;
    auto service = mock.service();
    ddi::DownloadTarget target;
    target.system_version = "11.4.1";
    ddi::MountOptions options;
    std::string error;
    auto result = ddi::inspect_mounted(service, target, options, error);
    require(
        result && result->state == ddi::MountState::AlreadyMounted && result->signature_matches == true && error.empty(),
        "pinned Classic signature is reusable without cache or download");
    require(mock.commands == std::vector<std::string> { "LookupImage" }, "reuse only queries the mounted signature");

    mock.signature[0] ^= 1;
    result = ddi::inspect_mounted(service, target, options, error);
    require(result && result->signature_matches == false, "different Classic signature cannot be reused");
    mock.signature.clear();
    result = ddi::inspect_mounted(service, target, options, error);
    require(
        result && result->state == ddi::MountState::AlreadyMounted && result->signature_matches == false,
        "mounted state alone does not establish Classic compatibility");
    mock.mounted = false;
    result = ddi::inspect_mounted(service, target, options, error);
    require(
        result && result->state == ddi::MountState::NotMounted && !result->signature_matches,
        "absent Classic image continues to preparation");
    mock.mounted = true;
    target.system_version = "10.0";
    result = ddi::inspect_mounted(service, target, options, error);
    require(!result && !error.empty(), "unsupported Classic catalog cannot establish compatibility");
}
} // namespace

int main()
{
    try {
        test_classic_reuse_without_download();
        std::cout << "Classic DDI mounted-image checks passed\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
