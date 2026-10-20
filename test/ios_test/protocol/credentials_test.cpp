#include "MaaUtils/Platform.h"
#include "ddi/DownloadInternal.h"
#include "remote/Pairing.h"
#include "wifi/Credentials.h"
#include "wifi/PairSetup.h"
#include "wifi/PairVerify.h"
#include "wifi/Srp.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}

void test_srp_failure_state()
{
    using namespace maa::ios::wifi;
    const Bytes salt(16, 0x11);
    SrpClient client("Pair-Setup", "123456", "a");
    SrpServer server("Pair-Setup", "123456", "B");
    std::string error;
    require(server.initialize(salt, error), "SRP server initializes");
    require(client.process(salt, server.server_public(), error), "SRP client derives its proof");
    const auto public_key = client.client_public();
    const auto proof = client.client_proof();
    require(server.process(public_key, proof, error), "SRP server accepts matching proof");
    require(
        client.session_key().size() == 64 && client.session_key() == server.session_key()
            && client.verify_server_proof(server.server_proof()),
        "SRP peers agree on the key and verify the server proof");
    require(!server.process(public_key, proof, error), "a consumed SRP challenge cannot be reused");
    require(server.session_key().empty() && server.server_proof().empty(), "rejected SRP replay clears published secrets");

    require(!client.process(salt, Bytes { 0 }, error), "zero server public key is rejected");
    require(
        client.client_public().empty() && client.session_key().empty() && client.client_proof().empty()
            && !client.verify_server_proof(Bytes(64, 0)),
        "failed SRP client calculation does not expose previous results");

    require(server.initialize(salt, error) && client.process(salt, server.server_public(), error), "SRP peers can start a fresh challenge");
    auto invalid_proof = client.client_proof();
    invalid_proof[0] ^= 1;
    require(!server.process(client.client_public(), invalid_proof, error), "incorrect SRP proof is rejected");
    require(server.session_key().empty() && server.server_proof().empty(), "proof rejection does not publish SRP secrets");
    require(!server.process(client.client_public(), client.client_proof(), error), "proof rejection consumes the SRP challenge");
    require(server.initialize(salt, error) && !server.initialize({ }, error), "invalid SRP initialization fails after clearing state");
    require(!server.process(client.client_public(), client.client_proof(), error), "failed initialization cannot leave a usable challenge");
}

int main()
{
    using namespace maa::ios;
    try {
        test_srp_failure_state();
        wifi::PairRecord record;
        record.udid = "synthetic-device";
        record.host_identifier = "synthetic-host";
        record.peer_identifier = "synthetic-peer";
        record.host_private_key.assign(32, 0xab);
        record.host_public_key.assign(32, 2);
        record.peer_public_key.assign(32, 4);
        remote::UsbPairingOptions options;
        options.udid = record.udid;
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
        require(paired.ok && paired.record && setups == 1, "new credential returned to caller");
        const auto first_host = paired.record->host_identifier;
        paired = remote::detail::run_usb_pairing_workflow(record.udid, "test-host", options, operations);
        require(
            paired.ok && paired.record && setups == 2 && paired.record->host_identifier != first_host,
            "each pairing creates a new host identity");
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
        const auto fetch = [](const std::string& url, auto, auto, const auto&, const auto& sink, std::string&) {
            const char value = url.ends_with(".signature") ? 'y' : 'x';
            return sink(&value, 1);
        };
        auto result = ddi::detail::download(download, catalog, fetch, error);
        require(result.has_value(), error.c_str());
        const auto temporary_root = result->directory.parent_path();
        auto copy = *result;
        result.reset();
        require(std::filesystem::exists(copy.directory), "temporary DDI remains alive while an owner exists");
        download.cache_root = temporary_root / MAA_NS::path("DDI 缓存 🙂");
        const auto cached = ddi::detail::download(download, catalog, fetch, error);
        require(cached.has_value(), error.c_str());
        const auto reused = ddi::detail::download(download, catalog, fetch, error);
        require(reused && reused->cache_hit && reused->directory == cached->directory, "Unicode cache path can be reused");
        std::filesystem::remove(cached->directory / "DeveloperDiskImage.dmg");
        const auto invalid = ddi::detail::download(download, catalog, fetch, error);
        require(
            !invalid && error.find(MAA_NS::path_to_utf8_string(cached->directory)) != std::string::npos,
            "invalid cache diagnostic contains the UTF-8 path");
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
