#pragma once

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

class IVolumeData;

/**
 * @brief Adapter that converts application volume data into vtkImageData.
 *
 * Responsibilities:
 * - Copy scalar voxels and geometry into a VTK-compatible image object.
 * - Keep VTK data structures outside core volume services.
 */
class VtkVolumeAdapter
{
public:
    /**
     * @brief Creates vtkImageData from diagnostic volume data.
     * @param diagnosticVolume Source volume.
     * @return VTK image data.
     */
    [[nodiscard]] static vtkSmartPointer<vtkImageData> createImageData(const IVolumeData& diagnosticVolume);
};
