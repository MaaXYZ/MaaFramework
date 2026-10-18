#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include <cstdio>
#include <meojson/json.hpp>
#include <stdexcept>

namespace
{
int cancel_calls = 0;

MaaBool cancelled(void*)
{
    ++cancel_calls;
    return true;
}

MaaBool throwing_cancel(void*)
{
    throw std::runtime_error("caller failure");
}

bool result_is(const MAA_NS::StringBuffer& buffer, bool stopped)
{
    const auto result = json::parse(buffer.get());
    return result && result->get("success", true) == false && result->get("cancelled", !stopped) == stopped
           && !result->get("error", "").empty();
}
}

int main()
{
    MAA_NS::StringBuffer result;
    MAA_NS::StringBuffer credential;
    if (MaaToolkitIOSDeviceDiscover("{}", nullptr, cancelled, nullptr, &result) || cancel_calls != 1 || !result_is(result, true)) {
        return 1;
    }
    cancel_calls = 0;
    if (MaaToolkitIOSDeviceDiscover("{\"timeout_ms\":-1}", nullptr, cancelled, nullptr, &result) || cancel_calls != 0
        || !result_is(result, false)) {
        return 2;
    }
    if (MaaToolkitIOSDeviceCheckConnection("{\"wifi\":\"auto\",\"port\":49152}", nullptr, nullptr, nullptr, &result)
        || !result_is(result, false)) {
        return 3;
    }
    if (MaaToolkitIOSDevicePair("{\"wifi\":\"auto\"}", nullptr, nullptr, nullptr, &result, &credential) || !result_is(result, false)
        || !credential.empty()) {
        return 4;
    }
    if (MaaToolkitIOSDevicePrepare("{\"replace\":1}", nullptr, nullptr, nullptr, &result) || !result_is(result, false)) {
        return 5;
    }
    if (MaaToolkitIOSDeviceDiscover("{\"unexpected\":true}", nullptr, nullptr, nullptr, &result) || !result_is(result, false)) {
        return 6;
    }
    if (MaaToolkitIOSDeviceDiscover("{}", nullptr, throwing_cancel, nullptr, &result) || !result_is(result, true)) {
        return 7;
    }
    if (MaaToolkitIOSDeviceDiscover(nullptr, nullptr, nullptr, nullptr, nullptr)) {
        return 8;
    }
    if (MaaToolkitIOSDeviceDiscover("{", nullptr, nullptr, nullptr, &result) || !result_is(result, false)) {
        return 9;
    }
    std::puts("iOS management C boundary validation and cancellation passed");
    return 0;
}
