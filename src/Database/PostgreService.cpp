#include "Database/PostgreService.h"

#include "Database/PostgreConnection.h"

#include <QBuffer>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace
{
QString measurementTypeToString(MeasurementType type)
{
    switch (type)
    {
    case MeasurementType::Distance:
        return "distance";
    case MeasurementType::Polyline:
        return "polyline";
    case MeasurementType::Angle:
        return "angle";
    case MeasurementType::RectangleRoi:
        return "rectangle_roi";
    }

    return "distance";
}

MeasurementType measurementTypeFromString(const QString& value)
{
    if (value == "polyline")
    {
        return MeasurementType::Polyline;
    }
    if (value == "angle")
    {
        return MeasurementType::Angle;
    }
    if (value == "rectangle_roi")
    {
        return MeasurementType::RectangleRoi;
    }
    return MeasurementType::Distance;
}

QString pointsToJson(const QVector<MeasurementPoint>& points)
{
    QJsonArray pointArray;
    for (const MeasurementPoint& point : points)
    {
        QJsonObject object;
        object.insert("x", point.x);
        object.insert("y", point.y);
        object.insert("z", point.z);
        pointArray.append(object);
    }

    return QString::fromUtf8(QJsonDocument(pointArray).toJson(QJsonDocument::Compact));
}

QVector<MeasurementPoint> pointsFromJson(const QString& json)
{
    QVector<MeasurementPoint> points;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray())
    {
        return points;
    }

    const QJsonArray pointArray = document.array();
    points.reserve(pointArray.size());
    for (const QJsonValue& pointValue : pointArray)
    {
        if (!pointValue.isObject())
        {
            continue;
        }

        const QJsonObject object = pointValue.toObject();
        MeasurementPoint point;
        point.x = object.value("x").toDouble();
        point.y = object.value("y").toDouble();
        point.z = object.value("z").toDouble();
        points.append(point);
    }

    return points;
}

QVariant nullableDouble(const std::optional<double>& value)
{
    return value ? QVariant(*value) : QVariant(QMetaType(QMetaType::Double));
}

QVariant nullableInteger(const std::optional<int>& value)
{
    return value ? QVariant(*value) : QVariant(QMetaType(QMetaType::Int));
}

std::optional<double> optionalDoubleFromQuery(const QSqlQuery& query, const char* fieldName)
{
    const QVariant value = query.value(fieldName);
    if (!value.isValid() || value.isNull())
    {
        return std::nullopt;
    }

    return value.toDouble();
}
}

PostgreService::PostgreService(const DatabaseSettings& databaseSettings)
    : m_connection(std::make_unique<PostgreConnection>(databaseSettings))
{
}

PostgreService::PostgreService(std::unique_ptr<PostgreConnection> connection)
    : m_connection(std::move(connection))
{
}

PostgreService::~PostgreService() = default;

bool PostgreService::initialize()
{
    return ensureConnection() && createTables();
}

QString PostgreService::lastErrorText() const
{
    if (!m_connection)
    {
        return "PostgreSQL connection is not available.";
    }

    return m_connection->lastErrorText();
}

bool PostgreService::savePatient(const PatientPtr& patient)
{
    if (!patient || !ensureConnection())
    {
        return false;
    }

    QSqlDatabase database = m_connection->database();
    if (!database.transaction())
    {
        qWarning() << "Failed to start PostgreSQL transaction:" << database.lastError().text();
        return false;
    }

    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO patients (patient_id, patient_name, patient_sex, date_of_birth) "
        "VALUES (:patient_id, :patient_name, :patient_sex, :date_of_birth) "
        "ON CONFLICT (patient_id) DO UPDATE SET "
        "patient_name = EXCLUDED.patient_name, "
        "patient_sex = EXCLUDED.patient_sex, "
        "date_of_birth = EXCLUDED.date_of_birth");
    query.bindValue(":patient_id", patient->patientId());
    query.bindValue(":patient_name", patient->patientName());
    query.bindValue(":patient_sex", patient->patientSex());
    query.bindValue(":date_of_birth", patient->dateOfBirth());

    if (!query.exec())
    {
        qWarning() << "Failed to save patient:" << query.lastError().text();
        database.rollback();
        return false;
    }

    for (auto it = patient->studyMap().cbegin(); it != patient->studyMap().cend(); ++it)
    {
        if (!saveStudy(patient->patientId(), *it->second))
        {
            database.rollback();
            return false;
        }
    }

    return database.commit();
}

