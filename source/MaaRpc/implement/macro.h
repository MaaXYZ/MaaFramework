#pragma once

#define MAA_GRPC_REQUIRED(name)                                 \
    if (!request->has_##name()) {                               \
        return Status(INVALID_ARGUMENT, #name " not provided"); \
    }

#define MAA_GRPC_REQUIRED_OPTION(name, type)                        \
    if (request->name##_case() == ::maarpc::type::OPTION_NOT_SET) { \
        return Status(INVALID_ARGUMENT, #name " not provided");     \
    }
