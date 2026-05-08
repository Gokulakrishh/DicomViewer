#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * @brief Read-only triangle mesh interface.
 *
 * Responsibilities:
 * - Expose mesh vertices, triangles, normals, and bounds without concrete
 *   storage details.
 * - Support rendering adapters and post-processing services.
 */
class IMeshData
{
public:
    virtual ~IMeshData() = default;

    /** @brief Returns number of mesh vertices. */
    [[nodiscard]] virtual std::size_t vertexCount() const = 0;
    /** @brief Returns number of mesh triangles. */
    [[nodiscard]] virtual std::size_t triangleCount() const = 0;
    /** @brief Reports whether vertex normals are available. */
    [[nodiscard]] virtual bool hasVertexNormals() const = 0;

    /** @brief Returns vertex position by index. */
    [[nodiscard]] virtual std::array<double, 3> vertexPosition(std::size_t index) const = 0;
    /** @brief Returns vertex normal by index. */
    [[nodiscard]] virtual std::array<double, 3> vertexNormal(std::size_t index) const = 0;
    /** @brief Returns triangle vertex indices by index. */
    [[nodiscard]] virtual std::array<std::uint32_t, 3> triangleIndices(std::size_t index) const = 0;

    /** @brief Returns minimum mesh bounds. */
    [[nodiscard]] virtual std::array<double, 3> minBounds() const = 0;
    /** @brief Returns maximum mesh bounds. */
    [[nodiscard]] virtual std::array<double, 3> maxBounds() const = 0;
};
