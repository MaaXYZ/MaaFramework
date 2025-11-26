/**
 * @file MaaToolkitProjectInterface.h
 * @brief Project Interface API for MaaToolkit
 *
 * @copyright Copyright (c) 2024
 */

// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Load interface.json and read agent configuration
     *
     * @param interface_path Path to the interface.json file
     * @return MaaBool Whether the load was successful
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceLoad(const char* interface_path);

    /**
     * @brief Check if project interface is loaded
     *
     * @return MaaBool Whether the interface is loaded
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceLoaded();

    /**
     * @brief Bind resource to the agent
     *
     * @param resource The resource to bind
     * @return MaaBool Whether the binding was successful
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceBindResource(MaaResource* resource);

    /**
     * @brief Start the agent using loaded configuration
     *
     * @return MaaBool Whether the start was successful
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceStartAgent();

    /**
     * @brief Stop the agent
     */
    MAA_TOOLKIT_API void MaaToolkitProjectInterfaceStopAgent();

    /**
     * @brief Check if the agent is running
     *
     * @return MaaBool Whether the agent is running
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceAgentRunning();

    /**
     * @brief Check if the agent is connected
     *
     * @return MaaBool Whether the agent is connected
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitProjectInterfaceAgentConnected();

#ifdef __cplusplus
}
#endif
