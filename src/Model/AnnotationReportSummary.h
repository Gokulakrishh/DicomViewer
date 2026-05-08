#pragma once

#include "ViewerTools/Measurements/MeasurementTypes.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

/**
 * @brief Aggregated annotation counts for one DICOM series.
 *
 * Used by the study browser to show annotation presence without loading full
 * geometry for every slice.
 */
struct AnnotationReportSummary
{
    QString seriesInstanceUid;
    int annotationCount{0};
    int annotatedSliceCount{0};
    int distanceCount{0};
    int polylineCount{0};
    int angleCount{0};
    int rectangleRoiCount{0};

    /**
     * @brief Reports whether the series has any active annotations.
     * @return True when annotationCount is greater than zero.
     */
    [[nodiscard]] bool hasAnnotations() const
    {
        return annotationCount > 0;
    }
};

using AnnotationReportSummaryBySeries = QHash<QString, AnnotationReportSummary>;

/**
 * @brief Filter used when querying annotation report rows.
 *
 * The filter supports global search and optional series/slice constraints so UI
 * panels can load bounded data for large DICOM databases.
 */
struct AnnotationReportFilter
{
    QString searchText;
    QString bodyRegion;
    QString measurementType;
    QString seriesInstanceUid;
    QString sopInstanceUid;
    int frameIndex{-1};
    int limit{200};
};

/**
 * @brief One annotation row enriched with DICOM hierarchy context.
 *
 * Responsibilities:
 * - Carry user-visible annotation metadata and display value.
 * - Carry enough patient/study/series/slice context for search and navigation.
 */
struct AnnotationReportRow
{
    QString annotationId;
    QString label;
    QString bodyRegion;
    QString note;
    MeasurementType measurementType{MeasurementType::Distance};
    QString measurementTypeName;
    QString displayValue;
    QString patientId;
    QString patientName;
    QString studyDate;
    QString seriesInstanceUid;
    QString seriesDescription;
    QString modality;
    QString sopInstanceUid;
    int frameIndex{0};
    QString instanceNumber;
    QDateTime updatedAtUtc;
};

using AnnotationReportRows = QList<AnnotationReportRow>;

/**
 * @brief Annotation rows grouped by one DICOM slice.
 *
 * This grouping backs the compact annotated-slice browser and prevents a flat
 * list of every measurement from overwhelming the user.
 */
struct AnnotationSliceGroup
{
    QString seriesInstanceUid;
    QString sopInstanceUid;
    QString instanceNumber;
    QString patientId;
    QString patientName;
    QString studyDate;
    QString seriesDescription;
    QString modality;
    int frameIndex{0};
    AnnotationReportRows rows;
};

using AnnotationSliceGroups = QList<AnnotationSliceGroup>;

/**
 * @brief Context for the currently displayed DICOM slice.
 *
 * The current-slice annotation panel uses this to label edits and ensure actions
 * remain scoped to the active SOP Instance UID.
 */
struct AnnotationCurrentSliceContext
{
    bool hasSlice{false};
    QString seriesInstanceUid;
    QString sopInstanceUid;
    int frameIndex{0};
    QString patientName;
    QString patientAge;
    QString patientDob;
    QString doctorName;
    QString studyDate;
    QString seriesDescription;
    QString modality;
    QString instanceNumber;
    int sliceIndex{-1};
    int sliceCount{0};
};
