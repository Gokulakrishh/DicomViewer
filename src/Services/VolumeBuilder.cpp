#include "VolumeBuilder.h"

#include "Errors/AppError.h"
#include "Model/DicomImage.h"
#include "Model/DicomParameters.h"
#include "Model/VolumeData.h"

#include <QDebug>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
AppError makeVolumeBuildError(ErrorCode code, const QString& technicalMessage, const QString& userMessage)
{
    return AppError{
        code,
        ErrorSeverity::Recoverable,
        "Volume Builder",
        technicalMessage,
        userMessage};
}

void appendGeometryWarning(QStringList& warnings, const QString& message)
{
    if (!warnings.contains(message))
    {
        warnings.append(message);
    }

    qWarning().noquote() << "[VolumeBuilder]" << message;
}

QString imageIdentity(const DicomImage& image)
{
    QStringList parts;
    if (!image.sopInstanceUid().isEmpty())
    {
        parts << QString("SOP=%1").arg(image.sopInstanceUid());
    }
    if (!image.instanceNumber().isEmpty())
    {
        parts << QString("Instance=%1").arg(image.instanceNumber());
    }
    if (image.frameCount() > 1)
    {
        parts << QString("Frame=%1").arg(image.frameIndex() + 1);
    }

    return parts.isEmpty() ? QString("affected slice") : parts.join(", ");
}
}

VolumeBuilder::VolumeBuilder(VolumeValidationSettings validationSettings)
    : m_validationSettings(std::move(validationSettings))
{
}

VolumeBuilder::VolumeInput VolumeBuilder::collectVolumeInput(const Series& series)
{
    const auto& images = series.images();
    if (images.empty() || !images.front())
    {
        return {};
    }

    const DicomImage& firstImage = *images.front();
    if (!firstImage.hasRawPixels() || firstImage.width() <= 0 || firstImage.height() <= 0)
    {
        return {};
    }

    VolumeInput input;
    input.firstImage = &firstImage;
    input.width = firstImage.width();
    input.height = firstImage.height();
    input.depth = static_cast<int>(images.size());
    input.orderedImages.reserve(images.size());

    for (const auto& imagePtr : images)
    {
        if (!imagePtr || !imagePtr->hasRawPixels() ||
            imagePtr->width() != input.width || imagePtr->height() != input.height)
        {
            throw makeVolumeBuildError(
                ErrorCode::VolumeBuildInconsistentGeometry,
                "Series cannot be converted into a consistent volume",
                "The selected series cannot be converted into a consistent diagnostic volume.");
        }

        input.orderedImages.push_back(imagePtr.get());
    }

    return input;
}

std::array<double, 3> VolumeBuilder::normalizeVector(const std::array<double, 3>& vector)
{
    const double magnitude = std::sqrt(
        (vector[0] * vector[0]) +
        (vector[1] * vector[1]) +
        (vector[2] * vector[2]));
    if (magnitude <= 0.0)
    {
        return {0.0, 0.0, 0.0};
    }

    return {
        vector[0] / magnitude,
        vector[1] / magnitude,
        vector[2] / magnitude};
}

double VolumeBuilder::dotProduct(const std::array<double, 3>& left, const std::array<double, 3>& right)
{
    return (left[0] * right[0]) +
           (left[1] * right[1]) +
           (left[2] * right[2]);
}

std::array<double, 3> VolumeBuilder::crossProduct(
    const std::array<double, 3>& left,
    const std::array<double, 3>& right)
{
    return {
        (left[1] * right[2]) - (left[2] * right[1]),
        (left[2] * right[0]) - (left[0] * right[2]),
        (left[0] * right[1]) - (left[1] * right[0])};
}

VolumeBuilder::SliceBasis VolumeBuilder::deriveSliceBasis(const DicomImage& firstImage)
{
    SliceBasis basis;
    if (!firstImage.hasImageOrientationPatient())
    {
        return basis;
    }

    const auto& orientation = firstImage.imageOrientationPatient();
    basis.row = normalizeVector({orientation[0], orientation[1], orientation[2]});
    basis.column = normalizeVector({orientation[3], orientation[4], orientation[5]});
    if (basis.row == std::array<double, 3>{0.0, 0.0, 0.0} ||
        basis.column == std::array<double, 3>{0.0, 0.0, 0.0})
    {
        return SliceBasis{};
    }

    const double projection = dotProduct(basis.column, basis.row);
    basis.column = normalizeVector({
        basis.column[0] - (projection * basis.row[0]),
        basis.column[1] - (projection * basis.row[1]),
        basis.column[2] - (projection * basis.row[2])});
    if (basis.column == std::array<double, 3>{0.0, 0.0, 0.0})
    {
        return SliceBasis{};
    }

    basis.normal = normalizeVector(crossProduct(basis.row, basis.column));
    if (basis.normal == std::array<double, 3>{0.0, 0.0, 0.0})
    {
        return SliceBasis{};
    }

    basis.column = normalizeVector(crossProduct(basis.normal, basis.row));
    return basis;
}

