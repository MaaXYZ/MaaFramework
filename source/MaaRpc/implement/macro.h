#pragma once

#define MAA_GRPC_REQUIRED(name)                                 \
    if (!request->has_##name()) {                               \
        return Status(INVALID_ARGUMENT, #name " not provided"); \
    }

#define MAA_GRPC_REQUIRED_OPTION(name, type)                        \
    if (request->name##_case() == ::maarpc::type::OPTION_NOT_SET) { \
        return Status(INVALID_ARGUMENT, #name " not provided");     \
    }

#define MAA_GRPC_GET_HANDLE                                \
    decltype(handles)::HandleType handle = nullptr;        \
    {                                                      \
        auto id = request->handle();                       \
        if (!handles.get(id, handle)) {                    \
            return Status(NOT_FOUND, "handle not exists"); \
        }                                                  \
    }

#define MAA_GRPC_GET_HANDLE_FROM(impl, name, field)         \
    decltype(impl->handles)::HandleType name = nullptr;     \
    {                                                       \
        auto id = request->field();                         \
        if (!impl->handles.get(id, name)) {                 \
            return Status(NOT_FOUND, #field " not exists"); \
        }                                                   \
    }

#define MAA_GRPC_DEL_HANDLE                                \
    decltype(handles)::HandleType handle = nullptr;        \
    {                                                      \
        auto id = request->handle();                       \
        if (!handles.del(id, handle)) {                    \
            return Status(NOT_FOUND, "handle not exists"); \
        }                                                  \
    }
