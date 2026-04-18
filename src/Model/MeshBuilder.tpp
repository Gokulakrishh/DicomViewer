#pragma once

#include "Utilities/Math3D.h"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

template<MeshScalar TScalar>
void MeshBuilder<TScalar>::reserve(std::size_t estimatedVertices, std::size_t estimatedTriangles)
{
    m_vertices.reserve(estimatedVertices);
    m_triangles.reserve(estimatedTriangles);
    m_normals.reserve(estimatedVertices);
    m_vertexIndexByEdge.reserve(estimatedVertices);
}

template<MeshScalar TScalar>
std::uint32_t MeshBuilder<TScalar>::addVertexForEdge(const EdgeKey& edgeKey, const Vec3<TScalar>& position)
{
    const auto existing = m_vertexIndexByEdge.find(edgeKey);
    if (existing != m_vertexIndexByEdge.end())
    {
        return existing->second;
    }

    const std::uint32_t vertexIndex = static_cast<std::uint32_t>(m_vertices.size());
    m_vertices.push_back(position);
    m_normals.push_back({});
    m_vertexIndexByEdge.emplace(edgeKey, vertexIndex);
    return vertexIndex;
}

template<MeshScalar TScalar>
void MeshBuilder<TScalar>::addTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2)
{
    if (i0 == i1 || i0 == i2 || i1 == i2)
    {
        return;
    }

    m_triangles.push_back({i0, i1, i2});

    const Vec3<TScalar>& v0 = m_vertices[static_cast<std::size_t>(i0)];
    const Vec3<TScalar>& v1 = m_vertices[static_cast<std::size_t>(i1)];
    const Vec3<TScalar>& v2 = m_vertices[static_cast<std::size_t>(i2)];
    const Vec3<TScalar> faceNormal = Math3D::normalize(
        Math3D::cross(Math3D::subtract(v1, v0), Math3D::subtract(v2, v0)));

    m_normals[static_cast<std::size_t>(i0)].x += faceNormal.x;
    m_normals[static_cast<std::size_t>(i0)].y += faceNormal.y;
    m_normals[static_cast<std::size_t>(i0)].z += faceNormal.z;
    m_normals[static_cast<std::size_t>(i1)].x += faceNormal.x;
    m_normals[static_cast<std::size_t>(i1)].y += faceNormal.y;
    m_normals[static_cast<std::size_t>(i1)].z += faceNormal.z;
    m_normals[static_cast<std::size_t>(i2)].x += faceNormal.x;
    m_normals[static_cast<std::size_t>(i2)].y += faceNormal.y;
    m_normals[static_cast<std::size_t>(i2)].z += faceNormal.z;
}

template<MeshScalar TScalar>
MeshData<TScalar> MeshBuilder<TScalar>::build() &&
{
    for (Vec3<TScalar>& normal : m_normals)
    {
        normal = Math3D::normalize(normal);
    }

    return MeshData<TScalar>(std::move(m_vertices), std::move(m_triangles), std::move(m_normals));
}

template<MeshScalar TScalar>
std::size_t MeshBuilder<TScalar>::EdgeKeyHash::operator()(const EdgeKey& key) const noexcept
{
    std::size_t seed = 0;
    const auto combine = [&seed](int value) {
        const std::size_t hashed = std::hash<int>{}(value);
        seed ^= hashed + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    };

    combine(key.x0);
    combine(key.y0);
    combine(key.z0);
    combine(key.x1);
    combine(key.y1);
    combine(key.z1);
    return seed;
}
