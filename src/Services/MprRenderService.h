#pragma once

#include "Model/IVolumeData.h"

#include <QImage>

class MprRenderService
{
public:
    enum class Plane
    {
        Axial,
        Coronal,
        Sagittal
    };

    QImage renderSlice(
        const IVolumeData& volume,
        Plane plane,
        int sliceIndex,
        int windowLevel,
        int windowWidth) const;
};
