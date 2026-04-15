#include "Database/PostgreService.h"

#include "Database/PostgreConnection.h"

#include <QBuffer>
#include <QDebug>
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

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
        "(SELECT COUNT(*) FROM dicom_images di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_images di "
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
        "(SELECT COUNT(*) FROM dicom_images di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_images di "
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
        "FROM dicom_images WHERE sop_instance_uid = :sop_instance_uid");
    query.bindValue(":sop_instance_uid", sopInstanceUid);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    return createImageFromQuery(query);
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
        "CREATE TABLE IF NOT EXISTS dicom_images ("
        "sop_instance_uid TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL REFERENCES series(series_instance_uid) ON DELETE CASCADE,"
        "file_path TEXT NOT NULL UNIQUE,"
        "instance_number TEXT,"
        "image_width INTEGER NOT NULL,"
        "image_height INTEGER NOT NULL"
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
        "INSERT INTO dicom_images (sop_instance_uid, series_instance_uid, file_path, instance_number, image_width, image_height) "
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
        "(SELECT COUNT(*) FROM dicom_images di WHERE di.series_instance_uid = s.series_instance_uid) AS image_count, "
        "(SELECT file_path FROM dicom_images di "
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
        "FROM dicom_images WHERE series_instance_uid = :series_instance_uid "
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
