#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Database/DatabaseService.h"

#include <memory>

class QSqlQuery;
class SqliteConnection;

/**
 * @brief SQLite-backed implementation of application persistence.
 *
 * Responsibilities:
 * - Maintain the local schema for DICOM hierarchy, previews, and annotations.
 * - Persist only lightweight metadata and user annotations to avoid duplicating
 *   large DICOM pixel data.
 * - Provide UI-oriented reporting queries with bounded result sets.
 *
 * Assumptions:
 * - SQLite is used as local-first storage and can be replaced behind
 *   DatabaseService without changing viewer tools.
 * - Source DICOM files remain the canonical image data.
 */
class SqliteService final : public DatabaseService
{
public:
    /**
     * @brief Creates a SQLite service from database settings.
     * @param databaseSettings SQLite file settings.
     */
    explicit SqliteService(const DatabaseSettings& databaseSettings);

    /**
     * @brief Creates a SQLite service with an injected connection.
     * @param connection Connection used by the service; ownership is transferred.
     */
    explicit SqliteService(std::unique_ptr<SqliteConnection> connection);
    ~SqliteService() override;

    /**
     * @brief Opens the database and ensures schema availability.
     * @return True when the service is ready for queries.
     */
    bool initialize() override;

    /**
     * @brief Returns the most recent database error.
     * @return Human-readable error text.
     */
    QString lastErrorText() const override;

    /**
     * @brief Saves a patient hierarchy.
     * @param patient Patient with nested study, series, and slice metadata.
     * @return True when all records were persisted.
     */
    bool savePatient(const PatientPtr& patient) override;

    /**
     * @brief Loads one patient by DICOM patient id.
     * @param patientId DICOM patient identifier.
     * @return Patient hierarchy, or null when not found.
     */
    PatientPtr getPatient(const QString& patientId) override;

    /**
     * @brief Loads patients matching an optional filter.
     * @param filterText Optional search text.
     * @return Matching patients.
     */
    QList<PatientPtr> getAllPatients(const QString& filterText = {}) override;

    /**
     * @brief Loads studies for a patient.
     * @param patientId DICOM patient identifier.
     * @param filterText Optional search text.
     * @return Matching studies.
     */
    QList<StudyPtr> getStudiesForPatient(const QString& patientId, const QString& filterText = {}) override;

    /**
     * @brief Loads series for a study.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @param filterText Optional search text.
     * @return Matching series.
     */
    QList<SeriesPtr> getSeriesForStudy(const QString& studyInstanceUid, const QString& filterText = {}) override;

    /**
     * @brief Loads one study by Study Instance UID.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @return Study hierarchy, or null when not found.
     */
    StudyPtr getStudy(const QString& studyInstanceUid) override;

    /**
     * @brief Loads one series by Series Instance UID.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @return Series with slice metadata, or null when not found.
     */
    SeriesPtr getSeries(const QString& seriesInstanceUid) override;

    /**
     * @brief Loads one DICOM slice metadata record.
     * @param sopInstanceUid DICOM SOP Instance UID.
     * @return Image metadata, or null when not found.
     */
    DicomImagePtr getImage(const QString& sopInstanceUid) override;

    /**
     * @brief Inserts or updates a slice annotation.
     * @param record Measurement/ROI annotation scoped to one slice.
     * @return True when the annotation was saved.
     */
    bool upsertSliceMeasurementAnnotation(const SliceMeasurementAnnotationRecord& record) override;

    /**
     * @brief Loads active annotations for one slice.
     * @param sopInstanceUid DICOM SOP Instance UID.
     * @return Non-deleted annotations.
     */
    QList<SliceMeasurementAnnotationRecord> loadSliceMeasurementAnnotations(
        const QString& sopInstanceUid,
        int frameIndex = 0) override;

    /**
     * @brief Soft-deletes an annotation by id.
     * @param annotationId Stable annotation identifier.
     * @return True when the annotation was marked deleted.
     */
    bool markSliceMeasurementAnnotationDeleted(const QString& annotationId) override;

    /**
     * @brief Loads annotation summary counts for series rows.
     * @param seriesInstanceUids Series identifiers to summarize.
     * @return Summary map keyed by Series Instance UID.
     */
    AnnotationReportSummaryBySeries loadSeriesAnnotationReportSummaries(
        const QList<QString>& seriesInstanceUids) override;

    /**
     * @brief Loads annotation report rows for browsing/search.
     * @param filter Report filter and result limit.
     * @return Matching annotation rows.
     */
    AnnotationReportRows loadAnnotationReportRows(const AnnotationReportFilter& filter) override;

    /**
     * @brief Updates editable annotation metadata.
     * @param annotationId Stable annotation identifier.
     * @param label User-facing annotation name.
     * @param bodyRegion Body region/group label.
     * @param note Optional note text.
     * @return True when metadata was updated.
     */
    bool updateAnnotationReportMetadata(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note = {}) override;

    /**
     * @brief Loads study preview items for a patient.
     * @param patientId DICOM patient identifier.
     * @return Lightweight study preview/navigation items.
     */
    DicomPreviewItems getStudyPreviewItemsForPatient(const QString& patientId) override;

    /**
     * @brief Loads series preview items for a study.
     * @param studyInstanceUid DICOM Study Instance UID.
     * @return Lightweight series preview/navigation items.
     */
    DicomPreviewItems getSeriesPreviewItemsForStudy(const QString& studyInstanceUid) override;

    /**
     * @brief Inserts or replaces a bounded representative preview for a series.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @param previewPixmap Derived thumbnail image.
     * @return True when the preview row was updated.
     */
    bool upsertSeriesPreview(const QString& seriesInstanceUid, const QPixmap& previewPixmap) override;

private:
    /** @brief Opens the connection lazily and updates availability flags. */
    bool ensureConnection();
    /** @brief Creates the current database schema when missing. */
    bool createTables();
    /** @brief Migrates annotation metadata columns used by the report UI. */
    bool ensureAnnotationMetadataColumns();
    bool ensureSliceMetadataColumns();
    /** @brief Persists one study row under an existing patient. */
    bool saveStudy(const QString& patientId, const Study& study);
    /** @brief Persists one series row under an existing study. */
    bool saveSeries(const QString& studyInstanceUid, const Series& series);
    /** @brief Persists one lightweight DICOM slice metadata row. */
    bool saveImage(const QString& seriesInstanceUid, const DicomImage& image);
    /** @brief Populates child studies for a loaded patient. */
    void populateStudies(Patient& patient);
    /** @brief Populates child series for a loaded study. */
    void populateSeries(Study& study);
    /** @brief Populates lightweight slice metadata for a loaded series. */
    void populateImages(Series& series);
    /** @brief Creates a bounded preview pixmap from representative series data. */
    QPixmap createSeriesPreviewPixmap(const Series& series) const;
    /** @brief Maps a SQL row into a lightweight DicomImage metadata object. */
    DicomImagePtr createImageFromQuery(const QSqlQuery& query) const;

private:
    std::unique_ptr<SqliteConnection> m_connection;
    bool m_connectionAttempted{false};
    bool m_connectionAvailable{false};
};
