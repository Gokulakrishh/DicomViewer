#pragma once

#include "Utilities/DatabaseSettings.h"
#include "Database/DatabaseService.h"

#include <memory>

class QSqlQuery;
class PostgreConnection;

class PostgreService final : public DatabaseService
{
public:
    explicit PostgreService(const DatabaseSettings& databaseSettings);
    explicit PostgreService(std::unique_ptr<PostgreConnection> connection);
    ~PostgreService() override;

    bool initialize() override;
    QString lastErrorText() const override;

    bool savePatient(const PatientPtr& patient) override;
    PatientPtr getPatient(const QString& patientId) override;
    QList<PatientPtr> getAllPatients(const QString& filterText = {}) override;
    QList<StudyPtr> getStudiesForPatient(const QString& patientId, const QString& filterText = {}) override;
    QList<SeriesPtr> getSeriesForStudy(const QString& studyInstanceUid, const QString& filterText = {}) override;

    StudyPtr getStudy(const QString& studyInstanceUid) override;
    SeriesPtr getSeries(const QString& seriesInstanceUid) override;
    DicomImagePtr getImage(const QString& sopInstanceUid) override;
    bool upsertSliceMeasurementAnnotation(const SliceMeasurementAnnotationRecord& record) override;
    QList<SliceMeasurementAnnotationRecord> loadSliceMeasurementAnnotations(const QString& sopInstanceUid) override;
    bool markSliceMeasurementAnnotationDeleted(const QString& annotationId) override;
    QPixmap getPreviewForPatient(const QString& patientId) override;
    QPixmap getPreviewForStudy(const QString& studyInstanceUid) override;
    QPixmap getPreviewForSeries(const QString& seriesInstanceUid) override;

private:
    bool ensureConnection();
    bool createTables();
    bool saveStudy(const QString& patientId, const Study& study);
    bool saveSeries(const QString& studyInstanceUid, const Series& series);
    bool saveImage(const QString& seriesInstanceUid, const DicomImage& image);
    void populateStudies(Patient& patient);
    void populateSeries(Study& study);
    void populateImages(Series& series);
    QPixmap createSeriesPreviewPixmap(const Series& series) const;
    DicomImagePtr createImageFromQuery(const QSqlQuery& query) const;

private:
    std::unique_ptr<PostgreConnection> m_connection;
    bool m_connectionAttempted{false};
    bool m_connectionAvailable{false};
};
