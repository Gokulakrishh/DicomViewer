#include "VTK/Adapters/VtkVolumeAdapter.h"

#include "Model/IVolumeData.h"
#include "Model/VolumeData.h"

#include <vtkImageData.h>
#include <vtkMatrix3x3.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkShortArray.h>

namespace
{
vtkNew<vtkMatrix3x3> createDirectionMatrix(const VolumeGeometry& geometry)
{
    vtkNew<vtkMatrix3x3> directionMatrix;
    directionMatrix->SetElement(0, 0, geometry.direction[0]);
    directionMatrix->SetElement(0, 1, geometry.direction[1]);
    directionMatrix->SetElement(0, 2, geometry.direction[2]);
    directionMatrix->SetElement(1, 0, geometry.direction[3]);
    directionMatrix->SetElement(1, 1, geometry.direction[4]);
    directionMatrix->SetElement(1, 2, geometry.direction[5]);
    directionMatrix->SetElement(2, 0, geometry.direction[6]);
    directionMatrix->SetElement(2, 1, geometry.direction[7]);
    directionMatrix->SetElement(2, 2, geometry.direction[8]);
    return directionMatrix;
}

vtkSmartPointer<vtkImageData> createEmptyImageData(const VolumeGeometry& geometry)
{
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(geometry.dimensions.x, geometry.dimensions.y, geometry.dimensions.z);
    imageData->SetSpacing(geometry.spacing.x, geometry.spacing.y, geometry.spacing.z);
    imageData->SetOrigin(geometry.origin.x, geometry.origin.y, geometry.origin.z);
    imageData->SetDirectionMatrix(createDirectionMatrix(geometry));
    imageData->AllocateScalars(VTK_SHORT, 1);
    return imageData;
}

vtkSmartPointer<vtkImageData> importImageData(const VolumeData<int16_t>& diagnosticVolume)
{
    const VolumeGeometry& geometry = diagnosticVolume.geometry();
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(geometry.dimensions.x, geometry.dimensions.y, geometry.dimensions.z);
    imageData->SetSpacing(geometry.spacing.x, geometry.spacing.y, geometry.spacing.z);
    imageData->SetOrigin(geometry.origin.x, geometry.origin.y, geometry.origin.z);
    imageData->SetDirectionMatrix(createDirectionMatrix(geometry));

    vtkNew<vtkShortArray> scalars;
    scalars->SetNumberOfComponents(1);
    scalars->SetArray(
        const_cast<short*>(diagnosticVolume.voxels().data()),
        static_cast<vtkIdType>(diagnosticVolume.voxels().size()),
        1);
    imageData->GetPointData()->SetScalars(scalars);
    imageData->Modified();
    return imageData;
}
}

vtkSmartPointer<vtkImageData> VtkVolumeAdapter::createImageData(const IVolumeData& diagnosticVolume)
{
    if (const auto* typedVolume = dynamic_cast<const VolumeData<int16_t>*>(&diagnosticVolume))
    {
        return importImageData(*typedVolume);
    }

    const VolumeGeometry& geometry = diagnosticVolume.geometry();
    vtkSmartPointer<vtkImageData> imageData = createEmptyImageData(geometry);

    for (int z = 0; z < geometry.dimensions.z; ++z)
    {
        for (int y = 0; y < geometry.dimensions.y; ++y)
        {
            for (int x = 0; x < geometry.dimensions.x; ++x)
            {
                auto* scalar = static_cast<short*>(imageData->GetScalarPointer(x, y, z));
                scalar[0] = static_cast<short>(diagnosticVolume.scalarAt(x, y, z));
            }
        }
    }

    imageData->Modified();
    return imageData;
}