void VolumeBuilder::validateImageOrientation(
    const DicomImage& image,
    const SliceBasis& referenceBasis,
    const VolumeValidationSettings& settings,
    QStringList& warnings)
{
    if (!image.hasImageOrientationPatient())
    {
        appendGeometryWarning(
            warnings,
            QString("One or more slices are missing Image Orientation Patient metadata. "
                    "MPR/3D can continue, but displayed geometry may be approximate. %1")
                .arg(imageIdentity(image)));
        return;
    }

    const SliceBasis imageBasis = deriveSliceBasis(image);
    if (std::abs(dotProduct(imageBasis.row, referenceBasis.row)) < (1.0 - settings.orientationAlignmentTolerance) ||
        std::abs(dotProduct(imageBasis.column, referenceBasis.column)) < (1.0 - settings.orientationAlignmentTolerance) ||
        std::abs(dotProduct(imageBasis.normal, referenceBasis.normal)) < (1.0 - settings.orientationAlignmentTolerance))
    {
        appendGeometryWarning(
            warnings,
            QString("The selected series has inconsistent slice orientation. "
                    "MPR/3D can continue, but derived plane geometry may be unreliable. %1")
                .arg(imageIdentity(image)));
    }
}

void VolumeBuilder::validateImageSpacing(
    const DicomImage& image,
    const DicomImage& referenceImage,
    const VolumeValidationSettings& settings,
    QStringList& warnings)
{
    const bool referenceHasPixelSpacing = referenceImage.hasPixelSpacing();
    const bool imageHasPixelSpacing = image.hasPixelSpacing();
    if (referenceHasPixelSpacing != imageHasPixelSpacing)
    {
        appendGeometryWarning(
            warnings,
            QString("The selected series has inconsistent pixel spacing availability. "
                    "MPR/3D will use available spacing where possible. %1")
                .arg(imageIdentity(image)));
        return;
    }

    if (!referenceHasPixelSpacing)
    {
        return;
    }

    if (std::abs(image.pixelSpacingX() - referenceImage.pixelSpacingX()) > settings.spacingTolerance ||
        std::abs(image.pixelSpacingY() - referenceImage.pixelSpacingY()) > settings.spacingTolerance)
    {
        appendGeometryWarning(
            warnings,
            QString("Series has inconsistent in-plane pixel spacing: reference=(%1,%2), image=(%3,%4), %5")
                .arg(referenceImage.pixelSpacingX())
                .arg(referenceImage.pixelSpacingY())
                .arg(image.pixelSpacingX())
                .arg(image.pixelSpacingY())
                .arg(imageIdentity(image)));
    }
}

void VolumeBuilder::validateSliceSpacing(
    const VolumeInput& input,
    const SliceBasis& basis,
    const VolumeValidationSettings& settings,
    QStringList& warnings)
{
    if (input.orderedImages.size() < 2)
    {
        return;
    }

    bool allHavePosition = true;
    for (const auto* imagePtr : input.orderedImages)
    {
        if (!imagePtr->hasImagePositionPatient())
        {
            allHavePosition = false;
            break;
        }
    }

    if (!allHavePosition)
    {
        appendGeometryWarning(
            warnings,
            "The selected series has incomplete Image Position Patient metadata. "
            "MPR/3D will use DICOM spacing fallback values.");
        return;
    }

    double expectedSpacing = -1.0;
    bool reportedDuplicatePosition = false;
    bool reportedNonUniformSpacing = false;
    for (std::size_t index = 1; index < input.orderedImages.size(); ++index)
    {
        const double previousCoordinate = sliceCoordinate(*input.orderedImages[index - 1], basis);
        const double currentCoordinate = sliceCoordinate(*input.orderedImages[index], basis);
        const double delta = std::abs(currentCoordinate - previousCoordinate);
        if (delta <= settings.spacingTolerance)
        {
            if (!reportedDuplicatePosition)
            {
                appendGeometryWarning(
                    warnings,
                    QString("Series has duplicate or overlapping slice positions near ordered index %1; "
                            "MPR will continue with spacing fallback where possible.")
                        .arg(index));
                reportedDuplicatePosition = true;
            }
            continue;
        }

        if (expectedSpacing < 0.0)
        {
            expectedSpacing = delta;
            continue;
        }

        if (std::abs(delta - expectedSpacing) > settings.spacingTolerance)
        {
            if (!reportedNonUniformSpacing)
            {
                appendGeometryWarning(
                    warnings,
                    QString("Series has inconsistent slice spacing near ordered index %1: expected=%2, actual=%3. "
                            "MPR will use robust derived spacing.")
                        .arg(index)
                        .arg(expectedSpacing)
                        .arg(delta));
                reportedNonUniformSpacing = true;
            }
        }
    }
}

