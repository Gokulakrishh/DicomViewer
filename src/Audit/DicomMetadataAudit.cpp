#include "Audit/DicomMetadataAudit.h"

#include "Model/DicomImage.h"
#include "Model/DicomParameters.h"

#include <cmath>

DicomMetadataAuditResult DicomMetadataAudit::evaluateSeries(const Series& series) const
{
    DicomMetadataAuditResult result;
    if (series.images().empty())
    {
        result.blockingIssues.append("Series has no image instances.");
        return result;
    }

    const DicomImage* firstImage = nullptr;
    for (const auto& image : series.images())
    {
        if (image)
        {
            firstImage = image.get();
            break;
        }
    }

    if (!firstImage)
    {
        result.blockingIssues.append("Series contains only null image references.");
        return result;
    }

    const bool hasPixelSpacing = firstImage->hasPixelSpacing();
    const bool hasOrientation = firstImage->hasImageOrientationPatient();
    bool allHavePosition = true;
    bool allHaveRawPixels = true;
    bool consistentDimensions = true;
    bool consistentPixelSpacing = true;
    bool consistentOrientation = true;
    bool hasCalibration = false;

    constexpr double tolerance = 1e-4;
    const auto firstMetadata = firstImage->metadata();
    hasCalibration = firstMetadata &&
                     firstMetadata->hasRescaleSlope &&
                     firstMetadata->hasRescaleIntercept;

    for (const auto& image : series.images())
    {
        if (!image)
        {
            result.blockingIssues.append("Series contains a null image reference.");
            continue;
        }

        consistentDimensions = consistentDimensions &&
                               image->width() == firstImage->width() &&
                               image->height() == firstImage->height();
        consistentPixelSpacing = consistentPixelSpacing &&
                                 image->hasPixelSpacing() == hasPixelSpacing &&
                                 std::abs(image->pixelSpacingX() - firstImage->pixelSpacingX()) <= tolerance &&
                                 std::abs(image->pixelSpacingY() - firstImage->pixelSpacingY()) <= tolerance;
        allHavePosition = allHavePosition && image->hasImagePositionPatient();
        allHaveRawPixels = allHaveRawPixels && image->hasRawPixels();

        if (hasOrientation != image->hasImageOrientationPatient())
        {
            consistentOrientation = false;
        }
        else if (hasOrientation)
        {
            const auto& reference = firstImage->imageOrientationPatient();
            const auto& current = image->imageOrientationPatient();
            for (std::size_t index = 0; index < reference.size(); ++index)
            {
                consistentOrientation = consistentOrientation &&
                                        std::abs(reference[index] - current[index]) <= tolerance;
            }
        }
    }

    if (!hasPixelSpacing)
    {
        result.blockingIssues.append("Pixel Spacing is missing; physical distance measurement is unavailable.");
    }
    if (!consistentDimensions)
    {
        result.blockingIssues.append("Series image dimensions are inconsistent.");
    }
    if (!consistentPixelSpacing)
    {
        result.blockingIssues.append("Series pixel spacing is inconsistent.");
    }
    if (!hasOrientation)
    {
        result.blockingIssues.append("Image Orientation Patient is missing.");
    }
    if (!consistentOrientation)
    {
        result.blockingIssues.append("Series image orientation is inconsistent.");
    }
    if (!allHavePosition)
    {
        result.blockingIssues.append("Image Position Patient is missing for one or more instances.");
    }
    if (!allHaveRawPixels)
    {
        result.warnings.append("Raw pixels are not loaded for one or more instances.");
    }
    if (!hasCalibration)
    {
        result.warnings.append("Rescale slope/intercept are incomplete; calibrated pixel statistics may be unavailable.");
    }

    result.distanceMeasurementAvailable = hasPixelSpacing && consistentDimensions && consistentPixelSpacing;
    result.volumeGeometryAvailable =
        result.distanceMeasurementAvailable &&
        hasOrientation &&
        consistentOrientation &&
        allHavePosition;
    result.calibratedPixelValuesAvailable = allHaveRawPixels && hasCalibration;
    return result;
}

AuditEvent DicomMetadataAudit::toAuditEvent(
    const DicomMetadataAuditResult& result,
    const QString& seriesInstanceUid) const
{
    AuditEvent event;
    event.type = AuditEventType::DataValidation;
    event.severity = result.hasBlockingIssues() ? AuditSeverity::Error :
        (!result.warnings.isEmpty() ? AuditSeverity::Warning : AuditSeverity::Info);
    event.module = "DICOM";
    event.action = "SeriesMetadataAudit";
    event.subjectId = seriesInstanceUid;
    event.message = result.hasBlockingIssues() ? "DICOM series metadata validation failed." :
        "DICOM series metadata validation completed.";
    event.attributes.insert("distance_measurement_available", result.distanceMeasurementAvailable ? "true" : "false");
    event.attributes.insert("volume_geometry_available", result.volumeGeometryAvailable ? "true" : "false");
    event.attributes.insert("calibrated_pixel_values_available", result.calibratedPixelValuesAvailable ? "true" : "false");
    event.attributes.insert("warnings", result.warnings.join(" | "));
    event.attributes.insert("blocking_issues", result.blockingIssues.join(" | "));
    return event;
}