DatabaseService::PatientPtr PostgreService::getPatient(const QString& patientId)
{
    if (patientId.isEmpty() || !ensureConnection())
    {
        return nullptr;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT patient_id, patient_name, patient_sex, date_of_birth "
        "FROM patients WHERE patient_id = :patient_id");
    query.bindValue(":patient_id", patientId);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    auto patient = std::make_shared<Patient>();
    patient->setPatientId(query.value("patient_id").toString());
    patient->setPatientName(query.value("patient_name").toString());
    patient->setPatientSex(query.value("patient_sex").toString());
    patient->setDateOfBirth(query.value("date_of_birth").toString());
    populateStudies(*patient);
    return patient;
}

QList<DatabaseService::PatientPtr> PostgreService::getAllPatients(const QString& filterText)
{
    QList<PatientPtr> patients;
    if (!ensureConnection())
    {
        return patients;
    }

    QSqlQuery query(m_connection->database());
    const QString normalizedFilter = filterText.trimmed().toLower();
    if (normalizedFilter.isEmpty())
    {
        if (!query.exec("SELECT patient_id, patient_name, patient_sex, date_of_birth FROM patients ORDER BY patient_id"))
        {
            return patients;
        }
    }
    else
    {
        query.prepare(
            "SELECT DISTINCT p.patient_id, p.patient_name, p.patient_sex, p.date_of_birth "
            "FROM patients p "
            "LEFT JOIN studies s ON s.patient_id = p.patient_id "
            "LEFT JOIN series se ON se.study_instance_uid = s.study_instance_uid "
            "WHERE "
            "LOWER(COALESCE(p.patient_id, '')) LIKE :pattern OR "
            "LOWER(COALESCE(p.patient_name, '')) LIKE :pattern OR "
            "LOWER(COALESCE(p.date_of_birth, '')) LIKE :pattern OR "
            "LOWER(COALESCE(s.study_description, '')) LIKE :pattern OR "
            "LOWER(COALESCE(s.study_date, '')) LIKE :pattern OR "
            "LOWER(COALESCE(s.doctor_name, '')) LIKE :pattern OR "
            "LOWER(COALESCE(se.series_description, '')) LIKE :pattern OR "
            "LOWER(COALESCE(se.modality, '')) LIKE :pattern "
            "ORDER BY p.patient_id");
        query.bindValue(":pattern", "%" + normalizedFilter + "%");
        if (!query.exec())
        {
            return patients;
        }
    }

    while (query.next())
    {
        auto patient = std::make_shared<Patient>();
        patient->setPatientId(query.value("patient_id").toString());
        patient->setPatientName(query.value("patient_name").toString());
        patient->setPatientSex(query.value("patient_sex").toString());
        patient->setDateOfBirth(query.value("date_of_birth").toString());
        patients.append(patient);
    }

    return patients;
}

QList<DatabaseService::StudyPtr> PostgreService::getStudiesForPatient(const QString& patientId, const QString& filterText)
{
    QList<StudyPtr> studies;
    if (patientId.isEmpty() || !ensureConnection())
    {
        return studies;
    }

    QSqlQuery query(m_connection->database());
    const QString normalizedFilter = filterText.trimmed().toLower();
    if (normalizedFilter.isEmpty())
    {
        query.prepare(
            "SELECT study_instance_uid, study_description, study_date, doctor_name "
            "FROM studies WHERE patient_id = :patient_id ORDER BY study_date, study_instance_uid");
        query.bindValue(":patient_id", patientId);

        if (!query.exec())
        {
            return studies;
        }
    }
    else
    {
        query.prepare(
            "SELECT DISTINCT s.study_instance_uid, s.study_description, s.study_date, s.doctor_name "
            "FROM studies s "
            "LEFT JOIN series se ON se.study_instance_uid = s.study_instance_uid "
            "WHERE s.patient_id = :patient_id AND ("
            "LOWER(COALESCE(s.study_description, '')) LIKE :pattern OR "
            "LOWER(COALESCE(s.study_date, '')) LIKE :pattern OR "
            "LOWER(COALESCE(s.doctor_name, '')) LIKE :pattern OR "
            "LOWER(COALESCE(se.series_description, '')) LIKE :pattern OR "
            "LOWER(COALESCE(se.modality, '')) LIKE :pattern) "
            "ORDER BY s.study_date, s.study_instance_uid");
        query.bindValue(":patient_id", patientId);
        query.bindValue(":pattern", "%" + normalizedFilter + "%");

        if (!query.exec())
        {
            return studies;
        }
    }

    while (query.next())
    {
        auto study = std::make_shared<Study>();
        study->setStudyInstanceUid(query.value("study_instance_uid").toString());
        study->setStudyDescription(query.value("study_description").toString());
        study->setStudyDate(query.value("study_date").toString());
        study->setDoctorName(query.value("doctor_name").toString());
        studies.append(study);
    }

    return studies;
}

