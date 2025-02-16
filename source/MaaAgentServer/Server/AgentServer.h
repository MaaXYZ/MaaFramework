#pragma

#include <string>
#include <unordered_map>
#include <vector>

#include "Conf/Conf.h"
#include "MaaAgentServer/MaaAgentServerDef.h"
#include "Utils/SingletonHolder.hpp"

MAA_AGENT_SERVER_NS_BEGIN

class AgentServer : public SingletonHolder<AgentServer>
{
    struct CustomRecognitionSession
    {
        MaaCustomRecognitionCallback recognition = nullptr;
        void* trans_arg = nullptr;
    };

    struct CustomActionSession
    {
        MaaCustomActionCallback action = nullptr;
        void* trans_arg = nullptr;
    };

public:
    ~AgentServer() = default;

    bool start_up(const std::vector<std::string>& args);
    void shut_down();

    bool register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg);
    bool register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg);

private:
    std::unordered_map<std::string, CustomRecognitionSession> custom_recognitions_;
    std::unordered_map<std::string, CustomActionSession> custom_actions_;
};

MAA_AGENT_SERVER_NS_END
