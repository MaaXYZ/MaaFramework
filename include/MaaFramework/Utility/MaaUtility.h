/**
 * @file MaaUtility.h
 * @author
 * @brief Provide global functions for the framework.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Get the version of the framework.
     *
     * @return MaaStringView
     */
    MaaStringView MAA_FRAMEWORK_API MaaVersion();

    /**
     * @brief Set options globally.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail. See
     * #MaaGlobalOptionEnum for details.
     *
     * @param key The option key.
     * @param value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MaaBool MAA_FRAMEWORK_API MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value,
                                                 MaaOptionValueSize val_size);

#ifdef __cplusplus
}
#endif
