#include "MaaToolkit/QuickJS/MaaToolkitQuickJS.h"

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#include <quickjs.h> // IWYU pragma: export
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "QuickJS/QuickJSRuntime.h"
#include "Utils/Buffer/BufferTypes.hpp"

void init_module_maa(JSContext* ctx);

void MaaToolkitRunQuickJS(const char* script, MaaStringBuffer* output)
{
    MAA_TOOLKIT_NS::QuickJSRuntime runtime;
    runtime.eval_script(script);
    runtime.exec_loop();
    output->set(runtime.get_result());
}
