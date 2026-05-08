#pragma once

#include "Services/IMeshPostProcessor.h"

/**
 * @brief Generates per-vertex normals for triangle meshes.
 *
 * Responsibilities:
 * - Add normals required by lighting/rendering pipelines.
 * - Leave geometry topology unchanged.
 */
class MeshNormalGenerationPostProcessor final : public IMeshPostProcessor
{
public:
    /**
     * @brief Generates normals for a mesh.
     * @param mesh Input mesh.
     * @return Mesh with generated normals.
     */
    [[nodiscard]] std::shared_ptr<IMeshData> process(const IMeshData& mesh) const override;
};
