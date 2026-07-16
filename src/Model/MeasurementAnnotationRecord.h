#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QDateTime>
#include <optional>

/**
 * @brief Persistable measurement/ROI annotation scoped to one DICOM slice.
 *
 * Responsibilities:
 * - Link measurement geometry to Series Instance UID, SOP Instance UID, and
 *   frame index for multi-frame XA/cine instances.
 * - Store user-editable label/body-region metadata and derived statistics.
 *
 * Assumptions:
 * - Annotations are stored in the application database, not written back into
 *   source DICOM files.
 */
struct SliceMeasurementAnnotationRecord
{
    QString seriesInstanceUid;
    QString sopInstanceUid;
    int frameIndex{0};
    QString label;
    QString bodyRegion;
    QString note;
    MeasurementAnnotation measurement;
    std::optional<double> angleDegrees;
    std::optional<RoiStatistics> roiStatistics;
    QDateTime createdAtUtc;
    QDateTime updatedAtUtc;
    bool deleted{false};
};

/**
 * @brief Persistable measurement/ROI annotation scoped to an MPR plane context.
 *
 * Responsibilities:
 * - Store MPR-derived annotations separately from source DICOM slice/frame
 *   annotations.
 * - Preserve the plane identity and patient/world measurement geometry needed
 *   to redisplay or navigate to the derived annotation.
 *
 * Assumptions:
 * - MPR annotations are application-derived data in SQLite and are not written
 *   back to source DICOM files.
 * - `planeType` is currently axial/coronal/sagittal; future oblique support
 *   shall add full plane origin/axis metadata before clinical claims.
 */
struct MprMeasurementAnnotationRecord
{
    QString seriesInstanceUid;
    QString planeType;
    double planePositionMm{0.0};
    QString label;
    QString bodyRegion;
    QString note;
    MeasurementAnnotation measurement;
    std::optional<double> angleDegrees;
    std::optional<RoiStatistics> roiStatistics;
    QDateTime createdAtUtc;
    QDateTime updatedAtUtc;
    bool deleted{false};
};
