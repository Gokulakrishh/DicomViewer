#include "Services/CompositeMeshPostProcessor.h"

#include "Model/IMeshData.h"
#include "Model/MeshData.h"

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
std::shared_ptr<IMeshData> cloneMesh(const IMeshData& mesh)
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

    std::vector<Vec3<float>> normals;
    if (mesh.hasVertexNormals())
    {
        normals.reserve(mesh.vertexCount());
        for (std::size_t vertexIndex = 0; vertexIndex < mesh.vertexCount(); ++vertexIndex)
        {
            const auto normal = mesh.vertexNormal(vertexIndex);
            normals.push_back({
                static_cast<float>(normal[0]),
                static_cast<float>(normal[1]),
                static_cast<float>(normal[2])});
        }
    }

    return std::make_shared<MeshData<float>>(std::move(vertices), std::move(triangles), std::move(normals));
}
}

CompositeMeshPostProcessor::CompositeMeshPostProcessor(std::vector<std::shared_ptr<IMeshPostProcessor>> processors)
    : m_processors(std::move(processors))
{
}

std::shared_ptr<IMeshData> CompositeMeshPostProcessor::process(const IMeshData& mesh) const
{
    std::shared_ptr<IMeshData> currentMesh = cloneMesh(mesh);

    for (const std::shared_ptr<IMeshPostProcessor>& processor : m_processors)
    {
        if (!processor)
        {
            continue;
        }

        currentMesh = processor->process(*currentMesh);
        if (!currentMesh)
        {
            throw std::runtime_error("Mesh post-processor returned a null mesh");
        }
    }

    return currentMesh;
}

const std::vector<std::shared_ptr<IMeshPostProcessor>>& CompositeMeshPostProcessor::processors() const
{
    return m_processors;
}
