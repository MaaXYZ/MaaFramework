#pragma once

#define MAA_GRPC_REQUIRED(name)                                 \
    if (!request->has_##name()) {                               \
        return Status(INVALID_ARGUMENT, #name " not provided"); \
    }

#define MAA_GRPC_REQUIRED_OPTION(name, type)                        \
    if (request->name##_case() == ::maarpc::type::OPTION_NOT_SET) { \
        return Status(INVALID_ARGUMENT, #name " not provided");     \
    }

#define MAA_GRPC_GET_HANDLE_BEGIN                          \
    decltype(handles)::mapped_type handle = nullptr;       \
    {                                                      \
        auto id = request->handle();                       \
        std::unique_lock<std::mutex> lock(handles_mtx);    \
        if (!handles.contains(id)) {                       \
            return Status(NOT_FOUND, "handle not exists"); \
        }                                                  \
        handle = handles[id];

#define MAA_GRPC_GET_HANDLE_END }

#define MAA_GRPC_GET_HANDLE MAA_GRPC_GET_HANDLE_BEGIN MAA_GRPC_GET_HANDLE_END
#define MAA_GRPC_SET_HANDLE                             \
    {                                                   \
        std::unique_lock<std::mutex> lock(handles_mtx); \
        handles[id] = handle;                           \
    }
