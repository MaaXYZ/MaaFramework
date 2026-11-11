#include "../../source/MaaFramework/Resource/WeChatOCRABI.h"

#include <cstring>
#include <string>
#include <vector>

namespace wechat_ocr_abi
{
struct Engine
{
    uint32_t protocol;
    bool incompatible;
    bool invalid_coordinates;
    std::string text;
};

struct Result
{
    uint32_t width;
    uint32_t height;
    uint32_t protocol;
    bool invalid_coordinates;
    std::string text;
};
}

#ifdef _WIN32
#define TEST_EXPORT __declspec(dllexport)
#else
#define TEST_EXPORT __attribute__((visibility("default")))
#endif

using namespace wechat_ocr_abi;

namespace
{
constexpr char kText[] = "\xe5\xbc\x80\xe5\xa7\x8b";

Status create_engine(const Options* options, Engine** engine)
{
    if (!options || options->struct_size != sizeof(Options) || options->abi_version != kVersion || !engine) {
        return 2;
    }
    if ((options->runtime_dir == nullptr) != (options->ocr_path == nullptr)) {
        return 1;
    }
    const char* entry = options->ocr_path ? options->ocr_path : "";
    *engine = new Engine {
        options->protocol,
        std::strstr(entry, "bad-abi") != nullptr,
        std::strstr(entry, "bad-coordinates") != nullptr,
        kText,
    };
    return 0;
}
}

extern "C"
{
    TEST_EXPORT const char* wco_version()
    {
#ifdef MAA_WECHAT_TEST_LEGACY
        return "0.1.0-test";
#else
        return "0.2.0-test";
#endif
    }

    TEST_EXPORT const char* wco_last_error()
    {
        return "controlled test SDK";
    }

    TEST_EXPORT Status wco_create(const Options* options, Engine** engine)
    {
        if (!options || !options->runtime_dir || !options->ocr_path) {
            return 1;
        }
        return create_engine(options, engine);
    }

#ifndef MAA_WECHAT_TEST_LEGACY
    TEST_EXPORT Status wco_create_auto(const Options* options, const DiscoveryOptions* discovery, Engine** engine)
    {
        if (!discovery || discovery->struct_size != sizeof(DiscoveryOptions) || (discovery->flags & ~1u) != 0
            || (discovery->search_root_count == 0) != (discovery->search_roots == nullptr)) {
            return 1;
        }
        std::vector<std::string> roots;
        for (uint32_t i = 0; i < discovery->search_root_count; ++i) {
            const char* root = discovery->search_roots[i];
            if (!root || !*root) {
                return 1;
            }
            roots.emplace_back(root);
        }
        if (discovery->flags == 0 && roots.empty()) {
            return 3;
        }
        auto status = create_engine(options, engine);
        if (status != 0) {
            return status;
        }
        (*engine)->text += "|auto|scan_system=" + std::to_string(discovery->flags) + "|roots=" + std::to_string(roots.size());
        for (size_t i = 0; i < roots.size(); ++i) {
            (*engine)->text += "|" + std::to_string(i) + ":" + roots[i];
        }
        return 0;
    }
#endif

    TEST_EXPORT Status wco_get_capabilities(const Engine* engine, Capabilities* caps)
    {
        *caps = { sizeof(Capabilities), engine->incompatible ? 2u : kVersion, 1, 0, 0, 1 };
        return 0;
    }

    TEST_EXPORT Status wco_recognize(Engine* engine, const Image* image, uint32_t, Result** result)
    {
        *result = new Result { image->width, image->height, engine->protocol, engine->invalid_coordinates, engine->text };
        return 0;
    }

    TEST_EXPORT uint32_t wco_result_line_count(const Result*)
    {
        return 1;
    }

    TEST_EXPORT Status wco_result_get_info(const Result* result, ResultInfo* info)
    {
        *info = { sizeof(ResultInfo), result->width, result->height, 0 };
        return 0;
    }

    TEST_EXPORT Status wco_result_get_line(const Result* result, uint32_t, Line* line)
    {
        *line = { sizeof(Line),
                  result->protocol == 0 ? 0u : 1u,
                  { 0, 0, float(result->width), 0, float(result->width), float(result->height), 0, float(result->height) },
                  result->protocol == 3 ? 0.2f : 0.9f,
                  result->text.c_str(),
                  result->text.size() };
        if (result->protocol == 4) {
            line->quad[0] = -0.00001f;
            line->quad[2] += 0.00001f;
        }
        if (result->invalid_coordinates) {
            line->quad[0] = -2.0f;
        }
        return 0;
    }

    TEST_EXPORT void wco_result_destroy(Result* result)
    {
        delete result;
    }

    TEST_EXPORT void wco_destroy(Engine* engine)
    {
        delete engine;
    }
}
