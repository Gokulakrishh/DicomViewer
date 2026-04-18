#pragma once

#include "IMeshPostProcessor.h"

#include <memory>
#include <vector>

class CompositeMeshPostProcessor final : public IMeshPostProcessor
{
public:
    explicit CompositeMeshPostProcessor(std::vector<std::shared_ptr<IMeshPostProcessor>> processors);

    [[nodiscard]] std::shared_ptr<IMeshData> process(const IMeshData& mesh) const override;
    [[nodiscard]] const std::vector<std::shared_ptr<IMeshPostProcessor>>& processors() const;

private:
    std::vector<std::shared_ptr<IMeshPostProcessor>> m_processors;
};
