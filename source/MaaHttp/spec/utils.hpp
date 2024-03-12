#pragma once

#pragma region call

#define __CALL_DECLARE_INPUT(arg_tag, v) \
    template <>                          \
    struct is_input<arg_tag, true>       \
    {                                    \
        constexpr static bool value = v; \
    };

#define __CALL_DECLARE_OUTPUT(arg_tag, v) \
    template <>                           \
    struct is_output<arg_tag, true>       \
    {                                     \
        constexpr static bool value = v;  \
    };

#define __CALL_DECLARE_OUTER_STATE(arg_tag, t) \
    template <>                                \
    struct is_outer_state<arg_tag, true>       \
    {                                          \
        constexpr static bool value = true;    \
        using type = t;                        \
    };

#define __CALL_DECLARE_HANDLE_OPER(arg_tag, o)              \
    template <>                                             \
    struct is_handle<arg_tag, true>                         \
    {                                                       \
        constexpr static bool value = true;                 \
        constexpr static handle_oper oper = handle_oper::o; \
    };

#define __CALL_DECLARE_PREPARE_STATE_BEGIN(func_tag, atag)                                                \
    template <>                                                                                           \
    struct prepare_state<typename func_tag::args, func_tag::atag::index, true>                            \
    {                                                                                                     \
        using arg_tuple = func_tag::args;                                                                 \
        using arg_tag = func_tag::atag;                                                                   \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                               \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                      \
        static bool prepare(lhg::ManagerProvider& provider, const json::object& req, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                                  \
        {

#define __CALL_DECLARE_PREPARE_STATE_END() \
    }                                      \
    }                                      \
    ;

#define __CALL_DECLARE_JSON_TO_ARG_BEGIN(func_tag, atag)                                                  \
    template <>                                                                                           \
    struct json_to_arg<typename func_tag::args, func_tag::atag::index, true>                              \
    {                                                                                                     \
        using arg_tuple = func_tag::args;                                                                 \
        using arg_tag = func_tag::atag;                                                                   \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                               \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                      \
        static bool convert(lhg::ManagerProvider& provider, const json::object& req, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                                  \
        {                                                                                                 \
            if constexpr (!is_input<arg_tag, true>::value) {                                              \
                return true;                                                                              \
            }                                                                                             \
            else {                                                                                        \
                auto name = arg_tag::name;                                                                \
                if (!req.contains(name)) {                                                                \
                    return false;                                                                         \
                }                                                                                         \
                const json::value& value = req.at(name);
#define __DECLARE_JSON_TO_ARG_END() \
    }                               \
    }                               \
    }                               \
    ;

#define __CALL_DECLARE_ARG_TO_JSON_BEGIN(func_tag, atag)                                            \
    template <>                                                                                     \
    struct arg_to_json<typename func_tag::args, func_tag::atag::index, true>                        \
    {                                                                                               \
        using arg_tuple = func_tag::args;                                                           \
        using arg_tag = func_tag::atag;                                                             \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                         \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                \
        static void convert(lhg::ManagerProvider& provider, json::object& res, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                            \
        {                                                                                           \
            if constexpr (!is_output<arg_tag, true>::value) {                                       \
                return;                                                                             \
            }                                                                                       \
            else {                                                                                  \
                auto name = arg_tag::name;
#define __CALL_DECLARE_ARG_TO_JSON_END() \
    }                                    \
    }                                    \
    }                                    \
    ;

#pragma endregion call

#pragma region callback

#define __CALLBACK_DECLARE_INPUT(arg_tag, v) \
    template <>                              \
    struct is_input<arg_tag, true>           \
    {                                        \
        constexpr static bool value = v;     \
    };

#define __CALLBACK_DECLARE_OUTPUT(arg_tag, v) \
    template <>                               \
    struct is_output<arg_tag, true>           \
    {                                         \
        constexpr static bool value = v;      \
    };

#define __CALLBACK_DECLARE_OUTER_STATE(arg_tag, t) \
    template <>                                    \
    struct is_outer_state<arg_tag, true>           \
    {                                              \
        constexpr static bool value = true;        \
        using type = t;                            \
    };

#define __CALLBACK_DECLARE_HANDLE_OPER(arg_tag, o)          \
    template <>                                             \
    struct is_handle<arg_tag, true>                         \
    {                                                       \
        constexpr static bool value = true;                 \
        constexpr static handle_oper oper = handle_oper::o; \
    };

#define __CALLBACK_DECLARE_JSON_TO_ARG_BEGIN(func_tag, atag)                                              \
    template <>                                                                                           \
    struct json_to_arg<typename func_tag::args, func_tag::atag::index, true>                              \
    {                                                                                                     \
        using arg_tuple = func_tag::args;                                                                 \
        using arg_tag = func_tag::atag;                                                                   \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                               \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                      \
        static bool convert(lhg::ManagerProvider& provider, const json::object& res, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                                  \
        {                                                                                                 \
            if constexpr (!is_output<arg_tag, true>::value) {                                             \
                return true;                                                                              \
            }                                                                                             \
            else {                                                                                        \
                auto name = arg_tag::name;                                                                \
                if (!res.contains(name)) {                                                                \
                    return false;                                                                         \
                }                                                                                         \
                const json::value& value = res.at(name);
#define __CALLBACK_DECLARE_JSON_TO_ARG_END() \
    }                                        \
    }                                        \
    }                                        \
    ;

#define __CALLBACK_DECLARE_ARG_TO_JSON_BEGIN(func_tag, atag)                                        \
    template <>                                                                                     \
    struct arg_to_json<typename func_tag::args, func_tag::atag::index, true>                        \
    {                                                                                               \
        using arg_tuple = func_tag::args;                                                           \
        using arg_tag = func_tag::atag;                                                             \
        using call_arg_tuple = convert_arg_type<arg_tuple>;                                         \
        using call_arg_state_tuple = convert_outer_state<arg_tuple>;                                \
        static void convert(lhg::ManagerProvider& provider, json::object& req, call_arg_tuple& arg, \
                            call_arg_state_tuple& state)                                            \
        {                                                                                           \
            if constexpr (!is_input<arg_tag, true>::value) {                                        \
                return;                                                                             \
            }                                                                                       \
            else {                                                                                  \
                auto name = arg_tag::name;
#define __CALLBACK_DECLARE_ARG_TO_JSON_END() \
    }                                        \
    }                                        \
    }                                        \
    ;

#pragma endregion callback
