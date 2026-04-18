#pragma once

#include "IMeshData.h"
#include "MeshConcepts.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

template<MeshScalar TScalar>
struct Vec3
{
    TScalar x{};
    TScalar y{};
    TScalar z{};
};

struct TriangleIndices
{
    std::uint32_t i0{};
    std::uint32_t i1{};
    std::uint32_t i2{};
};

template<MeshScalar TScalar = float>
class MeshData final : public IMeshData
{
public:
    MeshData() = default;

    MeshData(
        std::vector<Vec3<TScalar>> vertices,
        std::vector<TriangleIndices> triangles,
        std::vector<Vec3<TScalar>> normals = {});

    [[nodiscard]] std::size_t vertexCount() const override;
    [[nodiscard]] std::size_t triangleCount() const override;
    [[nodiscard]] bool hasVertexNormals() const override;

    [[nodiscard]] std::array<double, 3> vertexPosition(std::size_t index) const override;
    [[nodiscard]] std::array<double, 3> vertexNormal(std::size_t index) const override;
    [[nodiscard]] std::array<std::uint32_t, 3> triangleIndices(std::size_t index) const override;

    [[nodiscard]] std::array<double, 3> minBounds() const override;
    [[nodiscard]] std::array<double, 3> maxBounds() const override;

    [[nodiscard]] const std::vector<Vec3<TScalar>>& vertices() const;
    [[nodiscard]] const std::vector<TriangleIndices>& triangles() const;
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
