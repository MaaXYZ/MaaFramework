#include <MaaControlUnit/MaaControlUnitAPI.h>

using namespace MAA_CTRL_UNIT_NS;

class ScrollProbe final : public ScrollableUnit
{
public:
    bool scroll(int dx, int dy) override { return dx == 3 && dy == -2; }
};

int main()
{
    ScrollProbe probe;
    MaaCustomControllerCallbacks callbacks { };
    callbacks.scroll = [](int32_t dx, int32_t dy, void* arg) -> MaaBool {
        return static_cast<ScrollProbe*>(arg)->scroll(dx, dy);
    };
    auto* handle = MaaCustomControlUnitCreate(&callbacks, &probe);
    MaaControlUnitHandle base = handle;
    auto* scrollable = dynamic_cast<ScrollableUnit*>(base);
    bool ok = scrollable && dynamic_cast<CustomControlUnitAPI*>(base) == handle && scrollable->scroll(3, -2);
    MaaCustomControlUnitDestroy(handle);
    return ok ? 0 : 1;
}
