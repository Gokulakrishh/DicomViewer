#pragma once

#include "IMeshData.h"
#include "MeshConcepts.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * @brief Three-component vector used by mesh buffers.
 */
template<MeshScalar TScalar>
struct Vec3
{
    TScalar x{};
    TScalar y{};
    TScalar z{};
};

/**
 * @brief One triangle represented by three vertex indices.
 */
struct TriangleIndices
{
    std::uint32_t i0{};
    std::uint32_t i1{};
    std::uint32_t i2{};
};

/**
 * @brief Typed triangle mesh data container.
 *
 * Responsibilities:
 * - Own vertices, triangle indices, optional normals, and computed bounds.
 * - Provide IMeshData access for rendering and pipeline post-processing.
 *
 * Assumptions:
 * - Triangle indices are validated against the vertex buffer.
 */
template<MeshScalar TScalar = float>
class MeshData final : public IMeshData
{
public:
    MeshData() = default;

    /**
     * @brief Creates a mesh from vertex, triangle, and optional normal buffers.
     * @param vertices Mesh vertices.
     * @param triangles Triangle index buffer.
     * @param normals Optional per-vertex normals.
     */
    MeshData(
        std::vector<Vec3<TScalar>> vertices,
        std::vector<TriangleIndices> triangles,
        std::vector<Vec3<TScalar>> normals = {});

    /** @brief Returns number of vertices. */
    [[nodiscard]] std::size_t vertexCount() const override;
    /** @brief Returns number of triangles. */
    [[nodiscard]] std::size_t triangleCount() const override;
    /** @brief Reports whether normals are available. */
    [[nodiscard]] bool hasVertexNormals() const override;

    /** @brief Returns vertex position by index. */
    [[nodiscard]] std::array<double, 3> vertexPosition(std::size_t index) const override;
    /** @brief Returns vertex normal by index. */
    [[nodiscard]] std::array<double, 3> vertexNormal(std::size_t index) const override;
    /** @brief Returns triangle indices by index. */
    [[nodiscard]] std::array<std::uint32_t, 3> triangleIndices(std::size_t index) const override;

    /** @brief Returns minimum mesh bounds. */
    [[nodiscard]] std::array<double, 3> minBounds() const override;
    /** @brief Returns maximum mesh bounds. */
    [[nodiscard]] std::array<double, 3> maxBounds() const override;

    /** @brief Returns typed vertex buffer. */
    [[nodiscard]] const std::vector<Vec3<TScalar>>& vertices() const;
    /** @brief Returns triangle index buffer. */
    [[nodiscard]] const std::vector<TriangleIndices>& triangles() const;
    /** @brief Returns typed normal buffer. */
    [[nodiscard]] const std::vector<Vec3<TScalar>>& normals() const;

private:
    void validateTriangleIndices() const;
    void computeBounds();

    [[nodiscard]] const Vec3<TScalar>& checkedVertex(std::size_t index) const;
    [[nodiscard]] const Vec3<TScalar>& checkedNormal(std::size_t index) const;
    [[nodiscard]] const TriangleIndices& checkedTriangle(std::size_t index) const;

private:
    std::vector<Vec3<TScalar>> m_vertices;
    std::vector<TriangleIndices> m_triangles;
    std::vector<Vec3<TScalar>> m_normals;
    Vec3<TScalar> m_minBounds{};
    Vec3<TScalar> m_maxBounds{};
};
//This is explicit template instantiation declaration
extern template class MeshData<float>;
extern template class MeshData<double>;

#include "MeshData.tpp"
