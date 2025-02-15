#pragma once

#include "Conf/Conf.h"

#include "Common/MaaTypes.h"
#include "MaaFramework/MaaDef.h"
#include "Resource/ResourceMgr.h"
#include "Task/Context.h"

MAA_TASK_NS_BEGIN

class CustomAction
{
public:
    static bool
        run(Context& context,
            std::string node_name,
            MAA_RES_NS::CustomActionSession session,
            const MAA_RES_NS::Action::CustomParam& param,
            MaaRecoId reco_id,
            const cv::Rect& rect);
};

MAA_TASK_NS_END
