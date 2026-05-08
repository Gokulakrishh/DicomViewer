#pragma once

#include "Services/IMeshPostProcessor.h"

/**
 * @brief Parameters for Laplacian mesh smoothing.
 */
struct LaplacianMeshSmoothingParameters
{
    int iterations{5};
    float lambda{0.2F};
};

/**
 * @brief Laplacian smoothing post-processor for extracted 3D meshes.
 *
 * Responsibilities:
 * - Reduce stair-step artifacts from voxel-derived meshes.
 * - Preserve pipeline composition through IMeshPostProcessor.
 */
class LaplacianMeshSmoothingPostProcessor final : public IMeshPostProcessor
{
public:
    /**
     * @brief Creates a smoothing processor.
     * @param parameters Smoothing iteration and lambda settings.
     */
    explicit LaplacianMeshSmoothingPostProcessor(LaplacianMeshSmoothingParameters parameters = {});

    /**
     * @brief Smooths a mesh.
     * @param mesh Input mesh.
     * @return Smoothed mesh.
     */
    [[nodiscard]] std::shared_ptr<IMeshData> process(const IMeshData& mesh) const override;

    /**
     * @brief Returns smoothing parameters.
     * @return Current parameters.
     */
    [[nodiscard]] const LaplacianMeshSmoothingParameters& parameters() const;

private:
    LaplacianMeshSmoothingParameters m_parameters;
};
