#include "MprRenderService.h"

#include <algorithm>

QImage MprRenderService::renderSlice(
    const IVolumeData& volume,
    Plane plane,
    int sliceIndex,
    int windowLevel,
    int windowWidth) const
{
    const VolumeGeometry& geometry = volume.geometry();
    if (!geometry.isValid())
    {
        return {};
    }

    int width = 0;
    int height = 0;
    switch (plane)
    {
    case Plane::Axial:
        width = geometry.dimensions.x;
        height = geometry.dimensions.y;
        sliceIndex = std::clamp(sliceIndex, 0, geometry.dimensions.z - 1);
        break;
    case Plane::Coronal:
        width = geometry.dimensions.x;
        height = geometry.dimensions.z;
        sliceIndex = std::clamp(sliceIndex, 0, geometry.dimensions.y - 1);
        break;
    case Plane::Sagittal:
        width = geometry.dimensions.y;
        height = geometry.dimensions.z;
        sliceIndex = std::clamp(sliceIndex, 0, geometry.dimensions.x - 1);
        break;
    }

    QImage image(width, height, QImage::Format_Grayscale8);
    const double widthValue = std::max(1, windowWidth);
    const double lowerBound = static_cast<double>(windowLevel) - (widthValue / 2.0);

    for (int y = 0; y < height; ++y)
    {
        uchar* scanLine = image.scanLine(y);
        for (int x = 0; x < width; ++x)
        {
            double value = 0.0;
            const int displayZ = height - 1 - y;
            switch (plane)
            {
            case Plane::Axial:
                value = volume.scalarAt(x, y, sliceIndex);
                break;
            case Plane::Coronal:
                value = volume.scalarAt(x, sliceIndex, displayZ);
                break;
            case Plane::Sagittal:
                value = volume.scalarAt(sliceIndex, x, displayZ);
                break;
            }

            const int scaledValue = std::clamp(
                static_cast<int>(((value - lowerBound) / widthValue) * 255.0),
                0,
                255);
            scanLine[x] = static_cast<uchar>(scaledValue);
        }
    }

    return image;
}
