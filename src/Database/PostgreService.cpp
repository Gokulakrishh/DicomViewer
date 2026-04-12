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

QList<DatabaseService::PatientPtr> PostgreService::getAllPatients()
{
    QList<PatientPtr> patients;
    if (!ensureConnection())
    {
        return patients;
    }

    QSqlQuery query(m_connection->database());
    if (!query.exec("SELECT patient_id, patient_name, patient_sex, date_of_birth FROM patients ORDER BY patient_id"))
    {
        return patients;
    }

    while (query.next())
    {
        auto patient = std::make_shared<Patient>();
        patient->setPatientId(query.value("patient_id").toString());
        patient->setPatientName(query.value("patient_name").toString());
        patient->setPatientSex(query.value("patient_sex").toString());
        patient->setDateOfBirth(query.value("date_of_birth").toString());
        populateStudies(*patient);
        patients.append(patient);
    }

    return patients;
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
        "SELECT series_instance_uid, series_description, modality, series_number "
        "FROM series WHERE series_instance_uid = :series_instance_uid");
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
        "SELECT sop_instance_uid, file_path, instance_number, image_width, image_height, preview_png "
        "FROM dicom_images WHERE sop_instance_uid = :sop_instance_uid");
    query.bindValue(":sop_instance_uid", sopInstanceUid);

    if (!query.exec() || !query.next())
    {
        return nullptr;
    }

    return createImageFromQuery(query);
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
        "series_number TEXT"
        ")",
        "CREATE TABLE IF NOT EXISTS dicom_images ("
        "sop_instance_uid TEXT PRIMARY KEY,"
        "series_instance_uid TEXT NOT NULL REFERENCES series(series_instance_uid) ON DELETE CASCADE,"
        "file_path TEXT NOT NULL UNIQUE,"
        "instance_number TEXT,"
        "image_width INTEGER NOT NULL,"
        "image_height INTEGER NOT NULL,"
        "preview_png BYTEA"
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
    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO series (series_instance_uid, study_instance_uid, series_description, modality, series_number) "
        "VALUES (:series_instance_uid, :study_instance_uid, :series_description, :modality, :series_number) "
        "ON CONFLICT (series_instance_uid) DO UPDATE SET "
        "study_instance_uid = EXCLUDED.study_instance_uid, "
        "series_description = EXCLUDED.series_description, "
        "modality = EXCLUDED.modality, "
        "series_number = EXCLUDED.series_number");
    query.bindValue(":series_instance_uid", series.seriesInstanceUid());
    query.bindValue(":study_instance_uid", studyInstanceUid);
    query.bindValue(":series_description", series.seriesDescription());
    query.bindValue(":modality", series.modality());
    query.bindValue(":series_number", series.seriesNumber());

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
    QByteArray previewPng;
    if (!image.pixmap().isNull())
    {
        QBuffer buffer(&previewPng);
        buffer.open(QIODevice::WriteOnly);
        image.pixmap().save(&buffer, "PNG");
    }

    QSqlQuery query(m_connection->database());
    query.prepare(
        "INSERT INTO dicom_images (sop_instance_uid, series_instance_uid, file_path, instance_number, image_width, image_height, preview_png) "
        "VALUES (:sop_instance_uid, :series_instance_uid, :file_path, :instance_number, :image_width, :image_height, :preview_png) "
        "ON CONFLICT (sop_instance_uid) DO UPDATE SET "
        "series_instance_uid = EXCLUDED.series_instance_uid, "
        "file_path = EXCLUDED.file_path, "
        "instance_number = EXCLUDED.instance_number, "
        "image_width = EXCLUDED.image_width, "
        "image_height = EXCLUDED.image_height, "
        "preview_png = EXCLUDED.preview_png");
    query.bindValue(":sop_instance_uid", image.sopInstanceUid());
    query.bindValue(":series_instance_uid", seriesInstanceUid);
    query.bindValue(":file_path", image.filePath());
    query.bindValue(":instance_number", image.instanceNumber());
    query.bindValue(":image_width", image.width());
    query.bindValue(":image_height", image.height());
    query.bindValue(":preview_png", previewPng);

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
        "SELECT series_instance_uid, series_description, modality, series_number "
        "FROM series WHERE study_instance_uid = :study_instance_uid ORDER BY series_number, series_instance_uid");
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
        populateImages(series);
    }
}

void PostgreService::populateImages(Series& series)
{
    QSqlQuery query(m_connection->database());
    query.prepare(
        "SELECT sop_instance_uid, file_path, instance_number, image_width, image_height, preview_png "
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

    const QByteArray previewPng = query.value("preview_png").toByteArray();
    if (!previewPng.isEmpty())
    {
        QPixmap pixmap;
        pixmap.loadFromData(previewPng, "PNG");
        image->setPixmap(pixmap);
    }

    return image;
}
