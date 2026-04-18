#include "Model/DicomImage.h"
#include "Model/DicomParameters.h"
#include "Model/VolumeData.h"
#include "Services/ThreeDProfiles/Bone3dPipelineProfile.h"
#include "Services/ThreeDProfiles/Lung3dPipelineProfile.h"
#include "Services/ThreeDSeriesBuildService.h"
#include "Services/ThreeDimensionalPipelineService.h"

#include <QCoreApplication>
#include <QDebug>
#include <QVector>

#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

namespace
{
std::shared_ptr<IVolumeData> buildSyntheticSphereVolume()
{
    VolumeGeometry geometry;
    geometry.dimensions = {64, 64, 64};
    geometry.spacing = {1.0, 1.0, 1.0};
    geometry.origin = {-32.0, -32.0, -32.0};

    std::vector<std::int16_t> voxels(static_cast<std::size_t>(geometry.voxelCount()), std::int16_t{0});
    const double centerX = 31.5;
    const double centerY = 31.5;
    const double centerZ = 31.5;
    const double radius = 18.0;

    int flatIndex = 0;
    for (int z = 0; z < geometry.dimensions.z; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x; ++x)
            {
                const double dx = static_cast<double>(x) - centerX;
                const double dy = static_cast<double>(y) - centerY;
                const double dz = static_cast<double>(z) - centerZ;
                const double distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
                voxels[static_cast<std::size_t>(flatIndex++)] = distance <= radius ? std::int16_t{1000} : std::int16_t{0};
            }
        }
    }

    return std::make_shared<VolumeData<std::int16_t>>(geometry, std::move(voxels));
}

Series buildSyntheticSphereSeries()
{
    Series series;
    series.setSeriesInstanceUid("synthetic-sphere-series");
    series.setSeriesDescription("Synthetic Sphere");
    series.setModality("CT");
    series.setSeriesNumber("1");
    series.setImageCount(64);
    series.setRepresentativeFilePath("synthetic://sphere");

    const int width = 64;
    const int height = 64;
    const int depth = 64;
    const double centerX = 31.5;
    const double centerY = 31.5;
    const double centerZ = 31.5;
    const double radius = 18.0;

    for (int z = 0; z < depth; ++z)
    {
        auto image = std::make_unique<DicomImage>();
        image->setSopInstanceUid(QStringLiteral("synthetic-sop-%1").arg(z));
        image->setInstanceNumber(QString::number(z + 1));
        image->setDimensions(width, height);
        image->setMonochrome(true);
        image->setPixelSpacing(1.0, 1.0);
        image->setSliceThickness(1.0);
        image->setSpacingBetweenSlices(1.0);
        image->setImageOrientationPatient({1.0, 0.0, 0.0, 0.0, 1.0, 0.0});
        image->setImagePositionPatient({-32.0, -32.0, -32.0 + static_cast<double>(z)});

        QVector<int> rawPixels;
        rawPixels.resize(width * height);

        int flatIndex = 0;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const double dx = static_cast<double>(x) - centerX;
                const double dy = static_cast<double>(y) - centerY;
                const double dz = static_cast<double>(z) - centerZ;
                const double distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
                rawPixels[flatIndex++] = distance <= radius ? 1000 : 0;
            }
        }

        image->setRawPixels(rawPixels);
        series.addImage(std::move(image));
    }

    return series;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    try
    {
        const std::shared_ptr<IVolumeData> volume = buildSyntheticSphereVolume();

        Bone3dPipelineProfileParameters profileParameters;
        profileParameters.segmentation = {500.0, 1500.0, true, true};
        profileParameters.enableMeshSmoothing = false;

        Bone3dPipelineProfile profile(profileParameters);
        ThreeDimensionalPipelineService pipelineService;
        const ThreeDimensionalPipelineResult result = pipelineService.buildMesh(*volume, profile);

        if (!result.isValid())
        {
            qCritical() << "3D harness failed: invalid pipeline result";
            return 1;
        }

        qInfo() << "3D pipeline harness completed";
        qInfo() << "Profile:" << QString::fromUtf8(result.diagnostics.profileName.c_str());
        qInfo() << "Foreground voxels:" << result.diagnostics.foregroundVoxelCount;
        qInfo() << "Mesh vertices:" << static_cast<qulonglong>(result.diagnostics.meshVertexCount);
        qInfo() << "Mesh triangles:" << static_cast<qulonglong>(result.diagnostics.meshTriangleCount);

        Lung3dPipelineProfile lungProfile;
        const ThreeDimensionalPipelineResult lungResult = pipelineService.buildMesh(*volume, lungProfile);
        qInfo() << "Lung profile mesh vertices:" << static_cast<qulonglong>(lungResult.diagnostics.meshVertexCount);
        qInfo() << "Lung profile mesh triangles:" << static_cast<qulonglong>(lungResult.diagnostics.meshTriangleCount);

        const Series syntheticSeries = buildSyntheticSphereSeries();
        ThreeDSeriesBuildService seriesBuildService;
        const ThreeDimensionalPipelineResult seriesResult =
            seriesBuildService.buildFromDiagnosticSeries(syntheticSeries, profile);
        qInfo() << "Series entry mesh vertices:" << static_cast<qulonglong>(seriesResult.diagnostics.meshVertexCount);
        qInfo() << "Series entry mesh triangles:" << static_cast<qulonglong>(seriesResult.diagnostics.meshTriangleCount);
        return 0;
    }
    catch (const std::exception& exception)
    {
        qCritical() << "3D pipeline harness exception:" << exception.what();
        return 1;
    }
}
