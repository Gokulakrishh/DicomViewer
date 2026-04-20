#include "VTK/Presets/VtkVolumeRenderPreset.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

void applyColorPoints(vtkColorTransferFunction& transferFunction, const std::vector<VtkTransferFunctionPoint>& points)
{
    transferFunction.RemoveAllPoints();
    for (const VtkTransferFunctionPoint& point : points)
    {
        transferFunction.AddRGBPoint(point.scalar, point.valueA, point.valueB, point.valueC);
    }
}

void applyScalarPoints(vtkPiecewiseFunction& transferFunction, const std::vector<VtkTransferFunctionPoint>& points)
{
    transferFunction.RemoveAllPoints();
    for (const VtkTransferFunctionPoint& point : points)
    {
        transferFunction.AddPoint(point.scalar, point.valueA);
    }
}
