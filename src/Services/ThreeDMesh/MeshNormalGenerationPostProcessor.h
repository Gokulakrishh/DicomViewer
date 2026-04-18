#pragma once

#include "Services/IMeshPostProcessor.h"

class MeshNormalGenerationPostProcessor final : public IMeshPostProcessor
{
public:
    [[nodiscard]] std::shared_ptr<IMeshData> process(const IMeshData& mesh) const override;
};
