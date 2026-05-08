#pragma once

#include "IMeshPostProcessor.h"

#include <memory>
#include <vector>

/**
 * @brief Mesh post-processor that applies multiple processors in sequence.
 *
 * Responsibilities:
 * - Compose smoothing, normal generation, and future post-processing steps.
 * - Keep the 3D pipeline profile API to a single post-processor dependency.
 */
class CompositeMeshPostProcessor final : public IMeshPostProcessor
{
public:
    /**
     * @brief Creates a composite processor.
     * @param processors Ordered processor list.
     */
    explicit CompositeMeshPostProcessor(std::vector<std::shared_ptr<IMeshPostProcessor>> processors);

    /**
     * @brief Applies all configured processors.
     * @param mesh Input mesh.
     * @return Final processed mesh.
     */
    [[nodiscard]] std::shared_ptr<IMeshData> process(const IMeshData& mesh) const override;

    /**
     * @brief Returns configured processors.
     * @return Ordered processor list.
     */
    [[nodiscard]] const std::vector<std::shared_ptr<IMeshPostProcessor>>& processors() const;

private:
    std::vector<std::shared_ptr<IMeshPostProcessor>> m_processors;
};
