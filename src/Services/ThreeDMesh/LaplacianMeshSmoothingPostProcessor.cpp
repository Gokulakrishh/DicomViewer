#include "Services/ThreeDMesh/LaplacianMeshSmoothingPostProcessor.h"

#include "Model/IMeshData.h"
#include "Model/MeshData.h"
#include "Utilities/Math3D.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

LaplacianMeshSmoothingPostProcessor::LaplacianMeshSmoothingPostProcessor(
    LaplacianMeshSmoothingParameters parameters)
    : m_parameters(parameters)
{
}

std::shared_ptr<IMeshData> LaplacianMeshSmoothingPostProcessor::process(const IMeshData& mesh) const
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

    std::vector<std::unordered_set<std::uint32_t>> adjacency(vertices.size());
    for (const TriangleIndices& triangle : triangles)
    {
        adjacency[static_cast<std::size_t>(triangle.i0)].insert(triangle.i1);
        adjacency[static_cast<std::size_t>(triangle.i0)].insert(triangle.i2);
        adjacency[static_cast<std::size_t>(triangle.i1)].insert(triangle.i0);
        adjacency[static_cast<std::size_t>(triangle.i1)].insert(triangle.i2);
        adjacency[static_cast<std::size_t>(triangle.i2)].insert(triangle.i0);
        adjacency[static_cast<std::size_t>(triangle.i2)].insert(triangle.i1);
    }

    const int iterationCount = std::max(0, m_parameters.iterations);
    const float lambda = std::clamp(m_parameters.lambda, 0.0F, 1.0F);

    // Keep smoothing single-threaded until the full pipeline is timed. Later,
    // vertex updates can be parallelized per iteration because each output
    // position reads only the previous iteration state.
    for (int iteration = 0; iteration < iterationCount; ++iteration)
    {
        std::vector<Vec3<float>> updatedVertices = vertices;
        for (std::size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
        {
            const auto& neighbors = adjacency[vertexIndex];
            if (neighbors.empty())
            {
                continue;
            }

            Vec3<float> neighborAverage{};
            for (std::uint32_t neighborIndex : neighbors)
            {
                neighborAverage = Math3D::add(neighborAverage, vertices[static_cast<std::size_t>(neighborIndex)]);
            }

            const float inverseCount = 1.0F / static_cast<float>(neighbors.size());
            neighborAverage = Math3D::multiply(neighborAverage, inverseCount);
            updatedVertices[vertexIndex] = Math3D::add(
                vertices[vertexIndex],
                Math3D::multiply(Math3D::subtract(neighborAverage, vertices[vertexIndex]), lambda));
        }

        vertices = std::move(updatedVertices);
    }

    return std::make_shared<MeshData<float>>(std::move(vertices), std::move(triangles));
}

const LaplacianMeshSmoothingParameters& LaplacianMeshSmoothingPostProcessor::parameters() const
{
    return m_parameters;
}
