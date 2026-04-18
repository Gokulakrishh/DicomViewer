#include "Services/ThreeDMesh/MarchingCubesMeshExtractionStrategy.h"

#include "Model/ISegmentationMask.h"
#include "Model/VolumeGeometry.h"
#include "Utilities/Math3D.h"

#include <cmath>
#include <memory>
MarchingCubesMeshExtractionStrategy::MarchingCubesMeshExtractionStrategy(MarchingCubesParameters parameters)
    : m_parameters(parameters)
{
}

std::shared_ptr<IMeshData> MarchingCubesMeshExtractionStrategy::extract(const ISegmentationMask& mask) const
{
    return buildMeshFromMask(mask, static_cast<float>(m_parameters.isoValue));
}

const MarchingCubesParameters& MarchingCubesMeshExtractionStrategy::parameters() const
{
    return m_parameters;
}

Vec3<float> MarchingCubesMeshExtractionStrategy::indexToWorld(const VolumeGeometry& geometry, int x, int y, int z)
{
    const double ix = static_cast<double>(x) * geometry.spacing.x;
    const double iy = static_cast<double>(y) * geometry.spacing.y;
    const double iz = static_cast<double>(z) * geometry.spacing.z;

    const auto& d = geometry.direction;
    const double wx = geometry.origin.x + (d[0] * ix) + (d[3] * iy) + (d[6] * iz);
    const double wy = geometry.origin.y + (d[1] * ix) + (d[4] * iy) + (d[7] * iz);
    const double wz = geometry.origin.z + (d[2] * ix) + (d[5] * iy) + (d[8] * iz);
    return {static_cast<float>(wx), static_cast<float>(wy), static_cast<float>(wz)};
}

std::array<MarchingCubesMeshExtractionStrategy::ScalarPoint, 8> MarchingCubesMeshExtractionStrategy::sampleCube(
    const ISegmentationMask& mask,
    int x,
    int y,
    int z)
{
    std::array<ScalarPoint, 8> cube{};
    const VolumeGeometry& geometry = mask.geometry();

    for (std::size_t i = 0; i < kCubeCornerOffsets.size(); ++i)
    {
        const auto& offset = kCubeCornerOffsets[i];
        const int sx = x + offset[0];
        const int sy = y + offset[1];
        const int sz = z + offset[2];
        cube[i] = {
            indexToWorld(geometry, sx, sy, sz),
            mask.isForeground(sx, sy, sz) ? 1.0F : 0.0F,
            {sx, sy, sz}};
    }

    return cube;
}

MeshBuilder<float>::EdgeKey MarchingCubesMeshExtractionStrategy::makeEdgeKey(const ScalarPoint& a, const ScalarPoint& b)
{
    const bool swapNeeded =
        (a.gridIndex.x > b.gridIndex.x) ||
        (a.gridIndex.x == b.gridIndex.x && a.gridIndex.y > b.gridIndex.y) ||
        (a.gridIndex.x == b.gridIndex.x && a.gridIndex.y == b.gridIndex.y && a.gridIndex.z > b.gridIndex.z);

    const ScalarPoint& first = swapNeeded ? b : a;
    const ScalarPoint& second = swapNeeded ? a : b;

    return {
        first.gridIndex.x, first.gridIndex.y, first.gridIndex.z,
        second.gridIndex.x, second.gridIndex.y, second.gridIndex.z};
}

Vec3<float> MarchingCubesMeshExtractionStrategy::interpolateEdge(
    const ScalarPoint& a,
    const ScalarPoint& b,
    float isoValue)
{
    const float delta = b.value - a.value;
    if (std::abs(delta) < 1e-6F)
    {
        return Math3D::multiply(Math3D::add(a.position, b.position), 0.5F);
    }

    const float t = (isoValue - a.value) / delta;
    const Vec3<float> direction = Math3D::subtract(b.position, a.position);
    return Math3D::add(a.position, Math3D::multiply(direction, t));
}

void MarchingCubesMeshExtractionStrategy::appendTriangle(
    const ScalarPoint& e0a,
    const ScalarPoint& e0b,
    const ScalarPoint& e1a,
    const ScalarPoint& e1b,
    const ScalarPoint& e2a,
    const ScalarPoint& e2b,
    float isoValue,
    MeshBuilder<float>& meshBuilder)
{
    const std::uint32_t i0 = meshBuilder.addVertexForEdge(makeEdgeKey(e0a, e0b), interpolateEdge(e0a, e0b, isoValue));
    const std::uint32_t i1 = meshBuilder.addVertexForEdge(makeEdgeKey(e1a, e1b), interpolateEdge(e1a, e1b, isoValue));
    const std::uint32_t i2 = meshBuilder.addVertexForEdge(makeEdgeKey(e2a, e2b), interpolateEdge(e2a, e2b, isoValue));
    meshBuilder.addTriangle(i0, i1, i2);
}

