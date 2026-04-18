#pragma once

#include "Services/IMeshPostProcessor.h"

struct LaplacianMeshSmoothingParameters
{
    int iterations{5};
    float lambda{0.2F};
};

class LaplacianMeshSmoothingPostProcessor final : public IMeshPostProcessor
{
public:
    explicit LaplacianMeshSmoothingPostProcessor(LaplacianMeshSmoothingParameters parameters = {});

    [[nodiscard]] std::shared_ptr<IMeshData> process(const IMeshData& mesh) const override;
    [[nodiscard]] const LaplacianMeshSmoothingParameters& parameters() const;

private:
    LaplacianMeshSmoothingParameters m_parameters;
};
