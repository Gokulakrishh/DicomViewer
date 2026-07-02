#include "Database/SqliteService.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Utilities/DatabaseSettings.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFuture>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>

namespace
{
struct ImportResult
{
    bool success{false};
    QString errorMessage;
    int patientCount{0};
    int imageCount{0};
};

int countImages(const FileHandling::PatientList& patients)
{
    int imageCount = 0;
    for (const auto& patient : patients)
    {
        if (!patient)
        {
            continue;
        }
        for (const auto& [studyUid, study] : patient->studyMap())
        {
            Q_UNUSED(studyUid);
            if (!study)
            {
                continue;
            }
            for (const auto& [seriesUid, series] : study->seriesMap())
            {
                Q_UNUSED(seriesUid);
                if (series)
                {
                    imageCount += static_cast<int>(series->images().size());
                }
            }
        }
    }
    return imageCount;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() < 2)
    {
        qCritical() << "Usage: FolderImportDbSmokeTest /path/to/dicom-folder";
        return 1;
    }

    const QString folderPath = app.arguments().at(1);
    QTemporaryDir databaseDirectory;
    if (!databaseDirectory.isValid())
    {
        qCritical() << "Failed to create temporary database directory.";
        return 1;
    }

    DatabaseSettings databaseSettings;
    databaseSettings.setFilePath(databaseDirectory.filePath(QStringLiteral("folder-import.sqlite")));

    QFuture<ImportResult> future = QtConcurrent::run([databaseSettings, folderPath]() {
        ImportResult result;

        GDCMFileHandling fileHandling;
        SqliteService databaseService(databaseSettings);
        if (!databaseService.initialize())
        {
            result.errorMessage = databaseService.lastErrorText();
            return result;
        }

        const FileHandling::PatientList patients = fileHandling.loadDicomFolder(folderPath);
        result.patientCount = patients.size();
        result.imageCount = countImages(patients);
        if (patients.isEmpty())
        {
            result.errorMessage = QStringLiteral("No importable DICOM files found.");
            return result;
        }

        for (const auto& patient : patients)
        {
            if (!databaseService.savePatient(patient))
            {
                result.errorMessage = databaseService.lastErrorText();
                return result;
            }
        }

        result.success = true;
        return result;
    });

    future.waitForFinished();
    const ImportResult result = future.result();
    if (!result.success)
    {
        qCritical() << "Folder import failed:" << result.errorMessage;
        return 1;
    }

    qInfo() << "Imported patients:" << result.patientCount;
    qInfo() << "Imported images:" << result.imageCount;
    return 0;
}
