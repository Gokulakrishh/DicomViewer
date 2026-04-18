#include "FileHandling/GDCMFileHandling.h"
#include "Model/DicomParameters.h"
#include "Services/SeriesDataLoadService.h"
#include "Services/ThreeDProfiles/Bone3dPipelineProfile.h"
#include "Services/ThreeDProfiles/Lung3dPipelineProfile.h"
#include "Services/ThreeDSeriesBuildService.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QDebug>
#include <QString>

#include <exception>
#include <memory>
#include <vector>

namespace
{
struct SeriesSelection
{
    const Patient* patient{nullptr};
    const Study* study{nullptr};
    const Series* series{nullptr};
};

std::vector<SeriesSelection> collectSeries(const FileHandling::PatientList& patients)
{
    std::vector<SeriesSelection> selections;

    for (const auto& patientPtr : patients)
    {
        if (!patientPtr)
        {
            continue;
        }

        for (const auto& [studyUid, studyPtr] : patientPtr->studyMap())
        {
            Q_UNUSED(studyUid);
            if (!studyPtr)
            {
                continue;
            }

            for (const auto& [seriesUid, seriesPtr] : studyPtr->seriesMap())
            {
                Q_UNUSED(seriesUid);
                if (!seriesPtr)
                {
                    continue;
                }

                selections.push_back({patientPtr.get(), studyPtr.get(), seriesPtr.get()});
            }
        }
    }

    return selections;
}

const SeriesSelection* selectSeries(
    const std::vector<SeriesSelection>& selections,
    const QString& seriesUid,
    const QString& seriesNumber)
{
    if (!seriesUid.trimmed().isEmpty())
    {
        for (const SeriesSelection& selection : selections)
        {
            if (selection.series && selection.series->seriesInstanceUid() == seriesUid)
            {
                return &selection;
            }
        }
        return nullptr;
    }

    if (!seriesNumber.trimmed().isEmpty())
    {
        for (const SeriesSelection& selection : selections)
        {
            if (selection.series && selection.series->seriesNumber() == seriesNumber)
            {
                return &selection;
            }
        }
        return nullptr;
    }

    return selections.empty() ? nullptr : &selections.front();
}
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("ThreeDSeriesCli");

    QCommandLineParser parser;
    parser.setApplicationDescription("Run the 3D pipeline on a real DICOM series folder");
    parser.addHelpOption();

    QCommandLineOption folderOption(
        {"f", "folder"},
        "Path to the DICOM folder to scan.",
        "folder");
    QCommandLineOption profileOption(
        {"p", "profile"},
        "3D profile to use: bone or lung.",
        "profile",
        "bone");
    QCommandLineOption seriesUidOption(
        {"u", "series-uid"},
        "Optional series instance UID to select.",
        "series_uid");
    QCommandLineOption seriesNumberOption(
        {"n", "series-number"},
        "Optional series number to select if UID is not provided.",
        "series_number");

    parser.addOption(folderOption);
    parser.addOption(profileOption);
    parser.addOption(seriesUidOption);
    parser.addOption(seriesNumberOption);
    parser.process(app);

    const QString folderPath = parser.value(folderOption).trimmed();
    if (folderPath.isEmpty())
    {
        qCritical() << "Missing --folder argument";
        return 1;
    }

    const QString profileName = parser.value(profileOption).trimmed().toLower();
    const QString seriesUid = parser.value(seriesUidOption).trimmed();
    const QString seriesNumber = parser.value(seriesNumberOption).trimmed();

    try
    {
        GDCMFileHandling fileHandling;
        const FileHandling::PatientList patients = fileHandling.loadDicomFolder(folderPath);
        const std::vector<SeriesSelection> selections = collectSeries(patients);

        if (selections.empty())
        {
            qCritical() << "No series found in folder:" << folderPath;
            return 1;
        }

        const SeriesSelection* selection = selectSeries(selections, seriesUid, seriesNumber);
        if (!selection || !selection->series)
        {
            qCritical() << "Requested series was not found";
            return 1;
        }

        SeriesDataLoadService seriesDataLoadService(fileHandling);
        const Series* lightweightSeries = selection->series;
        const Series diagnosticSeries = seriesDataLoadService.loadDiagnosticSeries(*lightweightSeries);

        qInfo() << "Selected patient:" << (selection->patient ? selection->patient->patientName() : QString{});
        qInfo() << "Selected study:" << (selection->study ? selection->study->studyDescription() : QString{});
        qInfo() << "Selected series UID:" << lightweightSeries->seriesInstanceUid();
        qInfo() << "Selected series number:" << lightweightSeries->seriesNumber();
        qInfo() << "Selected series description:" << lightweightSeries->seriesDescription();
        qInfo() << "Series image count:" << diagnosticSeries.imageCount();

        ThreeDSeriesBuildService buildService;
        ThreeDimensionalPipelineResult result;

        if (profileName == "bone")
        {
            Bone3dPipelineProfile profile;
            result = buildService.buildFromDiagnosticSeries(diagnosticSeries, profile);
        }
        else if (profileName == "lung")
        {
            Lung3dPipelineProfile profile;
            result = buildService.buildFromDiagnosticSeries(diagnosticSeries, profile);
        }
        else
        {
            qCritical() << "Unsupported profile:" << profileName;
            return 1;
        }

        if (!result.isValid())
        {
            qCritical() << "3D pipeline returned an invalid result";
            return 1;
        }

        qInfo() << "3D pipeline completed";
        qInfo() << "Profile:" << QString::fromStdString(result.diagnostics.profileName);
        qInfo() << "Foreground voxels:" << result.diagnostics.foregroundVoxelCount;
        qInfo() << "Mesh vertices:" << static_cast<qulonglong>(result.diagnostics.meshVertexCount);
        qInfo() << "Mesh triangles:" << static_cast<qulonglong>(result.diagnostics.meshTriangleCount);
        return 0;
    }
    catch (const std::exception& exception)
    {
        qCritical() << "3D series CLI exception:" << exception.what();
        return 1;
    }
}