QList<DatabaseService::SeriesPtr> PostgreService::getSeriesForStudy(const QString& studyInstanceUid, const QString& filterText)
{
    QList<SeriesPtr> seriesList;
    if (studyInstanceUid.isEmpty() || !ensureConnection())
    {
        return seriesList;
    }

    QSqlQuery query(m_connection->database());
    const QString normalizedFilter = filterText.trimmed().toLower();
    QString queryText =
        "SELECT s.series_instance_uid, s.series_description, s.modality, s.series_number, "
        "(SELECT COUNT(*) FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_slice_metadata di "
        " WHERE di.series_instance_uid = s.series_instance_uid "
        " ORDER BY "
        " CASE WHEN instance_number ~ '^[0-9]+$' THEN instance_number::INTEGER END NULLS LAST, "
        " instance_number, sop_instance_uid "
        " LIMIT 1) AS representative_file_path "
        "FROM series s WHERE study_instance_uid = :study_instance_uid ";
    if (!normalizedFilter.isEmpty())
    {
        queryText +=
            "AND (LOWER(COALESCE(s.series_description, '')) LIKE :pattern OR "
            "LOWER(COALESCE(s.modality, '')) LIKE :pattern) ";
    }
    queryText += "ORDER BY series_number, series_instance_uid";

    query.prepare(queryText);
    query.bindValue(":study_instance_uid", studyInstanceUid);
    if (!normalizedFilter.isEmpty())
    {
        query.bindValue(":pattern", "%" + normalizedFilter + "%");
    }

    if (!query.exec())
    {
        return seriesList;
    }

    while (query.next())
    {
        auto series = std::make_shared<Series>();
        series->setSeriesInstanceUid(query.value("series_instance_uid").toString());
        series->setSeriesDescription(query.value("series_description").toString());
        series->setModality(query.value("modality").toString());
        series->setSeriesNumber(query.value("series_number").toString());
        series->setImageCount(query.value("image_count").toInt());
        series->setRepresentativeFilePath(query.value("representative_file_path").toString());

        seriesList.append(series);
    }

    return seriesList;
}

DatabaseService::StudyPtr PostgreService::getStudy(const QString& studyInstanceUid)
{
    if (studyInstanceUid.isEmpty() || !ensureConnection())
    {
        return nullptr;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT study_instance_uid, study_description, study_date, doctor_name "
        "FROM studies WHERE study_instance_uid = :study_instance_uid");
    query.bindValue(":study_instance_uid", studyInstanceUid);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    auto study = std::make_shared<Study>();
    study->setStudyInstanceUid(query.value("study_instance_uid").toString());
    study->setStudyDescription(query.value("study_description").toString());
    study->setStudyDate(query.value("study_date").toString());
    study->setDoctorName(query.value("doctor_name").toString());
    populateSeries(*study);
    return study;
}