void VolumeBuilder::validateSliceGeometry(
    const VolumeInput& input,
    const SliceBasis& basis,
    const VolumeValidationSettings& settings,
    QStringList& warnings)
{
    if (!input.firstImage)
    {
        throw makeVolumeBuildError(
            ErrorCode::VolumeBuildInvalidInput,
            "Series is missing a reference image for volume reconstruction",
            "The selected series is missing a valid reference image.");
    }

    const DicomImage& referenceImage = *input.firstImage;
    if (!referenceImage.hasImageOrientationPatient())
    {
        appendGeometryWarning(
            warnings,
            "The reference slice is missing Image Orientation Patient metadata. "
            "MPR/3D will use the default axial orientation.");
    }

    for (const auto* imagePtr : input.orderedImages)
    {
        validateImageOrientation(*imagePtr, basis, settings, warnings);
        validateImageSpacing(*imagePtr, referenceImage, settings, warnings);
    }

    validateSliceSpacing(input, basis, settings, warnings);
}

double VolumeBuilder::sliceCoordinate(const DicomImage& image, const SliceBasis& basis)
{
    if (!image.hasImagePositionPatient())
    {
        return 0.0;
    }

    const auto& position = image.imagePositionPatient();
    return (position[0] * basis.normal[0]) +
           (position[1] * basis.normal[1]) +
           (position[2] * basis.normal[2]);
}

void VolumeBuilder::sortSlicesByGeometry(std::vector<const DicomImage*>& orderedImages, const SliceBasis& basis)
{
    std::stable_sort(
        orderedImages.begin(),
        orderedImages.end(),
        [&basis](const DicomImage* left, const DicomImage* right) {
            return sliceCoordinate(*left, basis) < sliceCoordinate(*right, basis);
        });
}

double VolumeBuilder::deriveZSpacing(
    const VolumeInput& input,
    const SliceBasis& basis,
    const VolumeValidationSettings& settings)
{
    if (input.depth > 1)
    {
        bool allHavePosition = true;
        for (const auto* imagePtr : input.orderedImages)
        {
            if (!imagePtr->hasImagePositionPatient())
            {
                allHavePosition = false;
                break;
            }
        }

        if (allHavePosition)
        {
            std::vector<double> positiveDeltas;
            positiveDeltas.reserve(input.orderedImages.size() - 1);
            for (std::size_t index = 1; index < input.orderedImages.size(); ++index)
            {
                const double previousCoordinate = sliceCoordinate(*input.orderedImages[index - 1], basis);
                const double currentCoordinate = sliceCoordinate(*input.orderedImages[index], basis);
                const double delta = std::abs(currentCoordinate - previousCoordinate);
                if (delta > settings.spacingTolerance)
                {
                    positiveDeltas.push_back(delta);
                }
            }

            if (!positiveDeltas.empty())
            {
                const auto middle = positiveDeltas.begin() + static_cast<std::ptrdiff_t>(positiveDeltas.size() / 2);
                std::nth_element(positiveDeltas.begin(), middle, positiveDeltas.end());
                return *middle;
            }
        }
    }

    if (input.firstImage->spacingBetweenSlices() > 0.0)
    {
        return input.firstImage->spacingBetweenSlices();
    }
    if (input.firstImage->sliceThickness() > 0.0)
    {
        return input.firstImage->sliceThickness();
    }

    return 1.0;
}

VolumeGeometry VolumeBuilder::buildGeometry(
    const VolumeInput& input,
    const SliceBasis& basis,
    const VolumeValidationSettings& settings)
{
    VolumeGeometry geometry;
    geometry.dimensions = {input.width, input.height, input.depth};
    geometry.spacing = {
        input.firstImage->hasPixelSpacing() ? input.firstImage->pixelSpacingX() : 1.0,
        input.firstImage->hasPixelSpacing() ? input.firstImage->pixelSpacingY() : 1.0,
        1.0};

    geometry.spacing.z = deriveZSpacing(input, basis, settings);

    if (input.orderedImages.front()->hasImagePositionPatient())
    {
        const auto& origin = input.orderedImages.front()->imagePositionPatient();
        geometry.origin = {origin[0], origin[1], origin[2]};
    }

    geometry.direction = {
        basis.row[0], basis.column[0], basis.normal[0],
        basis.row[1], basis.column[1], basis.normal[1],
        basis.row[2], basis.column[2], basis.normal[2]};
    return geometry;
}

