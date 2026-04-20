#pragma once

#include <vector>

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

enum class VtkVolumeInputKind
{
    BaseVolume = 0,
    BoneFocusedVolume = 1
};

struct VtkTransferFunctionPoint
{
    double scalar{0.0};
    double valueA{0.0};
    double valueB{0.0};
    double valueC{0.0};
};

struct VtkVolumeRenderPreset
{
    VtkVolumeInputKind inputKind{VtkVolumeInputKind::BaseVolume};
    std::vector<VtkTransferFunctionPoint> colorPoints;
    std::vector<VtkTransferFunctionPoint> scalarOpacityPoints;
    std::vector<VtkTransferFunctionPoint> gradientOpacityPoints;
};

void applyColorPoints(vtkColorTransferFunction& transferFunction, const std::vector<VtkTransferFunctionPoint>& points);
void applyScalarPoints(vtkPiecewiseFunction& transferFunction, const std::vector<VtkTransferFunctionPoint>& points);