DatabaseService::SeriesPtr PostgreService::getSeries(const QString& seriesInstanceUid)
{
    if (seriesInstanceUid.isEmpty() || !ensureConnection())
    {
        return nullptr;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT s.series_instance_uid, s.series_description, s.modality, s.series_number, s.preview_png, "
        "(SELECT COUNT(*) FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_slice_metadata di "
        " WHERE di.series_instance_uid = s.series_instance_uid "
        " ORDER BY "
        " CASE WHEN instance_number ~ '^[0-9]+$' THEN instance_number::INTEGER END NULLS LAST, "
        " instance_number, sop_instance_uid "
        " LIMIT 1) AS representative_file_path "
        "FROM series s WHERE s.series_instance_uid = :series_instance_uid");
    query.bindValue(":series_instance_uid", seriesInstanceUid);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    auto series = std::make_shared<Series>();
    series->setSeriesInstanceUid(query.value("series_instance_uid").toString());
    series->setSeriesDescription(query.value("series_description").toString());
    series->setModality(query.value("modality").toString());
    series->setSeriesNumber(query.value("series_number").toString());
    series->setImageCount(query.value("image_count").toInt());
    series->setRepresentativeFilePath(query.value("representative_file_path").toString());
    const QByteArray seriesPreviewPng = query.value("preview_png").toByteArray();
    if (!seriesPreviewPng.isEmpty())
    {
        QPixmap pixmap;
        pixmap.loadFromData(seriesPreviewPng, "PNG");
        series->setPreviewPixmap(pixmap);
    }
    populateImages(*series);
    return series;
}

DatabaseService::DicomImagePtr PostgreService::getImage(const QString& sopInstanceUid)
{
    if (sopInstanceUid.isEmpty() || !ensureConnection())
    {
        return nullptr;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT sop_instance_uid, file_path, instance_number, image_width, image_height "
        "FROM dicom_slice_metadata WHERE sop_instance_uid = :sop_instance_uid");
    query.bindValue(":sop_instance_uid", sopInstanceUid);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    return createImageFromQuery(query);
}

