#pragma once

#include <vector>

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

/**
 * @brief Prepared volume input selected by a VTK volume render preset.
 */
enum class VtkVolumeInputKind
{
    BaseVolume = 0,
    BoneFocusedVolume = 1
};

/**
 * @brief One transfer-function control point.
 */
struct VtkTransferFunctionPoint
{
    double scalar{0.0};
    double valueA{0.0};
    double valueB{0.0};
    double valueC{0.0};
};

/**
 * @brief VTK volume rendering preset data.
 */
struct VtkVolumeRenderPreset
{
    VtkVolumeInputKind inputKind{VtkVolumeInputKind::BaseVolume};
    std::vector<VtkTransferFunctionPoint> colorPoints;
    std::vector<VtkTransferFunctionPoint> scalarOpacityPoints;
    std::vector<VtkTransferFunctionPoint> gradientOpacityPoints;
};

/** @brief Applies color transfer-function points. */
void applyColorPoints(vtkColorTransferFunction& transferFunction, const std::vector<VtkTransferFunctionPoint>& points);
/** @brief Applies scalar/opacity transfer-function points. */
void applyScalarPoints(vtkPiecewiseFunction& transferFunction, const std::vector<VtkTransferFunctionPoint>& points);
