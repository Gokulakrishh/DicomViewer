#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class IMeshData;

/**
 * @brief Converts mesh data into flat buffers consumed by QML/Quick3D geometry.
 *
 * Responsibilities:
 * - Hold the current mesh reference.
 * - Rebuild position, normal, and index buffers for rendering.
 */
class MeshSceneAdapter
{
public:
    /** @brief Sets the active mesh and rebuilds render buffers. */
    void setMesh(std::shared_ptr<IMeshData> mesh);
    /** @brief Clears the active mesh and buffers. */
    void clear();

    /** @brief Reports whether a mesh is available. */
    [[nodiscard]] bool hasMesh() const;
    /** @brief Returns the active mesh reference. */
    [[nodiscard]] const std::shared_ptr<IMeshData>& mesh() const;
    /** @brief Returns the flat position buffer. */
    [[nodiscard]] const std::vector<float>& positionBuffer() const;
    /** @brief Returns the flat normal buffer. */
    [[nodiscard]] const std::vector<float>& normalBuffer() const;
    /** @brief Returns the triangle index buffer. */
    [[nodiscard]] const std::vector<std::uint32_t>& indexBuffer() const;

private:
    void rebuildBuffers();

private:
    std::shared_ptr<IMeshData> m_mesh;
    std::vector<float> m_positionBuffer;
    std::vector<float> m_normalBuffer;
    std::vector<std::uint32_t> m_indexBuffer;
};
