#pragma once

#include "MeshData.h"

#include <cstdint>
#include <unordered_map>

/**
 * @brief Incremental mesh builder used by extraction algorithms.
 *
 * Responsibilities:
 * - Deduplicate vertices created along voxel-grid edges.
 * - Accumulate triangle indices before producing immutable MeshData.
 *
 * Assumptions:
 * - Builder state is local to one extraction job to avoid shared mutable buffers.
 */
template<MeshScalar TScalar = float>
class MeshBuilder
{
public:
    /**
     * @brief Stable key representing an interpolated voxel-grid edge.
     */
    struct EdgeKey
    {
        int x0{0};
        int y0{0};
        int z0{0};
        int x1{0};
        int y1{0};
        int z1{0};

        /**
         * @brief Compares two edge keys.
         * @param other Edge key to compare.
         * @return True when both keys represent the same edge.
         */
        [[nodiscard]] bool operator==(const EdgeKey& other) const = default;
    };

    MeshBuilder() = default;

    /**
     * @brief Reserves internal buffers.
     * @param estimatedVertices Expected vertex count.
     * @param estimatedTriangles Expected triangle count.
     */
    void reserve(std::size_t estimatedVertices, std::size_t estimatedTriangles);

    /**
     * @brief Adds or reuses a vertex for a voxel-grid edge.
     * @param edgeKey Edge key used for deduplication.
     * @param position Vertex position.
     * @return Vertex index.
     */
    [[nodiscard]] std::uint32_t addVertexForEdge(const EdgeKey& edgeKey, const Vec3<TScalar>& position);

    /**
     * @brief Appends a triangle.
     * @param i0 First vertex index.
     * @param i1 Second vertex index.
     * @param i2 Third vertex index.
     */
    void addTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2);

    /**
     * @brief Finalizes the builder into mesh data.
     * @return MeshData containing accumulated buffers.
     */
    [[nodiscard]] MeshData<TScalar> build() &&;

private:
    struct EdgeKeyHash
    {
        [[nodiscard]] std::size_t operator()(const EdgeKey& key) const noexcept;
    };

private:
    // Keep builder state local so extraction can later run per block in parallel
    // and merge finished block meshes afterward without shared mutable buffers.
    std::vector<Vec3<TScalar>> m_vertices;
    std::vector<TriangleIndices> m_triangles;
    std::vector<Vec3<TScalar>> m_normals;
    std::unordered_map<EdgeKey, std::uint32_t, EdgeKeyHash> m_vertexIndexByEdge;
};

extern template class MeshBuilder<float>;

#include "MeshBuilder.tpp"
