#include <napi.h>

Napi::Object Init(Napi::Env env, Napi::Object exports);

NODE_API_MODULE(maa, Init);
