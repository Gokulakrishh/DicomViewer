#pragma once

#include "Model/MeasurementAnnotationRecord.h"
#include "Model/DicomParameters.h"
#include "Model/AnnotationReportSummary.h"
#include "Model/DicomPreviewItem.h"

#include <QList>
#include <QString>
#include <memory>

/**
 * @brief Application persistence interface for DICOM hierarchy and annotations.
 *
 * Responsibilities:
 * - Persist and load patient, study, series, and slice metadata.
 * - Store user-created measurement/ROI annotations without modifying source
 *   DICOM files.
 * - Provide lightweight preview and annotation reporting queries for the UI.
 *
 * Assumptions:
 * - Pixel payloads are not duplicated in the database; source DICOM file paths
 *   and derived metadata are persisted.
 * - Annotation records are scoped to SOP Instance UID plus frame index so
 *   measurements remain frame-specific for multi-frame XA/cine instances.
 */
class DatabaseService
{
public:
    using PatientPtr = std::shared_ptr<Patient>;
    using StudyPtr = std::shared_ptr<Study>;
    using SeriesPtr = std::shared_ptr<Series>;
    using DicomImagePtr = std::shared_ptr<DicomImage>;

    virtual ~DatabaseService() = default;

    /**
     * @brief Initializes schema and opens required storage resources.
     * @return True when the service is ready for use.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Returns the last recoverable database error.
     * @return Human-readable error text.
     */
    virtual QString lastErrorText() const = 0;

    /**
     * @brief Saves a patient hierarchy.
     * @param patient Patient with nested studies, series, and slice metadata.
     * @return True when the hierarchy was persisted.
     */
    virtual bool savePatient(const PatientPtr& patient) = 0;

    /**
     * @brief Loads one patient by identifier.
     * @param patientId DICOM patient identifier.
     * @return Patient hierarchy, or null when not found.
     */
    virtual PatientPtr getPatient(const QString& patientId) = 0;

    /**
     * @brief Loads patients matching an optional search filter.
     * @param filterText Optional case-insensitive search text.
     * @return Matching patients with lightweight hierarchy data.
     */
    virtual QList<PatientPtr> getAllPatients(const QString& filterText = {}) = 0;

    /**
     * @brief Loads studies for one patient.
     * @param patientId DICOM patient identifier.
     * @param filterText Optional search text.
     * @return Matching studies.
     */
    virtual QList<StudyPtr> getStudiesForPatient(const QString& patientId, const QString& filterText = {}) = 0;

    /**
     * @brief Loads series for one study.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @param filterText Optional search text.
     * @return Matching series.
     */
    virtual QList<SeriesPtr> getSeriesForStudy(const QString& studyInstanceUid, const QString& filterText = {}) = 0;

    /**
     * @brief Loads one study by Study Instance UID.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @return Study hierarchy, or null when not found.
     */
    virtual StudyPtr getStudy(const QString& studyInstanceUid) = 0;

    /**
     * @brief Loads one series by Series Instance UID.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @return Series with slice metadata, or null when not found.
     */
    virtual SeriesPtr getSeries(const QString& seriesInstanceUid) = 0;

    /**
     * @brief Loads one slice metadata record.
     * @param sopInstanceUid DICOM SOP Instance UID.
     * @return Image metadata, or null when not found.
     */
    virtual DicomImagePtr getImage(const QString& sopInstanceUid) = 0;

    /**
     * @brief Inserts or updates a slice measurement annotation.
     * @param record Annotation record scoped to one series and SOP instance.
     * @return True when the annotation was saved.
     */
    virtual bool upsertSliceMeasurementAnnotation(const SliceMeasurementAnnotationRecord& record) = 0;

    /**
     * @brief Loads active annotations for one DICOM slice/frame.
     * @param sopInstanceUid DICOM SOP Instance UID.
     * @param frameIndex Zero-based frame index for multi-frame instances.
     * @return Non-deleted annotations for the slice/frame.
     */
    virtual QList<SliceMeasurementAnnotationRecord> loadSliceMeasurementAnnotations(
        const QString& sopInstanceUid,
        int frameIndex = 0) = 0;

    /**
     * @brief Soft-deletes an annotation.
     * @param annotationId Stable annotation identifier.
     * @return True when the annotation was marked deleted.
     */
    virtual bool markSliceMeasurementAnnotationDeleted(const QString& annotationId) = 0;

    /**
     * @brief Loads annotation summary counts for series rows.
     * @param seriesInstanceUids Series identifiers to summarize.
     * @return Summary map keyed by Series Instance UID.
     */
    virtual AnnotationReportSummaryBySeries loadSeriesAnnotationReportSummaries(
        const QList<QString>& seriesInstanceUids) = 0;

    /**
     * @brief Loads report rows for annotation browsing/search.
     * @param filter Search, type, body-region, and slice/series constraints.
     * @return Matching annotation report rows.
     */
    virtual AnnotationReportRows loadAnnotationReportRows(const AnnotationReportFilter& filter) = 0;

    /**
     * @brief Updates user-editable annotation metadata.
     * @param annotationId Stable annotation identifier.
     * @param label User-visible annotation name.
     * @param bodyRegion Body region/group label.
     * @param note Optional free-text note.
     * @return True when metadata was updated.
     */
    virtual bool updateAnnotationReportMetadata(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note = {}) = 0;

    /**
     * @brief Loads study thumbnail items for a patient.
     * @param patientId DICOM patient identifier.
     * @return Lightweight preview items for study browser navigation.
     */
    virtual DicomPreviewItems getStudyPreviewItemsForPatient(const QString& patientId) = 0;

    /**
     * @brief Loads series thumbnail items for a study.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @return Lightweight preview items for study browser navigation.
     */
    virtual DicomPreviewItems getSeriesPreviewItemsForStudy(const QString& studyInstanceUid) = 0;
};
