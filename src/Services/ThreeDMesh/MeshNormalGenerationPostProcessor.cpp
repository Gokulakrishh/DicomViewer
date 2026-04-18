#include "Services/ThreeDMesh/MeshNormalGenerationPostProcessor.h"

#include "Model/IMeshData.h"
#include "Model/MeshData.h"
#include "Utilities/Math3D.h"

#include <memory>
#include <vector>

std::shared_ptr<IMeshData> MeshNormalGenerationPostProcessor::process(const IMeshData& mesh) const
{
    std::vector<Vec3<float>> vertices;
    vertices.reserve(mesh.vertexCount());

    for (std::size_t vertexIndex = 0; vertexIndex < mesh.vertexCount(); ++vertexIndex)
    {
        const auto position = mesh.vertexPosition(vertexIndex);
        vertices.push_back({
            static_cast<float>(position[0]),
            static_cast<float>(position[1]),
            static_cast<float>(position[2])});
    }

    std::vector<TriangleIndices> triangles;
    triangles.reserve(mesh.triangleCount());

    for (std::size_t triangleIndex = 0; triangleIndex < mesh.triangleCount(); ++triangleIndex)
    {
        const auto triangle = mesh.triangleIndices(triangleIndex);
        triangles.push_back({triangle[0], triangle[1], triangle[2]});
    }

    std::vector<Vec3<float>> normals(vertices.size(), Vec3<float>{});
    for (const TriangleIndices& triangle : triangles)
    {
        const Vec3<float>& v0 = vertices[static_cast<std::size_t>(triangle.i0)];
        const Vec3<float>& v1 = vertices[static_cast<std::size_t>(triangle.i1)];
        const Vec3<float>& v2 = vertices[static_cast<std::size_t>(triangle.i2)];
        const Vec3<float> faceNormal = Math3D::normalize(
            Math3D::cross(Math3D::subtract(v1, v0), Math3D::subtract(v2, v0)));

        normals[static_cast<std::size_t>(triangle.i0)].x += faceNormal.x;
        normals[static_cast<std::size_t>(triangle.i0)].y += faceNormal.y;
        normals[static_cast<std::size_t>(triangle.i0)].z += faceNormal.z;
        normals[static_cast<std::size_t>(triangle.i1)].x += faceNormal.x;
        normals[static_cast<std::size_t>(triangle.i1)].y += faceNormal.y;
        normals[static_cast<std::size_t>(triangle.i1)].z += faceNormal.z;
        normals[static_cast<std::size_t>(triangle.i2)].x += faceNormal.x;
        normals[static_cast<std::size_t>(triangle.i2)].y += faceNormal.y;
        normals[static_cast<std::size_t>(triangle.i2)].z += faceNormal.z;
    }

    for (Vec3<float>& normal : normals)
    {
        normal = Math3D::normalize(normal);
    }

    return std::make_shared<MeshData<float>>(std::move(vertices), std::move(triangles), std::move(normals));
}
