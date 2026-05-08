#include "Database/SqliteService.h"

#include "Database/SqliteConnection.h"

#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

#include <algorithm>

namespace
{
constexpr auto kSliceOrderClause =
    "CASE WHEN instance_number IS NOT NULL "
    "AND instance_number GLOB '[0-9]*' "
    "AND instance_number NOT GLOB '*[^0-9]*' "
    "THEN 0 ELSE 1 END, "
    "CASE WHEN instance_number IS NOT NULL "
    "AND instance_number GLOB '[0-9]*' "
    "AND instance_number NOT GLOB '*[^0-9]*' "
    "THEN CAST(instance_number AS INTEGER) END, "
    "instance_number, sop_instance_uid";

constexpr auto kFrameCountSumExpression =
    "COALESCE(SUM(CASE WHEN frame_count IS NOT NULL AND frame_count > 0 THEN frame_count ELSE 1 END), 0)";

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

QString measurementTypeDisplayName(MeasurementType type)
{
    switch (type)
    {
    case MeasurementType::Distance:
        return "Distance";
    case MeasurementType::Polyline:
        return "Polyline";
    case MeasurementType::Angle:
        return "Angle";
    case MeasurementType::RectangleRoi:
        return "ROI";
    }

    return "Measurement";
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

QString normalizedBodyRegion(const QString& value)
{
    const QString normalizedValue = value.trimmed();
    return normalizedValue.isEmpty() ? QString("Other") : normalizedValue;
}

QString defaultAnnotationLabel(MeasurementType type)
{
    return QString("%1 annotation").arg(measurementTypeDisplayName(type));
}

QString reportDisplayValue(const QSqlQuery& query, MeasurementType type)
{
    switch (type)
    {
    case MeasurementType::Distance:
    case MeasurementType::Polyline: {
        const QVariant value = query.value("length_mm");
        return value.isValid() && !value.isNull()
            ? QString("%1 mm").arg(value.toDouble(), 0, 'f', 1)
            : QString();
    }
    case MeasurementType::Angle: {
        const QVariant value = query.value("angle_degrees");
        return value.isValid() && !value.isNull()
            ? QString("%1 deg").arg(value.toDouble(), 0, 'f', 1)
            : QString();
    }
    case MeasurementType::RectangleRoi: {
        const QVariant mean = query.value("mean_value");
        const QVariant sampleCount = query.value("sample_count");
        if (!mean.isValid() || mean.isNull())
        {
            return {};
        }
        if (sampleCount.isValid() && !sampleCount.isNull())
        {
            return QString("mean %1, n=%2").arg(mean.toDouble(), 0, 'f', 1).arg(sampleCount.toInt());
        }
        return QString("mean %1").arg(mean.toDouble(), 0, 'f', 1);
    }
    }

    return {};
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

QString dateTimeToDatabase(const QDateTime& value)
{
    const QDateTime normalizedValue = value.isValid() ? value.toUTC() : QDateTime::currentDateTimeUtc();
    return normalizedValue.toString(Qt::ISODateWithMs);
}

QDateTime dateTimeFromQuery(const QSqlQuery& query, const char* fieldName)
{
    const QDateTime value = QDateTime::fromString(query.value(fieldName).toString(), Qt::ISODateWithMs);
    return value.isValid() ? value.toUTC() : QDateTime();
}

QString countText(int count, const QString& singular, const QString& plural)
{
    return QString("%1 %2").arg(count).arg(count == 1 ? singular : plural);
}
}

SqliteService::SqliteService(const DatabaseSettings& databaseSettings)
    : m_connection(std::make_unique<SqliteConnection>(databaseSettings))
{
}

SqliteService::SqliteService(std::unique_ptr<SqliteConnection> connection)
    : m_connection(std::move(connection))
{
}

SqliteService::~SqliteService() = default;

bool SqliteService::initialize()
{
    return ensureConnection() && createTables();
}

QString SqliteService::lastErrorText() const
{
    if (!m_connection)
    {
        return "SQLite connection is not available.";
    }

    return m_connection->lastErrorText();
}

bool SqliteService::savePatient(const PatientPtr& patient)
{
    if (!patient || !ensureConnection())
    {
        return false;
    }

    QSqlDatabase database = m_connection->database();
    if (!database.transaction())
    {
        qWarning() << "Failed to start SQLite transaction:" << database.lastError().text();
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

DatabaseService::PatientPtr SqliteService::getPatient(const QString& patientId)
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

QList<DatabaseService::PatientPtr> SqliteService::getAllPatients(const QString& filterText)
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

QList<DatabaseService::StudyPtr> SqliteService::getStudiesForPatient(const QString& patientId, const QString& filterText)
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

QList<DatabaseService::SeriesPtr> SqliteService::getSeriesForStudy(const QString& studyInstanceUid, const QString& filterText)
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
        "(SELECT " + QString::fromUtf8(kFrameCountSumExpression) + " FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_slice_metadata di "
        " WHERE di.series_instance_uid = s.series_instance_uid "
        " ORDER BY " + QString::fromUtf8(kSliceOrderClause) + " "
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

DatabaseService::StudyPtr SqliteService::getStudy(const QString& studyInstanceUid)
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

DatabaseService::SeriesPtr SqliteService::getSeries(const QString& seriesInstanceUid)
{
    if (seriesInstanceUid.isEmpty() || !ensureConnection())
    {
        return nullptr;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT s.series_instance_uid, s.series_description, s.modality, s.series_number, s.preview_png, "
        "(SELECT " + QString::fromUtf8(kFrameCountSumExpression) + " FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_slice_metadata di "
        " WHERE di.series_instance_uid = s.series_instance_uid "
        " ORDER BY " + QString::fromUtf8(kSliceOrderClause) + " "
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

DatabaseService::DicomImagePtr SqliteService::getImage(const QString& sopInstanceUid)
{
    if (sopInstanceUid.isEmpty() || !ensureConnection())
    {
        return nullptr;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT sop_instance_uid, file_path, instance_number, frame_count, frame_time_ms, cine_rate_fps, frame_interval_ms, image_width, image_height "
        "FROM dicom_slice_metadata WHERE sop_instance_uid = :sop_instance_uid");
    query.bindValue(":sop_instance_uid", sopInstanceUid);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    return createImageFromQuery(query);
}

bool SqliteService::upsertSliceMeasurementAnnotation(const SliceMeasurementAnnotationRecord& record)
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
    const QString measurementType = measurementTypeToString(record.measurement.type);
    const QString label = record.label.trimmed().isEmpty()
        ? defaultAnnotationLabel(record.measurement.type)
        : record.label.trimmed();
    const QString bodyRegion = normalizedBodyRegion(record.bodyRegion);
    query.prepare(
        "INSERT INTO measurement_annotations ("
        "annotation_id, series_instance_uid, sop_instance_uid, frame_index, measurement_type, points_json, color_hex, "
        "length_mm, angle_degrees, area_mm2, sample_count, mean_value, stddev_value, min_value, max_value, "
        "label, body_region, note, created_at, updated_at, is_deleted) "
        "VALUES ("
        ":annotation_id, :series_instance_uid, :sop_instance_uid, :frame_index, :measurement_type, :points_json, :color_hex, "
        ":length_mm, :angle_degrees, :area_mm2, :sample_count, :mean_value, :stddev_value, :min_value, :max_value, "
        ":label, :body_region, :note, :created_at, :updated_at, :is_deleted) "
        "ON CONFLICT (annotation_id) DO UPDATE SET "
        "series_instance_uid = EXCLUDED.series_instance_uid, "
        "sop_instance_uid = EXCLUDED.sop_instance_uid, "
        "frame_index = EXCLUDED.frame_index, "
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
        "label = CASE "
        "WHEN measurement_annotations.label IS NULL OR TRIM(measurement_annotations.label) = '' "
        "THEN EXCLUDED.label ELSE measurement_annotations.label END, "
        "body_region = CASE "
        "WHEN EXCLUDED.body_region IS NULL OR TRIM(EXCLUDED.body_region) = '' OR EXCLUDED.body_region = 'Other' "
        "THEN measurement_annotations.body_region ELSE EXCLUDED.body_region END, "
        "note = CASE "
        "WHEN EXCLUDED.note IS NULL OR TRIM(EXCLUDED.note) = '' "
        "THEN measurement_annotations.note ELSE EXCLUDED.note END, "
        "updated_at = EXCLUDED.updated_at, "
        "is_deleted = EXCLUDED.is_deleted");
    query.bindValue(":annotation_id", record.measurement.id);
    query.bindValue(":series_instance_uid", record.seriesInstanceUid);
    query.bindValue(":sop_instance_uid", record.sopInstanceUid);
    query.bindValue(":frame_index", std::max(0, record.frameIndex));
    query.bindValue(":measurement_type", measurementType);
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
    query.bindValue(":label", label);
    query.bindValue(":body_region", bodyRegion);
    query.bindValue(":note", record.note.trimmed());
    query.bindValue(":created_at", dateTimeToDatabase(createdAtUtc));
    query.bindValue(":updated_at", dateTimeToDatabase(updatedAtUtc));
    query.bindValue(":is_deleted", record.deleted);

    if (!query.exec())
    {
        qWarning() << "Failed to save measurement annotation:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<SliceMeasurementAnnotationRecord> SqliteService::loadSliceMeasurementAnnotations(
    const QString& sopInstanceUid,
    int frameIndex)
{
    QList<SliceMeasurementAnnotationRecord> records;
    if (sopInstanceUid.isEmpty() || !ensureConnection())
    {
        return records;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT annotation_id, series_instance_uid, sop_instance_uid, frame_index, measurement_type, points_json, color_hex, "
        "length_mm, angle_degrees, area_mm2, sample_count, mean_value, stddev_value, min_value, max_value, "
        "label, body_region, note, created_at, updated_at, is_deleted "
        "FROM measurement_annotations "
        "WHERE sop_instance_uid = :sop_instance_uid AND frame_index = :frame_index AND is_deleted = 0 "
        "ORDER BY created_at, annotation_id");
    query.bindValue(":sop_instance_uid", sopInstanceUid);
    query.bindValue(":frame_index", std::max(0, frameIndex));

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
        record.frameIndex = query.value("frame_index").toInt();
        record.label = query.value("label").toString();
        record.bodyRegion = query.value("body_region").toString();
        record.note = query.value("note").toString();
        record.measurement.id = query.value("annotation_id").toString();
        record.measurement.type = measurementTypeFromString(query.value("measurement_type").toString());
        record.measurement.points = pointsFromJson(query.value("points_json").toString());
        record.measurement.color = QColor(query.value("color_hex").toString());
        record.measurement.lengthMm = query.value("length_mm").toDouble();
        record.angleDegrees = optionalDoubleFromQuery(query, "angle_degrees");
        record.createdAtUtc = dateTimeFromQuery(query, "created_at");
        record.updatedAtUtc = dateTimeFromQuery(query, "updated_at");
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

bool SqliteService::markSliceMeasurementAnnotationDeleted(const QString& annotationId)
{
    if (annotationId.isEmpty() || !ensureConnection())
    {
        return false;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "UPDATE measurement_annotations "
        "SET is_deleted = 1, updated_at = :updated_at "
        "WHERE annotation_id = :annotation_id");
    query.bindValue(":updated_at", dateTimeToDatabase(QDateTime::currentDateTimeUtc()));
    query.bindValue(":annotation_id", annotationId);

    if (!query.exec())
    {
        qWarning() << "Failed to soft-delete measurement annotation:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

AnnotationReportSummaryBySeries SqliteService::loadSeriesAnnotationReportSummaries(
    const QList<QString>& seriesInstanceUids)
{
    AnnotationReportSummaryBySeries summaries;
    if (seriesInstanceUids.isEmpty() || !ensureConnection())
    {
        return summaries;
    }

    QList<QString> uniqueSeriesUids;
    QSet<QString> seenSeriesUids;
    uniqueSeriesUids.reserve(seriesInstanceUids.size());
    for (const QString& seriesInstanceUid : seriesInstanceUids)
    {
        const QString normalizedUid = seriesInstanceUid.trimmed();
        if (normalizedUid.isEmpty() || seenSeriesUids.contains(normalizedUid))
        {
            continue;
        }

        seenSeriesUids.insert(normalizedUid);
        uniqueSeriesUids.append(normalizedUid);
    }

    if (uniqueSeriesUids.isEmpty())
    {
        return summaries;
    }

    QStringList placeholders;
    placeholders.reserve(uniqueSeriesUids.size());
    for (int index = 0; index < uniqueSeriesUids.size(); ++index)
    {
        placeholders.append(QString(":series_%1").arg(index));
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        QString(
            "SELECT series_instance_uid, "
            "COUNT(*) AS annotation_count, "
            "COUNT(DISTINCT sop_instance_uid || '#' || frame_index) AS annotated_slice_count, "
            "SUM(CASE WHEN measurement_type = 'distance' THEN 1 ELSE 0 END) AS distance_count, "
            "SUM(CASE WHEN measurement_type = 'polyline' THEN 1 ELSE 0 END) AS polyline_count, "
            "SUM(CASE WHEN measurement_type = 'angle' THEN 1 ELSE 0 END) AS angle_count, "
            "SUM(CASE WHEN measurement_type = 'rectangle_roi' THEN 1 ELSE 0 END) AS rectangle_roi_count "
            "FROM measurement_annotations "
            "WHERE is_deleted = 0 AND series_instance_uid IN (%1) "
            "GROUP BY series_instance_uid")
            .arg(placeholders.join(", ")));

    for (int index = 0; index < uniqueSeriesUids.size(); ++index)
    {
        query.bindValue(placeholders.at(index), uniqueSeriesUids.at(index));
    }

    if (!query.exec())
    {
        qWarning() << "Failed to load annotation report summaries:" << query.lastError().text();
        return summaries;
    }

    while (query.next())
    {
        AnnotationReportSummary summary;
        summary.seriesInstanceUid = query.value("series_instance_uid").toString();
        summary.annotationCount = query.value("annotation_count").toInt();
        summary.annotatedSliceCount = query.value("annotated_slice_count").toInt();
        summary.distanceCount = query.value("distance_count").toInt();
        summary.polylineCount = query.value("polyline_count").toInt();
        summary.angleCount = query.value("angle_count").toInt();
        summary.rectangleRoiCount = query.value("rectangle_roi_count").toInt();
        summaries.insert(summary.seriesInstanceUid, summary);
    }

    return summaries;
}

AnnotationReportRows SqliteService::loadAnnotationReportRows(const AnnotationReportFilter& filter)
{
    AnnotationReportRows rows;
    if (!ensureConnection())
    {
        return rows;
    }

    QString queryText =
        "SELECT ma.annotation_id, ma.label, ma.body_region, ma.note, ma.measurement_type, "
        "ma.length_mm, ma.angle_degrees, ma.mean_value, ma.sample_count, ma.updated_at, "
        "ma.series_instance_uid, ma.sop_instance_uid, ma.frame_index, di.instance_number, "
        "se.series_description, se.modality, st.study_date, p.patient_id, p.patient_name "
        "FROM measurement_annotations ma "
        "JOIN dicom_slice_metadata di ON di.sop_instance_uid = ma.sop_instance_uid "
        "JOIN series se ON se.series_instance_uid = ma.series_instance_uid "
        "JOIN studies st ON st.study_instance_uid = se.study_instance_uid "
        "JOIN patients p ON p.patient_id = st.patient_id "
        "WHERE ma.is_deleted = 0 ";

    const QString bodyRegionFilter = filter.bodyRegion.trimmed();
    const QString normalizedMeasurementType = filter.measurementType.trimmed();
    const QString normalizedSearchText = filter.searchText.trimmed().toLower();
    const QString normalizedSeriesInstanceUid = filter.seriesInstanceUid.trimmed();
    const QString normalizedSopInstanceUid = filter.sopInstanceUid.trimmed();
    if (!normalizedSeriesInstanceUid.isEmpty())
    {
        queryText += "AND ma.series_instance_uid = :series_instance_uid ";
    }
    if (!normalizedSopInstanceUid.isEmpty())
    {
        queryText += "AND ma.sop_instance_uid = :sop_instance_uid ";
    }
    if (filter.frameIndex >= 0)
    {
        queryText += "AND ma.frame_index = :frame_index ";
    }
    if (!bodyRegionFilter.isEmpty() && bodyRegionFilter != "All")
    {
        queryText += "AND ma.body_region = :body_region ";
    }
    if (!normalizedMeasurementType.isEmpty() && normalizedMeasurementType != "all")
    {
        queryText += "AND ma.measurement_type = :measurement_type ";
    }
    if (!normalizedSearchText.isEmpty())
    {
        queryText +=
            "AND (LOWER(COALESCE(ma.label, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(ma.note, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(ma.body_region, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(di.instance_number, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(p.patient_name, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(p.patient_id, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(se.series_description, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(se.modality, '')) LIKE :search_pattern OR "
            "LOWER(COALESCE(st.study_date, '')) LIKE :search_pattern) ";
    }
    queryText +=
        "ORDER BY st.study_date, se.series_instance_uid, "
        "CASE WHEN di.instance_number IS NOT NULL "
        "AND di.instance_number GLOB '[0-9]*' "
        "AND di.instance_number NOT GLOB '*[^0-9]*' "
        "THEN 0 ELSE 1 END, "
        "CASE WHEN di.instance_number IS NOT NULL "
        "AND di.instance_number GLOB '[0-9]*' "
        "AND di.instance_number NOT GLOB '*[^0-9]*' "
        "THEN CAST(di.instance_number AS INTEGER) END, "
        "di.instance_number, ma.frame_index, ma.updated_at DESC, ma.annotation_id LIMIT :limit";

    QSqlQuery query(m_connection->database());
    query.prepare(queryText);
    if (!normalizedSeriesInstanceUid.isEmpty())
    {
        query.bindValue(":series_instance_uid", normalizedSeriesInstanceUid);
    }
    if (!normalizedSopInstanceUid.isEmpty())
    {
        query.bindValue(":sop_instance_uid", normalizedSopInstanceUid);
    }
    if (filter.frameIndex >= 0)
    {
        query.bindValue(":frame_index", filter.frameIndex);
    }
    if (!bodyRegionFilter.isEmpty() && bodyRegionFilter != "All")
    {
        query.bindValue(":body_region", bodyRegionFilter);
    }
    if (!normalizedMeasurementType.isEmpty() && normalizedMeasurementType != "all")
    {
        query.bindValue(":measurement_type", normalizedMeasurementType);
    }
    if (!normalizedSearchText.isEmpty())
    {
        query.bindValue(":search_pattern", "%" + normalizedSearchText + "%");
    }
    query.bindValue(":limit", std::clamp(filter.limit, 1, 500));

    if (!query.exec())
    {
        qWarning() << "Failed to load annotation report rows:" << query.lastError().text();
        return rows;
    }

    while (query.next())
    {
        AnnotationReportRow row;
        row.annotationId = query.value("annotation_id").toString();
        row.measurementType = measurementTypeFromString(query.value("measurement_type").toString());
        row.measurementTypeName = measurementTypeDisplayName(row.measurementType);
        row.label = query.value("label").toString().trimmed();
        if (row.label.isEmpty())
        {
            row.label = defaultAnnotationLabel(row.measurementType);
        }
        row.bodyRegion = normalizedBodyRegion(query.value("body_region").toString());
        row.note = query.value("note").toString();
        row.displayValue = reportDisplayValue(query, row.measurementType);
        row.seriesInstanceUid = query.value("series_instance_uid").toString();
        row.sopInstanceUid = query.value("sop_instance_uid").toString();
        row.frameIndex = query.value("frame_index").toInt();
        row.instanceNumber = query.value("instance_number").toString();
        row.seriesDescription = query.value("series_description").toString();
        row.modality = query.value("modality").toString();
        row.studyDate = query.value("study_date").toString();
        row.patientId = query.value("patient_id").toString();
        row.patientName = query.value("patient_name").toString();
        row.updatedAtUtc = dateTimeFromQuery(query, "updated_at");
        rows.append(row);
    }

    return rows;
}

bool SqliteService::updateAnnotationReportMetadata(
    const QString& annotationId,
    const QString& label,
    const QString& bodyRegion,
    const QString& note)
{
    if (annotationId.trimmed().isEmpty() || !ensureConnection())
    {
        return false;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "UPDATE measurement_annotations "
        "SET label = :label, body_region = :body_region, note = :note, updated_at = :updated_at "
        "WHERE annotation_id = :annotation_id AND is_deleted = 0");
    query.bindValue(":label", label.trimmed());
    query.bindValue(":body_region", normalizedBodyRegion(bodyRegion));
    query.bindValue(":note", note.trimmed());
    query.bindValue(":updated_at", dateTimeToDatabase(QDateTime::currentDateTimeUtc()));
    query.bindValue(":annotation_id", annotationId.trimmed());

    if (!query.exec())
    {
        qWarning() << "Failed to update annotation report metadata:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

DicomPreviewItems SqliteService::getStudyPreviewItemsForPatient(const QString& patientId)
{
    DicomPreviewItems items;
    if (patientId.isEmpty() || !ensureConnection())
    {
        return items;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT st.study_instance_uid, st.study_description, st.study_date, "
        "COUNT(se.series_instance_uid) AS series_count, "
        "(SELECT se2.preview_png "
        " FROM series se2 "
        " WHERE se2.study_instance_uid = st.study_instance_uid AND se2.preview_png IS NOT NULL "
        " ORDER BY se2.series_number, se2.series_instance_uid "
        " LIMIT 1) AS preview_png "
        "FROM studies st "
        "LEFT JOIN series se ON se.study_instance_uid = st.study_instance_uid "
        "WHERE st.patient_id = :patient_id "
        "GROUP BY st.study_instance_uid, st.study_description, st.study_date "
        "ORDER BY st.study_date, st.study_instance_uid");
    query.bindValue(":patient_id", patientId);

    if (!query.exec())
    {
        qWarning() << "Failed to load study preview items:" << query.lastError().text();
        return items;
    }

    while (query.next())
    {
        const int seriesCount = query.value("series_count").toInt();
        DicomPreviewItem item;
        item.title = query.value("study_description").toString().trimmed();
        if (item.title.isEmpty())
        {
            item.title = "Unnamed Study";
        }
        item.subtitle = query.value("study_date").toString();
        item.badgeText = countText(seriesCount, "series", "series");
        item.targetType = DicomPreviewTargetType::Study;
        item.targetId = query.value("study_instance_uid").toString();
        item.parentId = patientId;

        const QByteArray previewPng = query.value("preview_png").toByteArray();
        if (!previewPng.isEmpty())
        {
            item.pixmap.loadFromData(previewPng, "PNG");
        }
        items.append(item);
    }

    return items;
}

DicomPreviewItems SqliteService::getSeriesPreviewItemsForStudy(const QString& studyInstanceUid)
{
    DicomPreviewItems items;
    if (studyInstanceUid.isEmpty() || !ensureConnection())
    {
        return items;
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT s.series_instance_uid, s.series_description, s.modality, s.preview_png, "
        "(SELECT " + QString::fromUtf8(kFrameCountSumExpression) + " FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count "
        "FROM series s "
        "WHERE s.study_instance_uid = :study_instance_uid "
        "ORDER BY s.series_number, s.series_instance_uid");
    query.bindValue(":study_instance_uid", studyInstanceUid);

    if (!query.exec())
    {
        qWarning() << "Failed to load series preview items:" << query.lastError().text();
        return items;
    }

    while (query.next())
    {
        const int imageCount = query.value("image_count").toInt();
        const QString modality = query.value("modality").toString().trimmed();
        const QString description = query.value("series_description").toString().trimmed();

        DicomPreviewItem item;
        item.title = description.isEmpty() ? QString("Unnamed Series") : description;
        item.subtitle = modality;
        item.badgeText = countText(imageCount, "image", "images");
        item.targetType = DicomPreviewTargetType::Series;
        item.targetId = query.value("series_instance_uid").toString();
        item.parentId = studyInstanceUid;

        const QByteArray previewPng = query.value("preview_png").toByteArray();
        if (!previewPng.isEmpty())
        {
            item.pixmap.loadFromData(previewPng, "PNG");
        }
        items.append(item);
    }

    return items;
}

bool SqliteService::ensureConnection()
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

bool SqliteService::createTables()
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
        "preview_png BLOB"
        ")",
        "CREATE TABLE IF NOT EXISTS dicom_slice_metadata ("
        "sop_instance_uid TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL REFERENCES series(series_instance_uid) ON DELETE CASCADE,"
        "file_path TEXT NOT NULL UNIQUE,"
        "instance_number TEXT,"
        "frame_count INTEGER NOT NULL DEFAULT 1,"
        "frame_time_ms REAL,"
        "cine_rate_fps REAL,"
        "frame_interval_ms REAL NOT NULL DEFAULT 100,"
        "image_width INTEGER NOT NULL,"
        "image_height INTEGER NOT NULL"
        ")",
        "CREATE TABLE IF NOT EXISTS measurement_annotations ("
        "annotation_id TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL REFERENCES series(series_instance_uid) ON DELETE CASCADE,"
        "sop_instance_uid TEXT NOT NULL REFERENCES dicom_slice_metadata(sop_instance_uid) ON DELETE CASCADE,"
        "frame_index INTEGER NOT NULL DEFAULT 0,"
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
        "label TEXT,"
        "body_region TEXT NOT NULL DEFAULT 'Other',"
        "note TEXT,"
        "created_at TEXT NOT NULL DEFAULT (STRFTIME('%Y-%m-%dT%H:%M:%fZ', 'now')),"
        "updated_at TEXT NOT NULL DEFAULT (STRFTIME('%Y-%m-%dT%H:%M:%fZ', 'now')),"
        "is_deleted INTEGER NOT NULL DEFAULT 0"
        ")"
    };

    for (const char* statement : tableStatements)
    {
        QSqlQuery query(m_connection->database());
        if (!query.exec(QString::fromUtf8(statement)))
        {
            qWarning() << "Failed to create SQLite table:" << query.lastError().text();
            return false;
        }
    }

    if (!ensureAnnotationMetadataColumns() || !ensureSliceMetadataColumns())
    {
        return false;
    }

    static const char* indexStatements[] = {
        "CREATE INDEX IF NOT EXISTS idx_dicom_slice_metadata_series_instance_uid "
        "ON dicom_slice_metadata(series_instance_uid)",
        "CREATE INDEX IF NOT EXISTS idx_measurement_annotations_sop_instance_uid "
        "ON measurement_annotations(sop_instance_uid)",
        "CREATE INDEX IF NOT EXISTS idx_measurement_annotations_series_sop "
        "ON measurement_annotations(series_instance_uid, sop_instance_uid)",
        "CREATE INDEX IF NOT EXISTS idx_measurement_annotations_series_sop_frame "
        "ON measurement_annotations(series_instance_uid, sop_instance_uid, frame_index)",
        "CREATE INDEX IF NOT EXISTS idx_measurement_annotations_body_region "
        "ON measurement_annotations(body_region)"
    };

    for (const char* statement : indexStatements)
    {
        QSqlQuery query(m_connection->database());
        if (!query.exec(QString::fromUtf8(statement)))
        {
            qWarning() << "Failed to create SQLite index:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool SqliteService::ensureAnnotationMetadataColumns()
{
    QSet<QString> existingColumns;
    QSqlQuery columnQuery(m_connection->database());
    if (!columnQuery.exec("PRAGMA table_info(measurement_annotations)"))
    {
        qWarning() << "Failed to inspect measurement annotation columns:" << columnQuery.lastError().text();
        return false;
    }

    while (columnQuery.next())
    {
        existingColumns.insert(columnQuery.value("name").toString());
    }

    const QVector<QString> migrationStatements{
        existingColumns.contains("label") ? QString() : QString("ALTER TABLE measurement_annotations ADD COLUMN label TEXT"),
        existingColumns.contains("body_region") ? QString() : QString("ALTER TABLE measurement_annotations ADD COLUMN body_region TEXT NOT NULL DEFAULT 'Other'"),
        existingColumns.contains("note") ? QString() : QString("ALTER TABLE measurement_annotations ADD COLUMN note TEXT"),
        existingColumns.contains("frame_index") ? QString() : QString("ALTER TABLE measurement_annotations ADD COLUMN frame_index INTEGER NOT NULL DEFAULT 0")};

    for (const QString& statement : migrationStatements)
    {
        if (statement.isEmpty())
        {
            continue;
        }

        QSqlQuery query(m_connection->database());
        if (!query.exec(statement))
        {
            qWarning() << "Failed to migrate measurement annotation table:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool SqliteService::ensureSliceMetadataColumns()
{
    QSet<QString> existingColumns;
    QSqlQuery columnQuery(m_connection->database());
    if (!columnQuery.exec("PRAGMA table_info(dicom_slice_metadata)"))
    {
        qWarning() << "Failed to inspect DICOM slice metadata columns:" << columnQuery.lastError().text();
        return false;
    }

    while (columnQuery.next())
    {
        existingColumns.insert(columnQuery.value("name").toString());
    }

    const QVector<QString> migrationStatements{
        existingColumns.contains("frame_count") ? QString() : QString("ALTER TABLE dicom_slice_metadata ADD COLUMN frame_count INTEGER NOT NULL DEFAULT 1"),
        existingColumns.contains("frame_time_ms") ? QString() : QString("ALTER TABLE dicom_slice_metadata ADD COLUMN frame_time_ms REAL"),
        existingColumns.contains("cine_rate_fps") ? QString() : QString("ALTER TABLE dicom_slice_metadata ADD COLUMN cine_rate_fps REAL"),
        existingColumns.contains("frame_interval_ms") ? QString() : QString("ALTER TABLE dicom_slice_metadata ADD COLUMN frame_interval_ms REAL NOT NULL DEFAULT 100")};

    for (const QString& statement : migrationStatements)
    {
        if (statement.isEmpty())
        {
            continue;
        }

        QSqlQuery query(m_connection->database());
        if (!query.exec(statement))
        {
            qWarning() << "Failed to migrate DICOM slice metadata table:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool SqliteService::saveStudy(const QString& patientId, const Study& study)
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

bool SqliteService::saveSeries(const QString& studyInstanceUid, const Series& series)
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

bool SqliteService::saveImage(const QString& seriesInstanceUid, const DicomImage& image)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO dicom_slice_metadata ("
        "sop_instance_uid, series_instance_uid, file_path, instance_number, frame_count, "
        "frame_time_ms, cine_rate_fps, frame_interval_ms, image_width, image_height) "
        "VALUES ("
        ":sop_instance_uid, :series_instance_uid, :file_path, :instance_number, :frame_count, "
        ":frame_time_ms, :cine_rate_fps, :frame_interval_ms, :image_width, :image_height) "
        "ON CONFLICT (sop_instance_uid) DO UPDATE SET "
        "series_instance_uid = EXCLUDED.series_instance_uid, "
        "file_path = EXCLUDED.file_path, "
        "instance_number = EXCLUDED.instance_number, "
        "frame_count = EXCLUDED.frame_count, "
        "frame_time_ms = EXCLUDED.frame_time_ms, "
        "cine_rate_fps = EXCLUDED.cine_rate_fps, "
        "frame_interval_ms = EXCLUDED.frame_interval_ms, "
        "image_width = EXCLUDED.image_width, "
        "image_height = EXCLUDED.image_height");
    query.bindValue(":sop_instance_uid", image.sopInstanceUid());
    query.bindValue(":series_instance_uid", seriesInstanceUid);
    query.bindValue(":file_path", image.filePath());
    query.bindValue(":instance_number", image.instanceNumber());
    query.bindValue(":frame_count", std::max(1, image.frameCount()));
    query.bindValue(":frame_time_ms", image.frameTimeMs() > 0.0 ? QVariant(image.frameTimeMs()) : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(":cine_rate_fps", image.cineRateFps() > 0.0 ? QVariant(image.cineRateFps()) : QVariant(QMetaType(QMetaType::Double)));
    query.bindValue(":frame_interval_ms", image.cineFrameIntervalMs());
    query.bindValue(":image_width", image.width());
    query.bindValue(":image_height", image.height());

    if (!query.exec())
    {
        qWarning() << "Failed to save DICOM image:" << query.lastError().text();
        return false;
    }

    return true;
}

void SqliteService::populateStudies(Patient& patient)
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

void SqliteService::populateSeries(Study& study)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT s.series_instance_uid, s.series_description, s.modality, s.series_number, s.preview_png, "
        "(SELECT " + QString::fromUtf8(kFrameCountSumExpression) + " FROM dicom_slice_metadata di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_slice_metadata di "
        " WHERE di.series_instance_uid = s.series_instance_uid "
        " ORDER BY " + QString::fromUtf8(kSliceOrderClause) + " "
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

void SqliteService::populateImages(Series& series)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT sop_instance_uid, file_path, instance_number, frame_count, frame_time_ms, cine_rate_fps, frame_interval_ms, image_width, image_height "
        "FROM dicom_slice_metadata WHERE series_instance_uid = :series_instance_uid "
        "ORDER BY " + QString::fromUtf8(kSliceOrderClause));
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
            const int frameCount = std::max(1, image->frameCount());
            for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
            {
                auto frameImage = std::make_unique<DicomImage>(*image);
                frameImage->setFrameIndex(frameIndex);
                series.addImage(std::move(frameImage));
            }
        }
    }
}

DatabaseService::DicomImagePtr SqliteService::createImageFromQuery(const QSqlQuery& query) const
{
    auto image = std::make_shared<DicomImage>();
    image->setSopInstanceUid(query.value("sop_instance_uid").toString());
    image->setFilePath(query.value("file_path").toString());
    image->setInstanceNumber(query.value("instance_number").toString());
    image->setFrameCount(query.value("frame_count").toInt());
    image->setCineTiming(
        query.value("frame_time_ms").toDouble(),
        query.value("cine_rate_fps").toDouble(),
        query.value("frame_interval_ms").toDouble());
    image->setDimensions(query.value("image_width").toInt(), query.value("image_height").toInt());

    return image;
}

QPixmap SqliteService::createSeriesPreviewPixmap(const Series& series) const
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
