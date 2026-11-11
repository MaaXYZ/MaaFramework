#include "../../source/MaaFramework/Resource/WeChatOCRABI.h"

#include <cstring>

namespace wechat_ocr_abi
{
struct Engine
{
    uint32_t protocol;
    bool incompatible;
    bool invalid_coordinates;
};

struct Result
{
    uint32_t width;
    uint32_t height;
    uint32_t protocol;
    bool invalid_coordinates;
};
}

#ifdef _WIN32
#define TEST_EXPORT __declspec(dllexport)
#else
#define TEST_EXPORT __attribute__((visibility("default")))
#endif

using namespace wechat_ocr_abi;

extern "C"
{
    TEST_EXPORT const char* wco_version()
    {
        return "0.1.0-test";
    }

    TEST_EXPORT const char* wco_last_error()
    {
        return "controlled test SDK";
    }

    TEST_EXPORT Status wco_create(const Options* options, Engine** engine)
    {
        if (!options || options->abi_version != kVersion || !engine) {
            return 2;
        }
        *engine = new Engine {
            options->protocol,
            std::strstr(options->ocr_path, "bad-abi") != nullptr,
            std::strstr(options->ocr_path, "bad-coordinates") != nullptr,
        };
        return 0;
    }

    TEST_EXPORT Status wco_get_capabilities(const Engine* engine, Capabilities* caps)
    {
        *caps = { sizeof(Capabilities), engine->incompatible ? 2u : kVersion, 1, 0, 0, 1 };
        return 0;
    }

    TEST_EXPORT Status wco_recognize(Engine* engine, const Image* image, uint32_t, Result** result)
    {
        *result = new Result { image->width, image->height, engine->protocol, engine->invalid_coordinates };
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
        static constexpr char text[] = "\xe5\xbc\x80\xe5\xa7\x8b";
        *line = { sizeof(Line),
                  result->protocol == 0 ? 0u : 1u,
                  { 0, 0, float(result->width), 0, float(result->width), float(result->height), 0, float(result->height) },
                  result->protocol == 3 ? 0.2f : 0.9f,
                  text,
                  sizeof(text) - 1 };
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
