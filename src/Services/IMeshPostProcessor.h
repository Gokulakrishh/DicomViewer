#pragma once

#include <memory>

class IMeshData;

/**
 * @brief Strategy interface for mesh post-processing.
 *
 * Responsibilities:
 * - Apply smoothing, normal generation, or other mesh refinements.
 * - Keep 3D profile composition independent of concrete processors.
 */
class IMeshPostProcessor
{
public:
    virtual ~IMeshPostProcessor() = default;

    /**
     * @brief Processes a mesh.
     * @param mesh Input mesh.
     * @return Processed mesh, or null on failure.
     */
    [[nodiscard]] virtual std::shared_ptr<IMeshData> process(const IMeshData& mesh) const = 0;
};
