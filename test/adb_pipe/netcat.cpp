#include "Manager/ScreencapAgent.h"
#include "Screencap/RawByNetcat.h"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include <opencv2/imgcodecs.hpp>

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 2;
    }
    const std::string_view mode = argv[1];
    if (mode == "--address") {
        std::cout << "10.0.2.2 0x1 0x2 00:00:00:00:00:00 * eth0\n";
        return 0;
    }
    if (mode == "--png") {
        const cv::Mat image(2, 3, CV_8UC3, cv::Scalar(17, 42, 93));
        std::vector<uchar> png;
        if (!cv::imencode(".png", image, png)) {
            return 1;
        }
        std::cout.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
        return 0;
    }
    if (mode != "bind_failure" && mode != "invalid_address" && mode != "loopback" && mode != "fallback") {
        return 2;
    }

    const auto executable = std::filesystem::absolute(argv[0]).string();
    const json::value config = json::object {
        { "command",
          json::object {
              { "NetcatAddress", json::array { "{ADB}", "--address" } },
              { "ScreencapEncode", json::array { "{ADB}", "--png" } },
          } },
    };
    // TEST-NET-1 represents a remote device, not an address on this host.
    std::string serial = "192.0.2.1:5555";
    if (mode == "invalid_address") {
        serial = "invalid:5555";
    }
    else if (mode == "loopback") {
        serial = "127.0.0.1:5555";
    }
    const auto configure = [&](auto& unit) {
        unit.set_replacement({ { "{ADB}", executable }, { "{ADB_SERIAL}", serial } });
        return unit.parse(config);
    };

    if (mode == "fallback") {
        MaaNS::CtrlUnitNs::ScreencapAgent agent(MaaAdbScreencapMethod_RawByNetcat | MaaAdbScreencapMethod_Encode, {});
        if (!configure(agent) || !agent.init()) {
            std::cerr << "failed to fall back from RawByNetcat to Encode\n";
            return 1;
        }
        const auto image = agent.screencap();
        if (!image || image->size() != cv::Size(3, 2) || image->at<cv::Vec3b>(0, 0) != cv::Vec3b(17, 42, 93)) {
            std::cerr << "fallback did not return the expected screenshot\n";
            return 1;
        }
        return 0;
    }

    MaaNS::CtrlUnitNs::ScreencapRawByNetcat unit;
    if (!configure(unit)) {
        std::cerr << "failed to configure RawByNetcat\n";
        return 1;
    }
    if (unit.init() != (mode == "loopback")) {
        std::cerr << "unexpected RawByNetcat initialization result\n";
        return 1;
    }
    if (mode != "loopback" && unit.screencap()) {
        std::cerr << "failed initialization left an available screenshot method\n";
        return 1;
    }
    if (mode != "loopback") {
        MaaNS::CtrlUnitNs::ScreencapAgent agent(MaaAdbScreencapMethod_RawByNetcat, {});
        if (!configure(agent) || agent.init() || agent.screencap()) {
            std::cerr << "unavailable RawByNetcat was not rejected by the agent\n";
            return 1;
        }
    }
    return 0;
}
