#include "Database/SqliteService.h"
#include "Model/DicomParameters.h"
#include "Services/AnnotationReportService.h"
#include "Utilities/DatabaseSettings.h"

#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <iostream>
#include <memory>

namespace
{
int fail(const QString& message)
{
    std::cerr << message.toStdString() << '\n';
    return 1;
}

std::unique_ptr<DicomImage> makeImage(const QString& sopInstanceUid, const QString& instanceNumber)
{
    auto image = std::make_unique<DicomImage>();
    image->setSopInstanceUid(sopInstanceUid);
    image->setInstanceNumber(instanceNumber);
    image->setFilePath(QString("/tmp/%1.dcm").arg(sopInstanceUid));
    image->setDimensions(16, 16);
    return image;
}

bool createLegacyAnnotationTableDatabase(const QString& databasePath, QString* errorMessage)
{
    const QString connectionName = "LegacyAnnotationTableSmokeTest_" + QUuid::createUuid().toString(QUuid::Id128);
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    database.setDatabaseName(databasePath);
    if (!database.open())
    {
        if (errorMessage)
        {
            *errorMessage = database.lastError().text();
        }
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    QSqlQuery query(database);
    const bool created = query.exec(
        "CREATE TABLE measurement_annotations ("
        "annotation_id TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL,"
        "sop_instance_uid TEXT NOT NULL,"
        "measurement_type TEXT NOT NULL,"
        "points_json TEXT NOT NULL,"
        "color_hex TEXT NOT NULL,"
        "length_mm REAL,"
        "angle_degrees REAL,"
        "area_mm2 REAL,"
        "sample_count INTEGER,"
        "mean_value REAL,"
        "stddev_value REAL,"
        "min_value REAL,"
        "max_value REAL,"
        "created_at TEXT NOT NULL DEFAULT (STRFTIME('%Y-%m-%dT%H:%M:%fZ', 'now')),"
        "updated_at TEXT NOT NULL DEFAULT (STRFTIME('%Y-%m-%dT%H:%M:%fZ', 'now')),"
        "is_deleted INTEGER NOT NULL DEFAULT 0"
        ")");
    if (!created && errorMessage)
    {
        *errorMessage = query.lastError().text();
    }

    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return created;
}
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QTemporaryDir directory;
    if (!directory.isValid())
    {
        return fail("Failed to create temporary test directory.");
    }

    DatabaseSettings settings;
    settings.setFilePath(directory.filePath("dicomviewer-test.sqlite"));

    SqliteService service(settings);
    if (!service.initialize())
    {
        return fail("Failed to initialize SQLite service: " + service.lastErrorText());
    }

    auto patient = std::make_shared<Patient>();
    patient->setPatientId("PAT-001");
    patient->setPatientName("SQLite^Smoke");
    patient->setPatientSex("O");
    patient->setDateOfBirth("19700101");

    Study& study = patient->getOrCreateStudy("1.2.3.study");
    study.setStudyDescription("SQLite smoke study");
    study.setStudyDate("20260503");
    study.setDoctorName("Test");

    Series& series = study.getOrCreateSeries("1.2.3.series");
    series.setSeriesDescription("SQLite smoke series");
    series.setModality("CT");
    series.setSeriesNumber("1");
    series.addImage(makeImage("1.2.3.slice.10", "10"));
    auto multiFrameImage = makeImage("1.2.3.slice.2", "2");
    multiFrameImage->setFrameCount(3);
    multiFrameImage->setCineTiming(40.0, 25.0, 40.0);
    series.addImage(std::move(multiFrameImage));

    if (!service.savePatient(patient))
    {
        return fail("Failed to save patient hierarchy: " + service.lastErrorText());
    }

    const QList<DatabaseService::PatientPtr> patients = service.getAllPatients();
    if (patients.size() != 1 || patients.front()->patientId() != "PAT-001")
    {
        return fail("Patient hierarchy was not reloaded from SQLite.");
    }

    const DatabaseService::SeriesPtr loadedSeries = service.getSeries("1.2.3.series");
    if (!loadedSeries || loadedSeries->images().size() != 4)
    {
        return fail("Series images were not reloaded from SQLite.");
    }
    if (loadedSeries->imageCount() != 4
        || loadedSeries->images().front()->instanceNumber() != "2"
        || loadedSeries->images().front()->frameIndex() != 0
        || loadedSeries->images().at(2)->frameIndex() != 2)
    {
        return fail("SQLite frame expansion or numeric instance ordering was not preserved.");
    }
    if (loadedSeries->images().at(2)->cineFrameIntervalMs() != 40.0)
    {
        return fail("SQLite did not preserve DICOM XA cine timing metadata.");
    }

    const DicomPreviewItems studyPreviews = service.getStudyPreviewItemsForPatient("PAT-001");
    if (studyPreviews.size() != 1 || studyPreviews.front().badgeText != "1 series")
    {
        return fail("Study preview items did not summarize patient studies.");
    }
    if (studyPreviews.front().targetType != DicomPreviewTargetType::Study
        || studyPreviews.front().targetId != "1.2.3.study"
        || studyPreviews.front().parentId != "PAT-001")
    {
        return fail("Study preview item did not carry navigation target.");
    }

    const DicomPreviewItems seriesPreviews = service.getSeriesPreviewItemsForStudy("1.2.3.study");
    if (seriesPreviews.size() != 1 || seriesPreviews.front().badgeText != "4 images")
    {
        return fail("Series preview items did not summarize study series.");
    }
    if (seriesPreviews.front().targetType != DicomPreviewTargetType::Series
        || seriesPreviews.front().targetId != "1.2.3.series"
        || seriesPreviews.front().parentId != "1.2.3.study")
    {
        return fail("Series preview item did not carry navigation target.");
    }

    QImage generatedPreviewImage(16, 16, QImage::Format_RGB32);
    generatedPreviewImage.fill(Qt::green);
    const QPixmap generatedPreview = QPixmap::fromImage(generatedPreviewImage);
    if (!service.upsertSeriesPreview("1.2.3.series", generatedPreview))
    {
        return fail("SQLite did not persist generated series preview.");
    }

    DicomPreviewItems previewsWithPixmap = service.getSeriesPreviewItemsForStudy("1.2.3.study");
    if (previewsWithPixmap.size() != 1 || previewsWithPixmap.front().pixmap.isNull())
    {
        return fail("SQLite did not reload generated series preview.");
    }

    if (!service.savePatient(patient))
    {
        return fail("Failed to re-save metadata-only hierarchy: " + service.lastErrorText());
    }

    previewsWithPixmap = service.getSeriesPreviewItemsForStudy("1.2.3.study");
    if (previewsWithPixmap.size() != 1 || previewsWithPixmap.front().pixmap.isNull())
    {
        return fail("Metadata-only re-save cleared the generated series preview.");
    }

    const DicomPreviewItems studyPreviewsWithPixmap = service.getStudyPreviewItemsForPatient("PAT-001");
    if (studyPreviewsWithPixmap.size() != 1 || studyPreviewsWithPixmap.front().pixmap.isNull())
    {
        return fail("Study preview did not reuse persisted series preview.");
    }

    SliceMeasurementAnnotationRecord annotation;
    annotation.seriesInstanceUid = "1.2.3.series";
    annotation.sopInstanceUid = "1.2.3.slice.2";
    annotation.frameIndex = 2;
    annotation.measurement.id = "ann-001";
    annotation.measurement.type = MeasurementType::Distance;
    annotation.measurement.points = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    annotation.measurement.lengthMm = 10.0;

    if (!service.upsertSliceMeasurementAnnotation(annotation))
    {
        return fail("Failed to save measurement annotation.");
    }

    QList<SliceMeasurementAnnotationRecord> annotations = service.loadSliceMeasurementAnnotations("1.2.3.slice.2", 2);
    if (annotations.size() != 1 || annotations.front().measurement.id != "ann-001")
    {
        return fail("Measurement annotation was not loaded from SQLite.");
    }
    if (!service.loadSliceMeasurementAnnotations("1.2.3.slice.2", 0).isEmpty())
    {
        return fail("Frame-specific measurement annotation leaked into another XA frame.");
    }

    AnnotationReportService reportService(service);
    const AnnotationReportSummaryBySeries summaries = reportService.loadSeriesSummaries({"1.2.3.series"});
    const AnnotationReportSummary summary = summaries.value("1.2.3.series");
    if (summary.annotationCount != 1
        || summary.annotatedSliceCount != 1
        || summary.distanceCount != 1)
    {
        return fail("Annotation report summary was not loaded from SQLite.");
    }

    if (!reportService.updateMetadata("ann-001", "Liver lesion 1", "Liver"))
    {
        return fail("Annotation report metadata was not updated in SQLite.");
    }

    AnnotationReportFilter filter;
    filter.searchText = "liver";
    const AnnotationReportRows rows = reportService.loadRows(filter);
    if (rows.size() != 1
        || rows.front().label != "Liver lesion 1"
        || rows.front().bodyRegion != "Liver"
        || rows.front().seriesInstanceUid != "1.2.3.series"
        || rows.front().sopInstanceUid != "1.2.3.slice.2")
    {
        return fail("Annotation report row search did not return expected metadata.");
    }

    const AnnotationReportRows currentSliceRows =
        reportService.loadCurrentSliceRows("1.2.3.series", "1.2.3.slice.2", 2);
    if (currentSliceRows.size() != 1 || currentSliceRows.front().annotationId != "ann-001")
    {
        return fail("Current-slice annotation report did not return the expected annotation.");
    }

    AnnotationReportFilter groupFilter;
    groupFilter.seriesInstanceUid = "1.2.3.series";
    const AnnotationSliceGroups groups = reportService.loadSliceGroups(groupFilter);
    if (groups.size() != 1
        || groups.front().sopInstanceUid != "1.2.3.slice.2"
        || groups.front().frameIndex != 2
        || groups.front().rows.size() != 1
        || groups.front().rows.front().annotationId != "ann-001")
    {
        return fail("Grouped slice annotation report did not return the expected slice group.");
    }

    if (!reportService.deleteAnnotation("ann-001"))
    {
        return fail("Failed to soft-delete measurement annotation.");
    }

    annotations = service.loadSliceMeasurementAnnotations("1.2.3.slice.2", 2);
    if (!annotations.isEmpty())
    {
        return fail("Deleted measurement annotation is still returned by load.");
    }
    if (!reportService.loadRows(filter).isEmpty())
    {
        return fail("Deleted measurement annotation is still returned by report rows.");
    }

    const QString legacyDatabasePath = directory.filePath("legacy-annotation-table.sqlite");
    QString legacyError;
    if (!createLegacyAnnotationTableDatabase(legacyDatabasePath, &legacyError))
    {
        return fail("Failed to create legacy annotation table database: " + legacyError);
    }

    DatabaseSettings legacySettings;
    legacySettings.setFilePath(legacyDatabasePath);

    SqliteService legacyService(legacySettings);
    if (!legacyService.initialize())
    {
        return fail("Failed to migrate legacy annotation table: " + legacyService.lastErrorText());
    }

    auto legacyPatient = std::make_shared<Patient>();
    legacyPatient->setPatientId("PAT-LEGACY");
    legacyPatient->setPatientName("Legacy^Annotation");

    Study& legacyStudy = legacyPatient->getOrCreateStudy("1.2.3.legacy.study");
    legacyStudy.setStudyDate("20260504");

    Series& legacySeries = legacyStudy.getOrCreateSeries("1.2.3.legacy.series");
    legacySeries.setModality("CT");
    legacySeries.addImage(makeImage("1.2.3.legacy.slice.1", "1"));

    if (!legacyService.savePatient(legacyPatient))
    {
        return fail("Failed to save patient after legacy annotation migration: " + legacyService.lastErrorText());
    }

    SliceMeasurementAnnotationRecord legacyAnnotation;
    legacyAnnotation.seriesInstanceUid = "1.2.3.legacy.series";
    legacyAnnotation.sopInstanceUid = "1.2.3.legacy.slice.1";
    legacyAnnotation.measurement.id = "ann-legacy-001";
    legacyAnnotation.measurement.type = MeasurementType::Distance;
    legacyAnnotation.measurement.points = {{0.0, 0.0, 0.0}, {12.0, 0.0, 0.0}};
    legacyAnnotation.measurement.lengthMm = 12.0;

    if (!legacyService.upsertSliceMeasurementAnnotation(legacyAnnotation))
    {
        return fail("Failed to save annotation after legacy metadata column migration.");
    }

    return 0;
}
