#pragma once

#include <type_traits>

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

template <typename T>
class SingletonHolder : public NonCopyable
{
public:
    static T& get_instance()
    {
        static T unique_instance;
        return unique_instance;
    }

    virtual ~SingletonHolder() = default;

protected:
    SingletonHolder() = default;
};

template <typename T>
concept Singleton = std::is_base_of_v<SingletonHolder<T>, T>;

MAA_NS_END
