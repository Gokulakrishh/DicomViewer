#pragma once

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

template<MeshScalar TScalar>
MeshData<TScalar>::MeshData(
    std::vector<Vec3<TScalar>> vertices,
    std::vector<TriangleIndices> triangles,
    std::vector<Vec3<TScalar>> normals)
    : m_vertices(std::move(vertices)),
      m_triangles(std::move(triangles)),
      m_normals(std::move(normals))
{
    if (!m_normals.empty() && m_normals.size() != m_vertices.size())
    {
        throw std::invalid_argument("Mesh normal count must match vertex count");
    }

    validateTriangleIndices();
    computeBounds();
}

template<MeshScalar TScalar>
std::size_t MeshData<TScalar>::vertexCount() const
{
    return m_vertices.size();
}

template<MeshScalar TScalar>
std::size_t MeshData<TScalar>::triangleCount() const
{
    return m_triangles.size();
}

template<MeshScalar TScalar>
bool MeshData<TScalar>::hasVertexNormals() const
{
    return !m_normals.empty();
}

template<MeshScalar TScalar>
std::array<double, 3> MeshData<TScalar>::vertexPosition(std::size_t index) const
{
    const Vec3<TScalar>& vertex = checkedVertex(index);
    return {static_cast<double>(vertex.x), static_cast<double>(vertex.y), static_cast<double>(vertex.z)};
}

template<MeshScalar TScalar>
std::array<double, 3> MeshData<TScalar>::vertexNormal(std::size_t index) const
{
    if (m_normals.empty())
    {
        throw std::out_of_range("Mesh does not contain vertex normals");
    }

    const Vec3<TScalar>& normal = checkedNormal(index);
    return {static_cast<double>(normal.x), static_cast<double>(normal.y), static_cast<double>(normal.z)};
}

template<MeshScalar TScalar>
std::array<std::uint32_t, 3> MeshData<TScalar>::triangleIndices(std::size_t index) const
{
    const TriangleIndices& triangle = checkedTriangle(index);
    return {triangle.i0, triangle.i1, triangle.i2};
}

template<MeshScalar TScalar>
std::array<double, 3> MeshData<TScalar>::minBounds() const
{
    return {static_cast<double>(m_minBounds.x),
            static_cast<double>(m_minBounds.y),
            static_cast<double>(m_minBounds.z)};
}

template<MeshScalar TScalar>
std::array<double, 3> MeshData<TScalar>::maxBounds() const
{
    return {static_cast<double>(m_maxBounds.x),
            static_cast<double>(m_maxBounds.y),
            static_cast<double>(m_maxBounds.z)};
}

template<MeshScalar TScalar>
const std::vector<Vec3<TScalar>>& MeshData<TScalar>::vertices() const
{
    return m_vertices;
}

template<MeshScalar TScalar>
const std::vector<TriangleIndices>& MeshData<TScalar>::triangles() const
{
    return m_triangles;
}

template<MeshScalar TScalar>
const std::vector<Vec3<TScalar>>& MeshData<TScalar>::normals() const
{
    return m_normals;
}

template<MeshScalar TScalar>
void MeshData<TScalar>::validateTriangleIndices() const
{
    for (const TriangleIndices& triangle : m_triangles)
    {
        const std::size_t vertexCountValue = m_vertices.size();
        if (triangle.i0 >= vertexCountValue ||
            triangle.i1 >= vertexCountValue ||
            triangle.i2 >= vertexCountValue)
        {
            throw std::invalid_argument("Triangle index is out of range for mesh vertices");
        }
    }
}

template<MeshScalar TScalar>
void MeshData<TScalar>::computeBounds()
{
    if (m_vertices.empty())
    {
        m_minBounds = {};
        m_maxBounds = {};
        return;
    }

    const TScalar maxValue = std::numeric_limits<TScalar>::max();
    const TScalar minValue = std::numeric_limits<TScalar>::lowest();
    m_minBounds = {maxValue, maxValue, maxValue};
    m_maxBounds = {minValue, minValue, minValue};

    for (const Vec3<TScalar>& vertex : m_vertices)
    {
        m_minBounds.x = std::min(m_minBounds.x, vertex.x);
        m_minBounds.y = std::min(m_minBounds.y, vertex.y);
        m_minBounds.z = std::min(m_minBounds.z, vertex.z);

        m_maxBounds.x = std::max(m_maxBounds.x, vertex.x);
        m_maxBounds.y = std::max(m_maxBounds.y, vertex.y);
        m_maxBounds.z = std::max(m_maxBounds.z, vertex.z);
    }
}

template<MeshScalar TScalar>
const Vec3<TScalar>& MeshData<TScalar>::checkedVertex(std::size_t index) const
{
    if (index >= m_vertices.size())
    {
        throw std::out_of_range("Mesh vertex index is out of range");
    }

    return m_vertices[index];
}

template<MeshScalar TScalar>
const Vec3<TScalar>& MeshData<TScalar>::checkedNormal(std::size_t index) const
{
    if (index >= m_normals.size())
    {
        throw std::out_of_range("Mesh normal index is out of range");
    }

    return m_normals[index];
}

template<MeshScalar TScalar>
const TriangleIndices& MeshData<TScalar>::checkedTriangle(std::size_t index) const
{
    if (index >= m_triangles.size())
    {
        throw std::out_of_range("Mesh triangle index is out of range");
    }

    return m_triangles[index];
}
