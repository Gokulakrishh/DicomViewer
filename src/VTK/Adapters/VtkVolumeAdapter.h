#pragma once

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

class IVolumeData;

class VtkVolumeAdapter
{
public:
    [[nodiscard]] static vtkSmartPointer<vtkImageData> createImageData(const IVolumeData& diagnosticVolume);
};
