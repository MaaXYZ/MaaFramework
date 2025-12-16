#pragma once

#include <cstdint>

#include "MaaUtils/Conf.h"

#define MAA_GLOBAL_NS MAA_NS::GlobalNS
#define MAA_GLOBAL_NS_BEGIN \
    namespace MAA_GLOBAL_NS \
    {
#define MAA_GLOBAL_NS_END }

#define MAA_RES_NS MAA_NS::ResourceNS
#define MAA_RES_NS_BEGIN \
    namespace MAA_RES_NS \
    {
#define MAA_RES_NS_END }

#define MAA_CTRL_NS MAA_NS::ControllerNS
#define MAA_CTRL_NS_BEGIN \
    namespace MAA_CTRL_NS \
    {
#define MAA_CTRL_NS_END }

#define MAA_CTRL_UNIT_NS MAA_NS::CtrlUnitNs
#define MAA_CTRL_UNIT_NS_BEGIN \
    namespace MAA_CTRL_UNIT_NS \
    {
#define MAA_CTRL_UNIT_NS_END }

#define MAA_VISION_NS MAA_NS::VisionNS
#define MAA_VISION_NS_BEGIN \
    namespace MAA_VISION_NS \
    {
#define MAA_VISION_NS_END }

#define MAA_TASK_NS MAA_NS::TaskNS
#define MAA_TASK_NS_BEGIN \
    namespace MAA_TASK_NS \
    {
#define MAA_TASK_NS_END }

#define MAA_TOOLKIT_NS MAA_NS::ToolkitNS
#define MAA_TOOLKIT_NS_BEGIN \
    namespace MAA_TOOLKIT_NS \
    {
#define MAA_TOOLKIT_NS_END }

#define MAA_PROJECT_INTERFACE_NS MAA_NS::ProjectInterfaceNS
#define MAA_PROJECT_INTERFACE_NS_BEGIN \
    namespace MAA_PROJECT_INTERFACE_NS \
    {
#define MAA_PROJECT_INTERFACE_NS_END }

#define MAA_AGENT_NS MAA_NS::AgentNS
#define MAA_AGENT_NS_BEGIN \
    namespace MAA_AGENT_NS \
    {
#define MAA_AGENT_NS_END }

#define MAA_AGENT_CLIENT_NS MAA_AGENT_NS::ClientNS

#define MAA_AGENT_CLIENT_NS_BEGIN \
    namespace MAA_AGENT_CLIENT_NS \
    {
#define MAA_AGENT_CLIENT_NS_END }

#define MAA_AGENT_SERVER_NS MAA_AGENT_NS::ServerNS
#define MAA_AGENT_SERVER_NS_BEGIN \
    namespace MAA_AGENT_SERVER_NS \
    {
#define MAA_AGENT_SERVER_NS_END }

constexpr int64_t kRunnerIdBase = 100'000'000;
constexpr int64_t kTaskIdBase = 200'000'000;
constexpr int64_t kNodeIdBase = 300'000'000;
constexpr int64_t kRecoIdBase = 400'000'000;
constexpr int64_t kActIdBase = 500'000'000;
