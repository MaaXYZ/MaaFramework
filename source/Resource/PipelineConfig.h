#pragma once

#include "AbstractResource.h"

MAA_RES_NS_BEGIN

class PipelineConfig : public AbstractResource
{
public:
    using AbstractResource::AbstractResource;
    virtual ~PipelineConfig() override = default;

public:
    virtual bool load(const std::filesystem::path& path) override;
};

MAA_RES_NS_END
