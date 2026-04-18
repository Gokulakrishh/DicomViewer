#pragma once

#include "MeshData.h"

#include <cstdint>
#include <unordered_map>

template<MeshScalar TScalar = float>
class MeshBuilder
{
public:
    struct EdgeKey
    {
        int x0{0};
        int y0{0};
        int z0{0};
        int x1{0};
        int y1{0};
        int z1{0};

        [[nodiscard]] bool operator==(const EdgeKey& other) const = default;
    };

    MeshBuilder() = default;

    void reserve(std::size_t estimatedVertices, std::size_t estimatedTriangles);
    [[nodiscard]] std::uint32_t addVertexForEdge(const EdgeKey& edgeKey, const Vec3<TScalar>& position);
    void addTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2);
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
