#include "AdbDevice/AdbDeviceSerial.h"
#include "AdbDevice/VBoxConfigDiscovery.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace
{
class Fixture
{
public:
    Fixture()
    {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path() / ("maa-toolkit-vbox-" + std::to_string(unique));
        std::filesystem::create_directories(directory_);
    }

    ~Fixture()
    {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    std::filesystem::path write(std::string_view xml, const std::filesystem::path& name = "instance.vbox") const
    {
        auto path = directory_ / name;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);
        file << xml;
        if (!file) {
            throw std::runtime_error("Could not write test fixture");
        }
        return path;
    }

    std::filesystem::path missing() const { return directory_ / "missing.vbox"; }

private:
    std::filesystem::path directory_;
};

void require_serials(const std::filesystem::path& file, const std::vector<std::string>& expected)
{
    const auto actual = MaaNS::ToolkitNS::read_vbox_adb_serials(file);
    if (actual != expected) {
        std::cerr << "Expected:";
        for (const auto& serial : expected) {
            std::cerr << ' ' << serial;
        }
        std::cerr << "\nActual:";
        for (const auto& serial : actual) {
            std::cerr << ' ' << serial;
        }
        throw std::runtime_error("Unexpected VBox ADB endpoints");
    }
}

std::string configuration(std::string_view adapters, std::string_view extra = "")
{
    return "<VirtualBox xmlns='http://www.virtualbox.org/' version='1.16-windows'><Machine name='test'>"
           "<Hardware><Network>"
           + std::string(adapters) + "</Network></Hardware>" + std::string(extra) + "</Machine></VirtualBox>";
}

std::string adapter(std::string_view rules)
{
    return "<Adapter slot='0' enabled='true'><NAT>" + std::string(rules) + "</NAT></Adapter>";
}

void formatting(const Fixture& fixture)
{
    const auto xml = configuration(adapter(R"(
        <Forwarding guestport = '5555' name='adb &amp; debug' hostport = '62026'
                    guestip='10.0.2.15' hostip='127.0.0.1' proto='1' />
        <Forwarding proto="1" hostport="62027" guestport="5555"></Forwarding>
    )"));
    require_serials(fixture.write(xml), { "127.0.0.1:62026", "127.0.0.1:62027" });
}

void namespaces(const Fixture& fixture)
{
    require_serials(
        fixture.write(R"(<?xml version="1.0"?>
        <v:VirtualBox xmlns:v="http://www.virtualbox.org/" version="1.16-windows">
          <v:Machine><v:Hardware><v:Network><v:Adapter enabled="1"><v:NAT>
            <!-- Snapshot and comment text are not current forwarding rules. -->
            <v:Forwarding hostport="21503" proto="1" guestport="5555"/>
          </v:NAT></v:Adapter></v:Network></v:Hardware></v:Machine>
        </v:VirtualBox>)"),
        { "127.0.0.1:21503" });
}

void filtering(const Fixture& fixture)
{
    const auto xml = configuration(adapter(R"(
          <Forwarding proto='0' hostport='1' guestport='5555'/>
          <Forwarding proto='tcp' hostport='2' guestport='5555'/>
          <Forwarding proto='1' hostport='3' guestport='5554'/>
          <Forwarding proto='1' hostport='0' guestport='5555'/>
          <Forwarding proto='1' hostport='65536' guestport='5555'/>
          <Forwarding proto='1' hostport='99999999999999999999' guestport='5555'/>
          <Forwarding proto='1' hostport='-10' guestport='5555'/>
          <Forwarding proto='1' hostport='+10' guestport='5555'/>
          <Forwarding proto='1' hostport='12x' guestport='5555'/>
          <Forwarding proto='1' hostport=' 12' guestport='5555'/>
          <Forwarding proto='1' guestport='5555'/>
          <Forwarding hostport='13' guestport='5555'/>
          <Forwarding proto='1' hostport='14'/>
          <Forwarding proto='1' hostport='1' guestport='5555'/>
          <Forwarding proto='1' hostport='65535' guestport='5555'/>
        )") + R"(<Adapter enabled='false'><NAT><Forwarding proto='1' hostport='20' guestport='5555'/></NAT></Adapter>
             <Adapter><NAT><Forwarding proto='1' hostport='21' guestport='5555'/></NAT></Adapter>
             <Adapter enabled='true'><DisabledModes><NAT><Forwarding proto='1' hostport='22' guestport='5555'/></NAT></DisabledModes>
               <BridgedInterface name='eth0'/></Adapter>)");
    require_serials(fixture.write(xml), { "127.0.0.1:1", "127.0.0.1:65535" });
}

void addresses(const Fixture& fixture)
{
    require_serials(
        fixture.write(configuration(adapter(R"(
          <Forwarding proto='1' hostport='62001' guestport='5555' hostip='0.0.0.0'/>
          <Forwarding proto='1' hostport='62002' guestport='5555' hostip='192.168.56.1'/>
          <Forwarding proto='1' hostport='62003' guestport='5555' hostip='::'/>
          <Forwarding proto='1' hostport='62004' guestport='5555' hostip='2001:db8::1'/>
          <Forwarding proto='1' hostport='62005' guestport='5555' hostip='localhost'/>
          <Forwarding proto='1' hostport='62006' guestport='5555' hostip='example.com'/>
          <Forwarding proto='1' hostport='62007' guestport='5555' hostip='127.0.0.999'/>
        )"))),
        { "127.0.0.1:62001", "192.168.56.1:62002", "[::1]:62003", "[2001:db8::1]:62004" });
}

void bracketed_addresses(const Fixture& fixture)
{
    require_serials(
        fixture.write(configuration(adapter(R"(
          <Forwarding proto='1' hostport='62001' guestport='5555' hostip='[::1]'/>
          <Forwarding proto='1' hostport='62001' guestport='5555' hostip='::1'/>
          <Forwarding proto='1' hostport='62002' guestport='5555' hostip='[2001:db8::1]'/>
          <Forwarding proto='1' hostport='62003' guestport='5555' hostip='[::]'/>
          <Forwarding proto='1' hostport='62004' guestport='5555' hostip='[127.0.0.1]'/>
          <Forwarding proto='1' hostport='62005' guestport='5555' hostip='[::1'/>
          <Forwarding proto='1' hostport='62006' guestport='5555' hostip='::1]'/>
          <Forwarding proto='1' hostport='62007' guestport='5555' hostip='[[::1]]'/>
          <Forwarding proto='1' hostport='62008' guestport='5555' hostip='[]'/>
          <Forwarding proto='1' hostport='62009' guestport='5555' hostip='[::1]:5555'/>
        )"))),
        { "[::1]:62001", "[2001:db8::1]:62002", "[::1]:62003" });
}

void snapshots(const Fixture& fixture)
{
    const auto previous = "<Snapshot name='old'><Hardware><Network>" + adapter("<Forwarding proto='1' hostport='60000' guestport='5555'/>")
                          + "</Network></Hardware></Snapshot>";
    require_serials(
        fixture.write(configuration(adapter("<Forwarding proto='1' hostport='62001' guestport='5555'/>"), previous)),
        { "127.0.0.1:62001" });
    require_serials(fixture.write(configuration("", previous)), { });
}

void deduplication(const Fixture& fixture)
{
    require_serials(
        fixture.write(configuration(
            adapter(
                "<Forwarding proto='1' hostport='62001' guestport='5555'/>"
                "<Forwarding proto='1' hostport='62001' guestport='5555' hostip='0.0.0.0'/>")
            + adapter("<Forwarding proto='1' hostport='62001' guestport='5555' hostip='127.0.0.1'/>")
            + adapter("<Forwarding proto='1' hostport='62002' guestport='5555'/>"))),
        { "127.0.0.1:62001", "127.0.0.1:62002" });
}

void invalid_xml(const Fixture& fixture)
{
    require_serials(fixture.write("<VirtualBox><Machine><Hardware></Machine>"), { });
    require_serials(fixture.write(""), { });
    require_serials(fixture.write("not XML"), { });
    require_serials(fixture.write("<VirtualBox/>"), { });
    require_serials(fixture.write(configuration(adapter("<Forwarding hostport='62001' proto='1' guestport='5555>"))), { });
    const auto valid = configuration(adapter("<Forwarding proto='1' hostport='62001' guestport='5555'/>"));
    auto mismatched = valid;
    mismatched.replace(mismatched.find("</NAT>"), 6, "</notNAT>");
    require_serials(fixture.write(mismatched), { });
    require_serials(fixture.write(valid + "<Other/>"), { });
    require_serials(fixture.write(valid + std::string(1, '\0') + "trailing garbage"), { });
}

void missing_file(const Fixture& fixture)
{
    require_serials(fixture.missing(), { });
    require_serials(fixture.missing().parent_path(), { });
}

void unicode_path(const Fixture& fixture)
{
    require_serials(
        fixture.write(
            configuration(adapter("<Forwarding proto='1' hostport='62001' guestport='5555'/>")),
            std::filesystem::path(u8"模拟器.memu")),
        { "127.0.0.1:62001" });
}

void oversized_file(const Fixture& fixture)
{
    auto xml = configuration(adapter("<Forwarding proto='1' hostport='62001' guestport='5555'/>"));
    xml.append(4 * 1024 * 1024, ' ');
    require_serials(fixture.write(xml), { });
}

void discovery_layouts(const Fixture& fixture)
{
    const auto xml = configuration(adapter("<Forwarding proto='1' hostport='62026' guestport='5555'/>"));
    const auto nox = fixture.write("", "Nox/bin/Nox.exe");
    fixture.write(xml, "Nox/bin/BignoxVMS/Nox_0/Nox_0.VBOX");
    fixture.write(xml, "Nox/bin/BignoxVMS/root.vbox");
    const auto memu = fixture.write("", "MEmu/MEmu.exe");
    fixture.write(xml, "MEmu/MemuHyperv VMs/MEmu_0/MEmu_0.memu");
    const auto mumu = fixture.write("", "MuMu/nemu/shell/NemuPlayer.exe");
    fixture.write(xml, "MuMu/nemu/shell/vms/nemu-0/nemu-0.nemu");
    fixture.write(xml, "MuMu/nemu/vms/nemu-1/nemu-1.nemu");
    for (const auto& [name, path] :
         std::vector<std::pair<std::string, std::filesystem::path>> { { "Nox", nox }, { "MEmuPlayer", memu }, { "MuMuPlayer v3", mumu } }) {
        if (MaaNS::ToolkitNS::find_vbox_adb_serials(name, path) != std::vector<std::string> { "127.0.0.1:62026" }) {
            throw std::runtime_error("Could not discover instance for " + name);
        }
    }
    if (!MaaNS::ToolkitNS::find_vbox_adb_serials("LDPlayer", nox).empty() || !MaaNS::ToolkitNS::find_vbox_adb_serials("Nox", { }).empty()
        || !MaaNS::ToolkitNS::find_vbox_adb_serials("Nox", fixture.missing()).empty()) {
        throw std::runtime_error("Unsupported or absent installations must be ignored");
    }
}

void discovery_boundaries(const Fixture& fixture)
{
    const auto xml = configuration(adapter("<Forwarding proto='1' hostport='62026' guestport='5555'/>"));
    const auto nox = fixture.write("", "Nox/bin/Nox.exe");
    fixture.write(xml, "Nox/bin/BignoxVMS/Nox_0/deep/nested.vbox");
    fixture.write(xml, "Nox/bin/BignoxVMS/Nox_0/Nox_0.vbox-prev");
    fixture.write(xml, "Nox/bin/BignoxVMS/Nox_0/other.memu");
    fixture.write("invalid XML", "Nox/bin/BignoxVMS/Nox_0/broken.vbox");

    std::error_code error;
    const auto external = fixture.write(xml, "outside/external.vbox");
    const auto root = nox.parent_path() / "BignoxVMS";
    std::filesystem::create_symlink(external, root / "linked.vbox", error);
    error.clear();
    std::filesystem::create_directory_symlink(external.parent_path(), root / "linked-directory", error);
    if (!MaaNS::ToolkitNS::find_vbox_adb_serials("Nox", nox).empty()) {
        throw std::runtime_error("Discovery followed a symlink, backup, wrong extension or nested directory");
    }
    fixture.write(xml, "Nox/bin/BignoxVMS/Nox_1/current.vbox");
    if (MaaNS::ToolkitNS::find_vbox_adb_serials("Nox", nox) != std::vector<std::string> { "127.0.0.1:62026" }) {
        throw std::runtime_error("Malformed neighboring settings must not hide a valid instance");
    }
}

void serial_aliases(const Fixture&)
{
    using MaaNS::ToolkitNS::adb_device_detail::canonical_serial;
    if (canonical_serial("emulator-5554") != "127.0.0.1:5555" || canonical_serial("LOCALHOST:5555") != "127.0.0.1:5555"
        || canonical_serial("::1:5555") != "[::1]:5555" || canonical_serial("[::1]:5555") == canonical_serial("127.0.0.1:5555")
        || canonical_serial("usb-device") != "usb-device" || canonical_serial("emulator-invalid") != "emulator-invalid") {
        throw std::runtime_error("ADB aliases must normalize without conflating IPv4 and IPv6 listeners");
    }
}

void candidate_aliases(const Fixture&)
{
    using MaaNS::ToolkitNS::adb_device_detail::unique_uncovered_serials;
    const std::vector<std::string> candidates { "127.0.0.1:62026", "127.0.0.1:62026", "emulator-5554", "127.0.0.1:5555" };
    if (unique_uncovered_serials(candidates, { }) != std::vector<std::string> { "127.0.0.1:62026", "emulator-5554", "127.0.0.1:5555" }) {
        throw std::runtime_error("Duplicate candidates must be removed while retaining an unregistered emulator's TCP fallback");
    }
    if (unique_uncovered_serials(candidates, { "127.0.0.1:5555" }) != std::vector<std::string> { "127.0.0.1:62026" }) {
        throw std::runtime_error("A connected device must exclude all of its candidate aliases");
    }
}
} // namespace

int main(int argc, char** argv)
{
    const std::vector<std::pair<std::string, void (*)(const Fixture&)>> cases {
        { "formatting", formatting },
        { "namespaces", namespaces },
        { "filtering", filtering },
        { "addresses", addresses },
        { "bracketed_addresses", bracketed_addresses },
        { "snapshots", snapshots },
        { "deduplication", deduplication },
        { "invalid_xml", invalid_xml },
        { "missing_file", missing_file },
        { "unicode_path", unicode_path },
        { "oversized_file", oversized_file },
        { "discovery_layouts", discovery_layouts },
        { "discovery_boundaries", discovery_boundaries },
        { "serial_aliases", serial_aliases },
        { "candidate_aliases", candidate_aliases },
    };
    try {
        for (const auto& [name, test] : cases) {
            if (argc == 1 || argv[1] == name) {
                Fixture fixture;
                test(fixture);
                std::cout << name << " passed\n";
                if (argc > 1) {
                    return 0;
                }
            }
        }
        return argc == 1 ? 0 : 2;
    }
    catch (const std::exception& error) {
        std::cerr << '\n' << error.what() << '\n';
        return 1;
    }
}