bool PostgreService::upsertSliceMeasurementAnnotation(const SliceMeasurementAnnotationRecord& record)
{
    if (record.seriesInstanceUid.isEmpty()
        || record.sopInstanceUid.isEmpty()
        || record.measurement.id.isEmpty()
        || !ensureConnection())
    {
        return false;
    }

    const QDateTime createdAtUtc = record.createdAtUtc.isValid()
        ? record.createdAtUtc.toUTC()
        : QDateTime::currentDateTimeUtc();
    const QDateTime updatedAtUtc = record.updatedAtUtc.isValid()
        ? record.updatedAtUtc.toUTC()
        : QDateTime::currentDateTimeUtc();

    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO measurement_annotations ("
        "annotation_id, series_instance_uid, sop_instance_uid, measurement_type, points_json, color_hex, "
        "length_mm, angle_degrees, area_mm2, sample_count, mean_value, stddev_value, min_value, max_value, "
        "created_at, updated_at, is_deleted) "
        "VALUES ("
        ":annotation_id, :series_instance_uid, :sop_instance_uid, :measurement_type, :points_json, :color_hex, "
        ":length_mm, :angle_degrees, :area_mm2, :sample_count, :mean_value, :stddev_value, :min_value, :max_value, "
        ":created_at, :updated_at, :is_deleted) "
        "ON CONFLICT (annotation_id) DO UPDATE SET "
        "series_instance_uid = EXCLUDED.series_instance_uid, "
        "sop_instance_uid = EXCLUDED.sop_instance_uid, "
        "measurement_type = EXCLUDED.measurement_type, "
        "points_json = EXCLUDED.points_json, "
        "color_hex = EXCLUDED.color_hex, "
        "length_mm = EXCLUDED.length_mm, "
        "angle_degrees = EXCLUDED.angle_degrees, "
        "area_mm2 = EXCLUDED.area_mm2, "
        "sample_count = EXCLUDED.sample_count, "
        "mean_value = EXCLUDED.mean_value, "
        "stddev_value = EXCLUDED.stddev_value, "
        "min_value = EXCLUDED.min_value, "
        "max_value = EXCLUDED.max_value, "
        "updated_at = EXCLUDED.updated_at, "
        "is_deleted = EXCLUDED.is_deleted");
    query.bindValue(":annotation_id", record.measurement.id);
    query.bindValue(":series_instance_uid", record.seriesInstanceUid);
    query.bindValue(":sop_instance_uid", record.sopInstanceUid);
    query.bindValue(":measurement_type", measurementTypeToString(record.measurement.type));
    query.bindValue(":points_json", pointsToJson(record.measurement.points));
    query.bindValue(":color_hex", record.measurement.color.name(QColor::HexRgb));
    query.bindValue(":length_mm", record.measurement.lengthMm);
    query.bindValue(":angle_degrees", nullableDouble(record.angleDegrees));
    query.bindValue(
        ":area_mm2",
        record.roiStatistics ? QVariant(record.roiStatistics->areaMm2) : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(
        ":sample_count",
        record.roiStatistics
            ? QVariant(record.roiStatistics->sampleCount)
            : nullableInteger(std::nullopt));
    query.bindValue(
        ":mean_value",
        record.roiStatistics ? QVariant(record.roiStatistics->mean) : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(
        ":stddev_value",
        record.roiStatistics
            ? QVariant(record.roiStatistics->standardDeviation)
            : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(
        ":min_value",
        record.roiStatistics ? QVariant(record.roiStatistics->minimum) : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(
        ":max_value",
        record.roiStatistics ? QVariant(record.roiStatistics->maximum) : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(":created_at", createdAtUtc);
    query.bindValue(":updated_at", updatedAtUtc);
    query.bindValue(":is_deleted", record.deleted);

    if (!query.exec())
    {
        qWarning() << "Failed to save measurement annotation:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<SliceMeasurementAnnotationRecord> PostgreService::loadSliceMeasurementAnnotations(const QString& sopInstanceUid)
{
    QList<SliceMeasurementAnnotationRecord> records;
    if (sopInstanceUid.isEmpty() || !ensureConnection())
    {
        return records;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT annotation_id, series_instance_uid, sop_instance_uid, measurement_type, points_json, color_hex, "
        "length_mm, angle_degrees, area_mm2, sample_count, mean_value, stddev_value, min_value, max_value, "
        "created_at, updated_at, is_deleted "
        "FROM measurement_annotations "
        "WHERE sop_instance_uid = :sop_instance_uid AND is_deleted = FALSE "
        "ORDER BY created_at, annotation_id");
    query.bindValue(":sop_instance_uid", sopInstanceUid);

    if (!query.exec())
    {
        qWarning() << "Failed to load measurement annotations:" << query.lastError().text();
        return records;
    }

    while (query.next())
    {
        SliceMeasurementAnnotationRecord record;
        record.seriesInstanceUid = query.value("series_instance_uid").toString();
        record.sopInstanceUid = query.value("sop_instance_uid").toString();
        record.measurement.id = query.value("annotation_id").toString();
        record.measurement.type = measurementTypeFromString(query.value("measurement_type").toString());
        record.measurement.points = pointsFromJson(query.value("points_json").toString());
        record.measurement.color = QColor(query.value("color_hex").toString());
        record.measurement.lengthMm = query.value("length_mm").toDouble();
        record.angleDegrees = optionalDoubleFromQuery(query, "angle_degrees");
        record.createdAtUtc = query.value("created_at").toDateTime().toUTC();
        record.updatedAtUtc = query.value("updated_at").toDateTime().toUTC();
        record.deleted = query.value("is_deleted").toBool();

        const QVariant sampleCountValue = query.value("sample_count");
        if (sampleCountValue.isValid() && !sampleCountValue.isNull())
        {
            RoiStatistics stats;
            stats.valid = true;
            stats.sampleCount = sampleCountValue.toInt();
            stats.mean = query.value("mean_value").toDouble();
            stats.standardDeviation = query.value("stddev_value").toDouble();
            stats.minimum = query.value("min_value").toDouble();
            stats.maximum = query.value("max_value").toDouble();
            stats.areaMm2 = query.value("area_mm2").toDouble();
            record.roiStatistics = stats;
        }

        records.append(record);
    }

    return records;
}

bool PostgreService::markSliceMeasurementAnnotationDeleted(const QString& annotationId)
{
    if (annotationId.isEmpty() || !ensureConnection())
    {
        return false;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "UPDATE measurement_annotations "
        "SET is_deleted = TRUE, updated_at = :updated_at "
        "WHERE annotation_id = :annotation_id");
    query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
    query.bindValue(":annotation_id", annotationId);

    if (!query.exec())
    {
        qWarning() << "Failed to soft-delete measurement annotation:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

QPixmap PostgreService::getPreviewForPatient(const QString& patientId)
{
    if (patientId.isEmpty() || !ensureConnection())
    {
        return QPixmap();
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT se.preview_png "
        "FROM series se "
        "JOIN studies s ON s.study_instance_uid = se.study_instance_uid "
        "WHERE s.patient_id = :patient_id AND se.preview_png IS NOT NULL "
        "ORDER BY s.study_date, se.series_number, se.series_instance_uid "
        "LIMIT 1");
    query.bindValue(":patient_id", patientId);

    if (!query.exec() || !query.next())
    {
        return QPixmap();
    }

    QPixmap pixmap;
    pixmap.loadFromData(query.value(0).toByteArray(), "PNG");
    return pixmap;
}

QPixmap PostgreService::getPreviewForStudy(const QString& studyInstanceUid)
{
    if (studyInstanceUid.isEmpty() || !ensureConnection())
    {
        return QPixmap();
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT preview_png "
        "FROM series "
        "WHERE study_instance_uid = :study_instance_uid AND preview_png IS NOT NULL "
        "ORDER BY series_number, series_instance_uid "
        "LIMIT 1");
    query.bindValue(":study_instance_uid", studyInstanceUid);

    if (!query.exec() || !query.next())
    {
        return QPixmap();
    }

    QPixmap pixmap;
    pixmap.loadFromData(query.value(0).toByteArray(), "PNG");
    return pixmap;
}

QPixmap PostgreService::getPreviewForSeries(const QString& seriesInstanceUid)
{
    if (seriesInstanceUid.isEmpty() || !ensureConnection())
    {
        return QPixmap();
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT preview_png "
        "FROM series "
        "WHERE series_instance_uid = :series_instance_uid");
    query.bindValue(":series_instance_uid", seriesInstanceUid);

    if (!query.exec() || !query.next())
    {
        return QPixmap();
    }

    QPixmap pixmap;
    pixmap.loadFromData(query.value(0).toByteArray(), "PNG");
    return pixmap;
}

bool PostgreService::ensureConnection()
{
    if (!m_connection)
    {
        return false;
    }

    if (m_connection->database().isOpen())
    {
        m_connectionAttempted = true;
        m_connectionAvailable = true;
        return true;
    }

    if (m_connectionAttempted)
    {
        return m_connectionAvailable;
    }

    m_connectionAttempted = true;
    if (!m_connection->openDB())
    {
        m_connectionAvailable = false;
        return false;
    }

    m_connectionAvailable = true;
    return true;
}

bool PostgreService::createTables()
{
    static const char* tableStatements[] = {
        "CREATE TABLE IF NOT EXISTS patients ("
        "patient_id TEXT PRIMARY KEY,"
        "patient_name TEXT,"
        "patient_sex TEXT,"
        "date_of_birth TEXT"
        ")",
        "CREATE TABLE IF NOT EXISTS studies ("
        "study_instance_uid TEXT PRIMARY KEY,"
        "patient_id TEXT NOT NULL REFERENCES patients(patient_id) ON DELETE CASCADE,"
        "study_description TEXT,"
        "study_date TEXT,"
        "doctor_name TEXT"
        ")",
        "CREATE TABLE IF NOT EXISTS series ("
        "series_instance_uid TEXT PRIMARY KEY,"
        "study_instance_uid TEXT NOT NULL REFERENCES studies(study_instance_uid) ON DELETE CASCADE,"
        "series_description TEXT,"
        "modality TEXT,"
        "series_number TEXT,"
        "preview_png BYTEA"
        ")",
        "CREATE TABLE IF NOT EXISTS dicom_slice_metadata ("
        "sop_instance_uid TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL REFERENCES series(series_instance_uid) ON DELETE CASCADE,"
        "file_path TEXT NOT NULL UNIQUE,"
        "instance_number TEXT,"
        "image_width INTEGER NOT NULL,"
        "image_height INTEGER NOT NULL"
        ")",
        "CREATE TABLE IF NOT EXISTS measurement_annotations ("
        "annotation_id TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL REFERENCES series(series_instance_uid) ON DELETE CASCADE,"
        "sop_instance_uid TEXT NOT NULL REFERENCES dicom_slice_metadata(sop_instance_uid) ON DELETE CASCADE,"
        "measurement_type TEXT NOT NULL,"
        "points_json TEXT NOT NULL,"
        "color_hex TEXT NOT NULL,"
        "length_mm DOUBLE PRECISION,"
        "angle_degrees DOUBLE PRECISION,"
        "area_mm2 DOUBLE PRECISION,"
        "sample_count INTEGER,"
        "mean_value DOUBLE PRECISION,"
        "stddev_value DOUBLE PRECISION,"
        "min_value DOUBLE PRECISION,"
        "max_value DOUBLE PRECISION,"
        "created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),"
        "updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),"
        "is_deleted BOOLEAN NOT NULL DEFAULT FALSE"
        ")"
    };

    for (const char* statement : tableStatements)
    {
        QSqlQuery query(m_connection->database());
        if (!query.exec(QString::fromUtf8(statement)))
        {
            qWarning() << "Failed to create PostgreSQL table:" << query.lastError().text();
            return false;
        }
    }

    static const char* indexStatements[] = {
        "CREATE INDEX IF NOT EXISTS idx_dicom_slice_metadata_series_instance_uid "
        "ON dicom_slice_metadata(series_instance_uid)",
        "CREATE INDEX IF NOT EXISTS idx_measurement_annotations_sop_instance_uid "
        "ON measurement_annotations(sop_instance_uid)",
        "CREATE INDEX IF NOT EXISTS idx_measurement_annotations_series_sop "
        "ON measurement_annotations(series_instance_uid, sop_instance_uid)"
    };

    for (const char* statement : indexStatements)
    {
        QSqlQuery query(m_connection->database());
        if (!query.exec(QString::fromUtf8(statement)))
        {
            qWarning() << "Failed to create PostgreSQL index:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool PostgreService::saveStudy(const QString& patientId, const Study& study)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO studies (study_instance_uid, patient_id, study_description, study_date, doctor_name) "
        "VALUES (:study_instance_uid, :patient_id, :study_description, :study_date, :doctor_name) "
        "ON CONFLICT (study_instance_uid) DO UPDATE SET "
        "patient_id = EXCLUDED.patient_id, "
        "study_description = EXCLUDED.study_description, "
        "study_date = EXCLUDED.study_date, "
        "doctor_name = EXCLUDED.doctor_name");
    query.bindValue(":study_instance_uid", study.studyInstanceUid());
    query.bindValue(":patient_id", patientId);
    query.bindValue(":study_description", study.studyDescription());
    query.bindValue(":study_date", study.studyDate());
    query.bindValue(":doctor_name", study.doctorName());

    if (!query.exec())
    {
        qWarning() << "Failed to save study:" << query.lastError().text();
        return false;
    }

    for (auto it = study.seriesMap().cbegin(); it != study.seriesMap().cend(); ++it)
    {
        if (!saveSeries(study.studyInstanceUid(), *it->second))
        {
            return false;
        }
    }

    return true;
}

bool PostgreService::saveSeries(const QString& studyInstanceUid, const Series& series)
{
    QByteArray previewPng;
    const QPixmap previewPixmap = createSeriesPreviewPixmap(series);
    if (!previewPixmap.isNull())
    {
        QBuffer buffer(&previewPng);
        buffer.open(QIODevice::WriteOnly);
        previewPixmap.save(&buffer, "PNG");
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO series (series_instance_uid, study_instance_uid, series_description, modality, series_number, preview_png) "
        "VALUES (:series_instance_uid, :study_instance_uid, :series_description, :modality, :series_number, :preview_png) "
        "ON CONFLICT (series_instance_uid) DO UPDATE SET "
        "study_instance_uid = EXCLUDED.study_instance_uid, "
        "series_description = EXCLUDED.series_description, "
        "modality = EXCLUDED.modality, "
        "series_number = EXCLUDED.series_number, "
        "preview_png = EXCLUDED.preview_png");
    query.bindValue(":series_instance_uid", series.seriesInstanceUid());
    query.bindValue(":study_instance_uid", studyInstanceUid);
    query.bindValue(":series_description", series.seriesDescription());
    query.bindValue(":modality", series.modality());
    query.bindValue(":series_number", series.seriesNumber());
    query.bindValue(":preview_png", previewPng);

    if (!query.exec())
    {
        qWarning() << "Failed to save series:" << query.lastError().text();
        return false;
    }

    for (const auto& image : series.images())
    {
        if (image && !saveImage(series.seriesInstanceUid(), *image))
        {
            return false;
        }
    }

    return true;
}

bool PostgreService::saveImage(const QString& seriesInstanceUid, const DicomImage& image)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO dicom_slice_metadata (sop_instance_uid, series_instance_uid, file_path, instance_number, image_width, image_height) "
        "VALUES (:sop_instance_uid, :series_instance_uid, :file_path, :instance_number, :image_width, :image_height) "
        "ON CONFLICT (sop_instance_uid) DO UPDATE SET "
        "series_instance_uid = EXCLUDED.series_instance_uid, "
        "file_path = EXCLUDED.file_path, "
        "instance_number = EXCLUDED.instance_number, "
        "image_width = EXCLUDED.image_width, "
        "image_height = EXCLUDED.image_height");
    query.bindValue(":sop_instance_uid", image.sopInstanceUid());
    query.bindValue(":series_instance_uid", seriesInstanceUid);
    query.bindValue(":file_path", image.filePath());
    query.bindValue(":instance_number", image.instanceNumber());
    query.bindValue(":image_width", image.width());
    query.bindValue(":image_height", image.height());

    if (!query.exec())
    {
        qWarning() << "Failed to save DICOM image:" << query.lastError().text();
        return false;
    }

    return true;
}

void PostgreService::populateStudies(Patient& patient)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT study_instance_uid, study_description, study_date, doctor_name "
        "FROM studies WHERE patient_id = :patient_id ORDER BY study_date, study_instance_uid");
    query.bindValue(":patient_id", patient.patientId());

    if (!query.exec())
    {
        return;
    }

    while (query.next())
    {
        Study& study = patient.getOrCreateStudy(query.value("study_instance_uid").toString());
        study.setStudyDescription(query.value("study_description").toString());
        study.setStudyDate(query.value("study_date").toString());
        study.setDoctorName(query.value("doctor_name").toString());
        populateSeries(study);
    }
}

void PostgreService::populateSeries(Study& study)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT s.series_instance_uid, s.series_description, s.modality, s.series_number, s.preview_png, "
        "(SELECT COUNT(*) FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_slice_metadata di "
        " WHERE di.series_instance_uid = s.series_instance_uid "
        " ORDER BY "
        " CASE WHEN instance_number ~ '^[0-9]+$' THEN instance_number::INTEGER END NULLS LAST, "
        " instance_number, sop_instance_uid "
        " LIMIT 1) AS representative_file_path "
        "FROM series s WHERE study_instance_uid = :study_instance_uid ORDER BY series_number, series_instance_uid");
    query.bindValue(":study_instance_uid", study.studyInstanceUid());

    if (!query.exec())
    {
        return;
    }

    while (query.next())
    {
        Series& series = study.getOrCreateSeries(query.value("series_instance_uid").toString());
        series.setSeriesDescription(query.value("series_description").toString());
        series.setModality(query.value("modality").toString());
        series.setSeriesNumber(query.value("series_number").toString());
        series.setImageCount(query.value("image_count").toInt());
        series.setRepresentativeFilePath(query.value("representative_file_path").toString());

        const QByteArray previewPng = query.value("preview_png").toByteArray();
        if (!previewPng.isEmpty())
        {
            QPixmap pixmap;
            pixmap.loadFromData(previewPng, "PNG");
            series.setPreviewPixmap(pixmap);
        }
    }
}

void PostgreService::populateImages(Series& series)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT sop_instance_uid, file_path, instance_number, image_width, image_height "
        "FROM dicom_slice_metadata WHERE series_instance_uid = :series_instance_uid "
        "ORDER BY "
        "CASE WHEN instance_number ~ '^[0-9]+$' THEN instance_number::INTEGER END NULLS LAST, "
        "instance_number, "
        "sop_instance_uid");
    query.bindValue(":series_instance_uid", series.seriesInstanceUid());

    if (!query.exec())
    {
        return;
    }

    while (query.next())
    {
        DicomImagePtr image = createImageFromQuery(query);
        if (image)
        {
            series.addImage(std::make_unique<DicomImage>(*image));
        }
    }
}

DatabaseService::DicomImagePtr PostgreService::createImageFromQuery(const QSqlQuery& query) const
{
    auto image = std::make_shared<DicomImage>();
    image->setSopInstanceUid(query.value("sop_instance_uid").toString());
    image->setFilePath(query.value("file_path").toString());
    image->setInstanceNumber(query.value("instance_number").toString());
    image->setDimensions(query.value("image_width").toInt(), query.value("image_height").toInt());

    return image;
}

QPixmap PostgreService::createSeriesPreviewPixmap(const Series& series) const
{
    if (!series.previewPixmap().isNull())
    {
        return series.previewPixmap();
    }

    const auto& images = series.images();
    if (!images.empty() && images.front())
    {
        return images.front()->pixmap();
    }

    return QPixmap();
}