void MarchingCubesMeshExtractionStrategy::polygoniseTetrahedron(
    const std::array<ScalarPoint, 4>& tetrahedron,
    float isoValue,
    MeshBuilder<float>& meshBuilder)
{
    std::array<int, 4> insideIndices{};
    std::array<int, 4> outsideIndices{};
    int insideCount = 0;
    int outsideCount = 0;

    for (int i = 0; i < 4; ++i)
    {
        if (tetrahedron[static_cast<std::size_t>(i)].value >= isoValue)
        {
            insideIndices[static_cast<std::size_t>(insideCount++)] = i;
        }
        else
        {
            outsideIndices[static_cast<std::size_t>(outsideCount++)] = i;
        }
    }

    if (insideCount == 0 || insideCount == 4)
    {
        return;
    }

    if (insideCount == 1 || insideCount == 3)
    {
        const bool invert = insideCount == 3;
        const int apexIndex = invert ? outsideIndices[0] : insideIndices[0];
        const auto& apex = tetrahedron[static_cast<std::size_t>(apexIndex)];
        const auto& s0 = tetrahedron[static_cast<std::size_t>(invert ? insideIndices[0] : outsideIndices[0])];
        const auto& s1 = tetrahedron[static_cast<std::size_t>(invert ? insideIndices[1] : outsideIndices[1])];
        const auto& s2 = tetrahedron[static_cast<std::size_t>(invert ? insideIndices[2] : outsideIndices[2])];

        if (invert)
        {
            appendTriangle(apex, s0, apex, s2, apex, s1, isoValue, meshBuilder);
        }
        else
        {
            appendTriangle(apex, s0, apex, s1, apex, s2, isoValue, meshBuilder);
        }
        return;
    }

    const auto& i0 = tetrahedron[static_cast<std::size_t>(insideIndices[0])];
    const auto& i1 = tetrahedron[static_cast<std::size_t>(insideIndices[1])];
    const auto& o0 = tetrahedron[static_cast<std::size_t>(outsideIndices[0])];
    const auto& o1 = tetrahedron[static_cast<std::size_t>(outsideIndices[1])];

    appendTriangle(i0, o0, i0, o1, i1, o0, isoValue, meshBuilder);
    appendTriangle(i0, o1, i1, o1, i1, o0, isoValue, meshBuilder);
}

void MarchingCubesMeshExtractionStrategy::polygoniseCube(
    const std::array<ScalarPoint, 8>& cube,
    float isoValue,
    MeshBuilder<float>& meshBuilder)
{
    for (const auto& tetrahedronIndices : kTetrahedraInCube)
    {
        std::array<ScalarPoint, 4> tetrahedron{
            cube[static_cast<std::size_t>(tetrahedronIndices[0])],
            cube[static_cast<std::size_t>(tetrahedronIndices[1])],
            cube[static_cast<std::size_t>(tetrahedronIndices[2])],
            cube[static_cast<std::size_t>(tetrahedronIndices[3])]};
        polygoniseTetrahedron(tetrahedron, isoValue, meshBuilder);
    }
}

std::shared_ptr<IMeshData> MarchingCubesMeshExtractionStrategy::buildMeshFromMask(
    const ISegmentationMask& mask,
    float isoValue)
{
    const VolumeGeometry& geometry = mask.geometry();
    if (geometry.dimensions.x < 2 || geometry.dimensions.y < 2 || geometry.dimensions.z < 2)
    {
        return std::make_shared<MeshData<float>>();
    }

    MeshBuilder<float> meshBuilder;
    meshBuilder.reserve(
        static_cast<std::size_t>(geometry.dimensions.x * geometry.dimensions.y),
        static_cast<std::size_t>(geometry.dimensions.x * geometry.dimensions.y));

    // Keep extraction single-threaded until the full pipeline is finished and timed.
    // Later parallelization should run independent local builders per z-slab or block,
    // then merge finished mesh buffers instead of sharing one mutable builder.
    for (int z = 0; z < geometry.dimensions.z - 1; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y - 1; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x - 1; ++x)
            {
                const auto cube = sampleCube(mask, x, y, z);
                polygoniseCube(cube, isoValue, meshBuilder);
            }
        }
    }

    return std::make_shared<MeshData<float>>(std::move(meshBuilder).build());
}
