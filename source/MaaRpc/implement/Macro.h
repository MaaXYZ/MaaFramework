#pragma once

#define MAA_GRPC_REQUIRED(name)                                 \
    if (!request->has_##name()) {                               \
        return Status(INVALID_ARGUMENT, #name " not provided"); \
    }

#define MAA_GRPC_REQUIRED_CASE(name, name_cap)                                                        \
    if (request->name##_case() == std::remove_pointer<decltype(request)>::type::name_cap##_NOT_SET) { \
        return Status(INVALID_ARGUMENT, #name " not provided");                                       \
    }

#define MAA_GRPC_REQUIRED_CASE_AS(name, case)                                              \
    if (request->name##_case() != std::remove_pointer<decltype(request)>::type::k##case) { \
        return Status(INVALID_ARGUMENT, #name " should be " #case);                        \
    }

#define MAA_GRPC_REQUIRED_OF(name, req)                         \
    if (!req->has_##name()) {                                   \
        return Status(INVALID_ARGUMENT, #name " not provided"); \
    }

#define MAA_GRPC_GET_HANDLE                                         \
    std::decay_t<decltype(handles())>::HandleType handle = nullptr; \
    {                                                               \
        auto id = request->handle();                                \
        if (!handles().get(id, handle)) {                           \
            return Status(NOT_FOUND, "handle not exists");          \
        }                                                           \
    }

#define MAA_GRPC_GET_HANDLE_FROM(impl, name, field)                     \
    std::decay_t<decltype(impl->handles())>::HandleType name = nullptr; \
    {                                                                   \
        auto id = request->field();                                     \
        if (!impl->handles().get(id, name)) {                           \
            return Status(NOT_FOUND, #field " not exists");             \
        }                                                               \
    }

#define MAA_GRPC_DEL_HANDLE                                         \
    std::decay_t<decltype(handles())>::HandleType handle = nullptr; \
    {                                                               \
        auto id = request->handle();                                \
        if (!handles().del(id, handle)) {                           \
            return Status(NOT_FOUND, "handle not exists");          \
        }                                                           \
    }
