#pragma once

#include "Services/IMeshExtractionStrategy.h"

#include "Model/MeshBuilder.h"
#include "Model/MeshData.h"
#include "Model/VolumeGeometry.h"

#include <array>
#include <memory>
#include <vector>

struct MarchingCubesParameters
{
    double isoValue{0.5};
};

class MarchingCubesMeshExtractionStrategy final : public IMeshExtractionStrategy
{
public:
    explicit MarchingCubesMeshExtractionStrategy(MarchingCubesParameters parameters = {});

    [[nodiscard]] std::shared_ptr<IMeshData> extract(const ISegmentationMask& mask) const override;
    [[nodiscard]] const MarchingCubesParameters& parameters() const;

private:
    struct Index3D
    {
        int x{0};
        int y{0};
        int z{0};
    };

    struct ScalarPoint
    {
        Vec3<float> position;
        float value{0.0F};
        Index3D gridIndex;
    };

    [[nodiscard]] static std::shared_ptr<IMeshData> buildMeshFromMask(const ISegmentationMask& mask, float isoValue);
    [[nodiscard]] static Vec3<float> indexToWorld(const VolumeGeometry& geometry, int x, int y, int z);
    [[nodiscard]] static std::array<ScalarPoint, 8> sampleCube(const ISegmentationMask& mask, int x, int y, int z);
    [[nodiscard]] static MeshBuilder<float>::EdgeKey makeEdgeKey(const ScalarPoint& a, const ScalarPoint& b);
    [[nodiscard]] static Vec3<float> interpolateEdge(const ScalarPoint& a, const ScalarPoint& b, float isoValue);
    static void appendTriangle(
        const ScalarPoint& e0a,
        const ScalarPoint& e0b,
        const ScalarPoint& e1a,
        const ScalarPoint& e1b,
        const ScalarPoint& e2a,
        const ScalarPoint& e2b,
        float isoValue,
        MeshBuilder<float>& meshBuilder);
    static void polygoniseTetrahedron(
        const std::array<ScalarPoint, 4>& tetrahedron,
        float isoValue,
        MeshBuilder<float>& meshBuilder);
    static void polygoniseCube(
        const std::array<ScalarPoint, 8>& cube,
        float isoValue,
        MeshBuilder<float>& meshBuilder);

private:
    static constexpr std::array<std::array<int, 3>, 8> kCubeCornerOffsets{{
        {{0, 0, 0}},
        {{1, 0, 0}},
        {{1, 1, 0}},
        {{0, 1, 0}},
        {{0, 0, 1}},
        {{1, 0, 1}},
        {{1, 1, 1}},
        {{0, 1, 1}},
    }};

    static constexpr std::array<std::array<int, 4>, 6> kTetrahedraInCube{{
        {{0, 5, 1, 6}},
        {{0, 1, 2, 6}},
        {{0, 2, 3, 6}},
        {{0, 3, 7, 6}},
        {{0, 7, 4, 6}},
        {{0, 4, 5, 6}},
    }};

    MarchingCubesParameters m_parameters;
};
