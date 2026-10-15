#include "ddi/DownloadInternal.h"
#include "remote/Pairing.h"
#include "wifi/PairSetup.h"
#include "wifi/PairVerify.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main()
{
    using namespace maa::ios;
    try {
        wifi::PairRecord record;
        record.udid = "synthetic-device";
        record.host_identifier = "synthetic-host";
        record.host_private_key.assign(32, 1);
        record.host_public_key.assign(32, 2);
        record.peer_identifier = { 3 };
        record.peer_public_key.assign(32, 4);
        remote::UsbPairingOptions options;
        options.udid = record.udid;
        options.existing_records = { record };
        remote::detail::PairingWorkflowOperations operations;
        operations.verify = [](const auto&, std::string&) {
            wifi::PairVerifyResult result;
            result.outcome = wifi::VerifyOutcome::Paired;
            return result;
        };
        int setups = 0;
        operations.setup = [&](auto host, auto, auto udid, std::string&) {
            ++setups;
            wifi::PairSetupResult result;
            result.ok = true;
            result.record = record;
            result.record.udid = udid;
            result.record.host_identifier = host;
            return result;
        };
        auto paired = remote::detail::run_usb_pairing_workflow(record.udid, "test-host", options, operations);
        require(paired.ok && paired.reused && paired.record && setups == 0, "reuse supplied credential after verification");
        options.existing_records.clear();
        paired = remote::detail::run_usb_pairing_workflow(record.udid, "test-host", options, operations);
        require(paired.ok && !paired.reused && paired.record && setups == 1, "new credential returned to caller");
        std::string error;
        const auto exported = wifi::format_record(*paired.record);
        auto imported = wifi::parse_record(exported, error);
        require(imported && imported->host_private_key == paired.record->host_private_key, "opaque credential export/import");
        operations.verify = [](const auto&, std::string&) {
            return wifi::PairVerifyResult { };
        };
        paired = remote::detail::run_usb_pairing_workflow(record.udid, "test-host", options, operations);
        require(!paired.ok && !paired.record, "verification failure never returns credentials");

        ddi::detail::Catalog catalog { "16.0",
                                       "https://example.invalid/ddi",
                                       { { "DeveloperDiskImage.dmg", 1, "", "c1b0730e0133447badcfd47fd144e254807b06e1" },
                                         { "DeveloperDiskImage.dmg.signature", 1, "", "e25f1814e51579d5f55c0f1fe0135ddb28a47f4a" } },
                                       ddi::Kind::Classic };
        ddi::DownloadOptions download;
        auto result = ddi::detail::download(
            download,
            catalog,
            [](const std::string& url, auto, auto, const auto&, const auto& sink, std::string&) {
                const char value = url.ends_with(".signature") ? 'y' : 'x';
                return sink(&value, 1);
            },
            error);
        require(result.has_value(), error.c_str());
        const auto temporary_root = result->directory.parent_path();
        auto copy = *result;
        result.reset();
        require(std::filesystem::exists(copy.directory), "temporary DDI remains alive while an owner exists");
        copy.temporary_storage.reset();
        require(!std::filesystem::exists(temporary_root), "temporary DDI removed with its last owner");
        std::cout << "caller-owned credentials and temporary DDI lifetime passed\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