std::vector<int16_t> VolumeBuilder::buildVoxelBuffer(const VolumeInput& input)
{
    std::vector<int16_t> voxels;
    voxels.reserve(input.width * input.height * input.depth);

    for (const auto* imagePtr : input.orderedImages)
    {
        for (int y = 0; y < input.height; ++y)
        {
            for (int x = 0; x < input.width; ++x)
            {
                const int value = imagePtr->rawPixelValueAt(x, y);
                voxels.push_back(static_cast<int16_t>(std::clamp(value, -32768, 32767)));
            }
        }
    }

    return voxels;
}

AppResult<VolumeBuildResult> VolumeBuilder::buildFromDiagnosticSeries(const Series& diagnosticSeries) const
{
    try
    {
        VolumeInput input = collectVolumeInput(diagnosticSeries);
        if (!input.firstImage)
        {
            return makeVolumeBuildError(
                ErrorCode::VolumeBuildInvalidInput,
                "Diagnostic series does not contain a valid first image",
                "The selected series does not contain enough diagnostic image data to build a volume.");
        }

        const SliceBasis basis = deriveSliceBasis(*input.firstImage);
        sortSlicesByGeometry(input.orderedImages, basis);

        const DicomImage* firstOrderedImage = input.orderedImages.empty() ? nullptr : input.orderedImages.front();
        const DicomImage* lastOrderedImage = input.orderedImages.empty() ? nullptr : input.orderedImages.back();
        /*qDebug().nospace()
            << "VolumeBuilder ordered input:"
            << " count=" << input.orderedImages.size()
            << " dims=(" << input.width << ", " << input.height << ", " << input.depth << ")"
            << " basis.row=(" << basis.row[0] << ", " << basis.row[1] << ", " << basis.row[2] << ")"
            << " basis.column=(" << basis.column[0] << ", " << basis.column[1] << ", " << basis.column[2] << ")"
            << " basis.normal=(" << basis.normal[0] << ", " << basis.normal[1] << ", " << basis.normal[2] << ")";*/

        if (firstOrderedImage && lastOrderedImage &&
            firstOrderedImage->hasImagePositionPatient() &&
            lastOrderedImage->hasImagePositionPatient())
        {
            const auto& firstPosition = firstOrderedImage->imagePositionPatient();
            const auto& lastPosition = lastOrderedImage->imagePositionPatient();
            /*qDebug().nospace()
                << "VolumeBuilder ordered positions:"
                << " first=(" << firstPosition[0] << ", " << firstPosition[1] << ", " << firstPosition[2] << ")"
                << " last=(" << lastPosition[0] << ", " << lastPosition[1] << ", " << lastPosition[2] << ")"
                << " firstCoord=" << sliceCoordinate(*firstOrderedImage, basis)
                << " lastCoord=" << sliceCoordinate(*lastOrderedImage, basis);*/
        }

        QStringList warnings;
        validateSliceGeometry(input, basis, m_validationSettings, warnings);
        VolumeGeometry geometry = buildGeometry(input, basis, m_validationSettings);
        /*qDebug().nospace()
            << "VolumeBuilder output geometry:"
            << " dims=(" << geometry.dimensions.x << ", " << geometry.dimensions.y << ", " << geometry.dimensions.z << ")"
            << " spacing=(" << geometry.spacing.x << ", " << geometry.spacing.y << ", " << geometry.spacing.z << ")"
            << " origin=(" << geometry.origin.x << ", " << geometry.origin.y << ", " << geometry.origin.z << ")"
            << " direction=["
            << geometry.direction[0] << ", " << geometry.direction[1] << ", " << geometry.direction[2] << "; "
            << geometry.direction[3] << ", " << geometry.direction[4] << ", " << geometry.direction[5] << "; "
            << geometry.direction[6] << ", " << geometry.direction[7] << ", " << geometry.direction[8] << "]";*/
        std::vector<int16_t> voxels = buildVoxelBuffer(input);

        return VolumeBuildResult{
            std::make_shared<VolumeData<int16_t>>(std::move(geometry), std::move(voxels)),
            std::move(warnings)};
    }
    catch (const AppError& error)
    {
        return error;
    }
    catch (const std::exception& exception)
    {
        return makeVolumeBuildError(
            ErrorCode::VolumeBuildInvalidInput,
            exception.what(),
            "Failed to build a diagnostic volume from the selected series.");
    }
}
