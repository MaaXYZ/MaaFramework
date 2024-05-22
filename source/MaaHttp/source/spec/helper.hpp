#pragma once

#define MaaDeclStringBuffer(tag, is_call)                                            \
    LHGOutput(CURF::tag);                                                            \
    LHGInherit (pre_process, CURF::tag) {                                            \
        void process()                                                               \
        {                                                                            \
            std::get<index>(arg) = std::get<index>(state) = MaaCreateStringBuffer(); \
        }                                                                            \
    };                                                                               \
    LHGInherit (post_process, CURF::tag) {                                           \
        void process()                                                               \
        {                                                                            \
            MaaDestroyStringBuffer(std::get<index>(state));                          \
        }                                                                            \
    };                                                                               \
    LHGInherit (arg_to_json, CURF::tag, is_call) {                                   \
        void convert()                                                               \
        {                                                                            \
            auto handle = std::get<index>(state);                                    \
            auto size = MaaGetStringSize(handle);                                    \
            std::string data(MaaGetString(handle), size);                            \
            std::cout << name << ' ' << data << std::endl;                           \
            res[name] = data;                                                        \
        }                                                                            \
    };                                                                               \
    LHGSchema(arg_to_json_schema, CURF::tag, is_call, "string");

#define MaaDeclRectOut(tag, is_call)                                 \
    LHGInherit (pre_process, CURF::tag) {                            \
        void process()                                               \
        {                                                            \
            std::get<index>(arg) = &std::get<index>(state);          \
        }                                                            \
    };                                                               \
    LHGInherit (arg_to_json, CURF::tag, is_call) {                   \
        void convert()                                               \
        {                                                            \
            res[name] = pri_maa::from_rect(&std::get<index>(state)); \
        }                                                            \
    };                                                               \
    LHGInherit (arg_to_json_schema, CURF::tag, is_call) {            \
        void schema()                                                \
        {                                                            \
            pri_maa::schema_rect(b);                                 \
            res[name] = b.obj;                                       \
        }                                                            \
    };

#define MaaDeclRectIn(tag, is_call)                                  \
    LHGInherit (pre_process, CURF::tag) {                            \
        void process()                                               \
        {                                                            \
            std::get<index>(arg) = &std::get<index>(state);          \
        }                                                            \
    };                                                               \
    LHGInherit (json_to_arg, CURF::tag, is_call) {                   \
        bool convert()                                               \
        {                                                            \
            if (!req.contains(name)) {                               \
                return false;                                        \
            }                                                        \
            pri_maa::to_rect(req.at(name), &std::get<index>(state)); \
            return true;                                             \
        }                                                            \
    };                                                               \
    LHGInherit (json_to_arg_schema, CURF::tag, is_call) {            \
        void schema()                                                \
        {                                                            \
            pri_maa::schema_rect(b);                                 \
            res[name] = b.obj;                                       \
        }                                                            \
    };

#define MaaDeclPointerOut(tag, is_call)                                                          \
    LHGInherit (arg_to_json, CURF::tag, is_call) {                                               \
        void convert()                                                                           \
        {                                                                                        \
            res[name] = std::format("{:#018x}", reinterpret_cast<size_t>(std::get<index>(arg))); \
        }                                                                                        \
    };                                                                                           \
    LHGSchema(arg_to_json_schema, CURF::tag, is_call, "string");

#define MaaDeclPointerIn(tag, is_call)                                                      \
    LHGInherit (json_to_arg, CURF::tag, is_call) {                                          \
        bool convert()                                                                      \
        {                                                                                   \
            if (!req.contains(name)) {                                                      \
                return false;                                                               \
            }                                                                               \
            std::get<index>(arg) =                                                          \
                reinterpret_cast<void*>(std::stoull(req.at(name).as_string(), nullptr, 0)); \
            return true;                                                                    \
        }                                                                                   \
    };                                                                                      \
    LHGSchema(json_to_arg_schema, CURF::tag, is_call, "string");
