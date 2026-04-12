#include "GDCMFileHandling.h"

#include "DicomViewerWindow/DicomRenderService.h"
#include "Model/DicomImage.h"

#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QList>
#include <QPixmap>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <gdcmImage.h>
#include <gdcmPhotometricInterpretation.h>
#include <gdcmTag.h>

GDCMFileHandling::GDCMFileHandling()
{
    for (const auto& format : QImageReader::supportedImageFormats())
    {
        m_supportedFormats << ("*." + QString::fromLatin1(format));
    }

    if (!m_supportedFormats.contains("*.dcm", Qt::CaseInsensitive))
    {
        m_supportedFormats << "*.dcm";
    }
}

FileHandling::PatientList GDCMFileHandling::loadDicomFolder(const QString& folderPath)
{
    PatientList patients;
    std::map<QString, PatientPtr> patientMap;

    std::error_code errorCode;
    const std::filesystem::path rootPath = std::filesystem::path(folderPath.toStdString());
    if (!std::filesystem::exists(rootPath, errorCode))
    {
        return patients;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath, errorCode))
    {
        if (errorCode || !entry.is_regular_file())
        {
            continue;
        }

        const QString filePath = QString::fromStdString(entry.path().string());
        if (!canLoad(filePath))
        {
            continue;
        }

        PatientPtr patient = loadDicomHierarchy(filePath);
        if (!patient)
        {
            continue;
        }

        auto it = patientMap.find(patient->patientId());
        if (it == patientMap.end())
        {
            patientMap.emplace(patient->patientId(), patient);
        }
        else
        {
            mergePatientHierarchy(patient, *it->second);
        }
    }

    for (const auto& [patientId, patient] : patientMap)
    {
        Q_UNUSED(patientId);
        patients.append(patient);
    }

    return patients;
}

std::unique_ptr<MedicalImage> GDCMFileHandling::loadImage(const QString& filePath)
{
    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!reader.Read())
    {
        qDebug() << "Failed to read DICOM file:" << filePath;
        return nullptr;
    }

    return loadDicomImage(filePath, reader, true);
}

std::unique_ptr<DicomImage> GDCMFileHandling::loadImageData(const QString& filePath) const
{
    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!reader.Read())
    {
        qDebug() << "Failed to read DICOM file:" << filePath;
        return nullptr;
    }

    return loadDicomImage(filePath, reader, false);
}

FileHandling::PatientPtr GDCMFileHandling::loadDicomHierarchy(const QString& filePath)
{
    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!reader.Read())
    {
        qDebug() << "Failed to read DICOM hierarchy from file:" << filePath;
        return nullptr;
    }

    return buildHierarchy(filePath, reader);
}

QStringList GDCMFileHandling::getSupportedFormats() const
{
    return m_supportedFormats;
}

bool GDCMFileHandling::canLoad(const QString& filePath) const
{
    const QFileInfo info(filePath);
    const QString suffix = "*." + info.suffix();
    return m_supportedFormats.contains(suffix, Qt::CaseInsensitive);
}

void GDCMFileHandling::mergePatientHierarchy(const PatientPtr& sourcePatient, Patient& targetPatient) const
{
    if (!sourcePatient)
    {
        return;
    }

    if (targetPatient.patientName().isEmpty())
    {
        targetPatient.setPatientName(sourcePatient->patientName());
    }
    if (targetPatient.patientSex().isEmpty())
    {
        targetPatient.setPatientSex(sourcePatient->patientSex());
    }
    if (targetPatient.dateOfBirth().isEmpty())
    {
        targetPatient.setDateOfBirth(sourcePatient->dateOfBirth());
    }

    for (const auto& [studyInstanceUid, sourceStudy] : sourcePatient->studyMap())
    {
        Study& targetStudy = targetPatient.getOrCreateStudy(studyInstanceUid);
        if (targetStudy.studyDescription().isEmpty())
        {
            targetStudy.setStudyDescription(sourceStudy->studyDescription());
        }
        if (targetStudy.studyDate().isEmpty())
        {
            targetStudy.setStudyDate(sourceStudy->studyDate());
        }
        if (targetStudy.doctorName().isEmpty())
        {
            targetStudy.setDoctorName(sourceStudy->doctorName());
        }

        for (const auto& [seriesInstanceUid, sourceSeries] : sourceStudy->seriesMap())
        {
            Series& targetSeries = targetStudy.getOrCreateSeries(seriesInstanceUid);
            if (targetSeries.seriesDescription().isEmpty())
            {
                targetSeries.setSeriesDescription(sourceSeries->seriesDescription());
            }
            if (targetSeries.modality().isEmpty())
            {
                targetSeries.setModality(sourceSeries->modality());
            }
            if (targetSeries.seriesNumber().isEmpty())
            {
                targetSeries.setSeriesNumber(sourceSeries->seriesNumber());
            }

            for (const auto& sourceImage : sourceSeries->images())
            {
                if (!sourceImage)
                {
                    continue;
                }

                auto copiedImage = std::make_unique<DicomImage>();
                copiedImage->setPixmap(sourceImage->pixmap());
                copiedImage->setFilePath(sourceImage->filePath());
                copiedImage->setSopInstanceUid(sourceImage->sopInstanceUid());
                copiedImage->setInstanceNumber(sourceImage->instanceNumber());
                copiedImage->setDimensions(sourceImage->width(), sourceImage->height());
                targetSeries.addImage(std::move(copiedImage));
            }
        }
    }
}

QString GDCMFileHandling::normalizeDicomDate(const QString& dicomDate) const
{
    const QString trimmedDate = dicomDate.trimmed();
    if (trimmedDate.isEmpty())
    {
        return {};
    }

    const QDate parsedDate = QDate::fromString(trimmedDate, "yyyyMMdd");
    if (parsedDate.isValid())
    {
        return parsedDate.toString("yyyy-MM-dd");
    }

    return trimmedDate;
}

QString GDCMFileHandling::readStringTag(const gdcm::StringFilter& stringFilter, uint16_t group, uint16_t element) const
{
    const QString value = QString::fromStdString(stringFilter.ToString(gdcm::Tag(group, element))).trimmed();
    return value;
}

std::unique_ptr<DicomImage> GDCMFileHandling::loadDicomImage(
    const QString& filePath,
    const gdcm::ImageReader& reader,
    bool renderPixmap) const
{
    const gdcm::Image& gdcmImage = reader.GetImage();
    const unsigned int width = gdcmImage.GetDimensions()[0];
    const unsigned int height = gdcmImage.GetDimensions()[1];
    const unsigned long bufferLength = gdcmImage.GetBufferLength();
    const unsigned int samplesPerPixel = gdcmImage.GetPixelFormat().GetSamplesPerPixel();
    const unsigned int bitsAllocated = gdcmImage.GetPixelFormat().GetBitsAllocated();
    const bool isSignedPixelData = gdcmImage.GetPixelFormat().GetPixelRepresentation() != 0;
    const bool isMonochrome1 =
        gdcmImage.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::MONOCHROME1;

    QVector<char> buffer(static_cast<qsizetype>(bufferLength));
    if (!gdcmImage.GetBuffer(buffer.data()))
    {
        qDebug() << "Failed to get pixel buffer for:" << filePath;
        return nullptr;
    }

    gdcm::StringFilter stringFilter;
    stringFilter.SetFile(reader.GetFile());

    const auto readNumericTagValue = [this, &stringFilter](uint16_t group, uint16_t element, double fallbackValue) {
        const QString tagValue = readStringTag(stringFilter, group, element);
        if (tagValue.isEmpty())
        {
            return fallbackValue;
        }

        const QString firstComponent = tagValue.split('\\').value(0).trimmed();
        bool isDouble = false;
        const double parsedValue = firstComponent.toDouble(&isDouble);
        return isDouble ? parsedValue : fallbackValue;
    };
    const auto readPixelSpacingValues = [this, &stringFilter]() {
        const QString tagValue = readStringTag(stringFilter, 0x0028, 0x0030);
        const QStringList components = tagValue.split('\\', Qt::SkipEmptyParts);
        if (components.size() < 2)
        {
            return QPair<double, double>(0.0, 0.0);
        }

        bool rowOk = false;
        bool columnOk = false;
        const double rowSpacing = components.at(0).trimmed().toDouble(&rowOk);
        const double columnSpacing = components.at(1).trimmed().toDouble(&columnOk);
        if (!rowOk || !columnOk || rowSpacing <= 0.0 || columnSpacing <= 0.0)
        {
            return QPair<double, double>(0.0, 0.0);
        }

        return QPair<double, double>(columnSpacing, rowSpacing);
    };

    QImage image;
    QVector<int> rawPixels;
    bool isMonochromeImage = false;
    int minimumStoredValue = 0;
    int maximumStoredValue = 255;
    int defaultWindowLevel = 0;
    int defaultWindowWidth = 255;
    if (samplesPerPixel == 3 && bitsAllocated <= 8)
    {
        image = QImage(
                    reinterpret_cast<const uchar*>(buffer.constData()),
                    static_cast<int>(width),
                    static_cast<int>(height),
                    static_cast<int>(width) * 3,
                    QImage::Format_RGB888)
                    .rgbSwapped()
                    .copy();
    }
    else if (samplesPerPixel == 1 && bitsAllocated <= 8)
    {
        isMonochromeImage = true;
        image = QImage(
                    reinterpret_cast<const uchar*>(buffer.constData()),
                    static_cast<int>(width),
                    static_cast<int>(height),
                    static_cast<int>(width),
                    QImage::Format_Grayscale8)
                    .copy();

        const int pixelCount = static_cast<int>(width * height);
        rawPixels.resize(pixelCount);
        minimumStoredValue = std::numeric_limits<int>::max();
        maximumStoredValue = std::numeric_limits<int>::min();
        const auto* pixelData = reinterpret_cast<const uint8_t*>(buffer.constData());
        for (int index = 0; index < pixelCount; ++index)
        {
            const int value = static_cast<int>(pixelData[index]);
            rawPixels[index] = value;
            minimumStoredValue = std::min(minimumStoredValue, value);
            maximumStoredValue = std::max(maximumStoredValue, value);
        }
    }
    else if (samplesPerPixel == 1 && bitsAllocated <= 16)
    {
        isMonochromeImage = true;
        image = QImage(static_cast<int>(width), static_cast<int>(height), QImage::Format_Grayscale8);

        const int pixelCount = static_cast<int>(width * height);
        minimumStoredValue = std::numeric_limits<int>::max();
        maximumStoredValue = std::numeric_limits<int>::min();
        rawPixels.resize(pixelCount);
        const double rescaleSlope = readNumericTagValue(0x0028, 0x1053, 1.0);
        const double rescaleIntercept = readNumericTagValue(0x0028, 0x1052, 0.0);

        if (isSignedPixelData)
        {
            const auto* pixelData = reinterpret_cast<const int16_t*>(buffer.constData());
            for (int index = 0; index < pixelCount; ++index)
            {
                const int value = static_cast<int>(std::lround((static_cast<double>(pixelData[index]) * rescaleSlope) + rescaleIntercept));
                rawPixels[index] = value;
                minimumStoredValue = std::min(minimumStoredValue, value);
                maximumStoredValue = std::max(maximumStoredValue, value);
            }
        }
        else
        {
            const auto* pixelData = reinterpret_cast<const uint16_t*>(buffer.constData());
            for (int index = 0; index < pixelCount; ++index)
            {
                const int value = static_cast<int>(std::lround((static_cast<double>(pixelData[index]) * rescaleSlope) + rescaleIntercept));
                rawPixels[index] = value;
                minimumStoredValue = std::min(minimumStoredValue, value);
                maximumStoredValue = std::max(maximumStoredValue, value);
            }
        }

        defaultWindowLevel = static_cast<int>(std::lround(readNumericTagValue(
            0x0028,
            0x1050,
            (static_cast<double>(minimumStoredValue) + static_cast<double>(maximumStoredValue)) / 2.0)));
        defaultWindowWidth = std::max(
            1,
            static_cast<int>(std::lround(readNumericTagValue(
                0x0028,
                0x1051,
                static_cast<double>(std::max(1, maximumStoredValue - minimumStoredValue))))));
    }
    else
    {
        qDebug() << "Unsupported DICOM pixel format for:" << filePath
                 << "samplesPerPixel=" << samplesPerPixel
                 << "bitsAllocated=" << bitsAllocated;
        return nullptr;
    }

    auto dicomImage = std::make_unique<DicomImage>();
    dicomImage->setFilePath(filePath);
    dicomImage->setDimensions(static_cast<int>(width), static_cast<int>(height));
    dicomImage->setSopInstanceUid(readStringTag(stringFilter, 0x0008, 0x0018));
    dicomImage->setInstanceNumber(readStringTag(stringFilter, 0x0020, 0x0013));
    const auto [pixelSpacingX, pixelSpacingY] = readPixelSpacingValues();
    dicomImage->setPixelSpacing(pixelSpacingX, pixelSpacingY);

    if (isMonochromeImage && !rawPixels.isEmpty())
    {
        dicomImage->setMonochrome(true);
        dicomImage->setMonochrome1(isMonochrome1);
        dicomImage->setRawPixels(rawPixels);
        dicomImage->setValueRange(minimumStoredValue, maximumStoredValue);

        if (samplesPerPixel == 1 && bitsAllocated <= 8)
        {
            defaultWindowLevel = static_cast<int>(std::lround(readNumericTagValue(
                0x0028,
                0x1050,
                (static_cast<double>(minimumStoredValue) + static_cast<double>(maximumStoredValue)) / 2.0)));
            defaultWindowWidth = std::max(
                1,
                static_cast<int>(std::lround(readNumericTagValue(
                    0x0028,
                    0x1051,
                    static_cast<double>(std::max(1, maximumStoredValue - minimumStoredValue))))));
        }

        dicomImage->setDefaultWindow(defaultWindowLevel, defaultWindowWidth);
        if (renderPixmap)
        {
            DicomRenderService renderService;
            dicomImage->setPixmap(renderService.renderImage(*dicomImage, defaultWindowLevel, defaultWindowWidth)->pixmap());
        }
    }
    else
    {
        if (renderPixmap)
        {
            dicomImage->setPixmap(QPixmap::fromImage(image));
        }
    }

    if (dicomImage->sopInstanceUid().isEmpty())
    {
        const QFileInfo fileInfo(filePath);
        dicomImage->setSopInstanceUid(fileInfo.completeBaseName());
    }

    return dicomImage;
}

FileHandling::PatientPtr GDCMFileHandling::buildHierarchy(const QString& filePath, const gdcm::ImageReader& reader) const
{
    gdcm::StringFilter stringFilter;
    stringFilter.SetFile(reader.GetFile());

    auto patient = std::make_shared<Patient>();
    patient->setPatientId(readStringTag(stringFilter, 0x0010, 0x0020));
    patient->setPatientName(readStringTag(stringFilter, 0x0010, 0x0010));
    patient->setPatientSex(readStringTag(stringFilter, 0x0010, 0x0040));
    patient->setDateOfBirth(normalizeDicomDate(readStringTag(stringFilter, 0x0010, 0x0030)));

    if (patient->patientId().isEmpty())
    {
        patient->setPatientId("UNKNOWN_PATIENT");
    }

    QString studyInstanceUid = readStringTag(stringFilter, 0x0020, 0x000D);
    if (studyInstanceUid.isEmpty())
    {
        studyInstanceUid = patient->patientId() + "_STUDY";
    }

    Study& study = patient->getOrCreateStudy(studyInstanceUid);
    study.setStudyDescription(readStringTag(stringFilter, 0x0008, 0x1030));
    study.setStudyDate(normalizeDicomDate(readStringTag(stringFilter, 0x0008, 0x0020)));
    study.setDoctorName(readStringTag(stringFilter, 0x0008, 0x0090));

    QString seriesInstanceUid = readStringTag(stringFilter, 0x0020, 0x000E);
    if (seriesInstanceUid.isEmpty())
    {
        seriesInstanceUid = studyInstanceUid + "_SERIES";
    }

    Series& series = study.getOrCreateSeries(seriesInstanceUid);
    series.setSeriesDescription(readStringTag(stringFilter, 0x0008, 0x103E));
    series.setModality(readStringTag(stringFilter, 0x0008, 0x0060));
    series.setSeriesNumber(readStringTag(stringFilter, 0x0020, 0x0011));

    std::unique_ptr<DicomImage> dicomImage = loadDicomImage(filePath, reader, true);
    if (dicomImage)
    {
        series.addImage(std::move(dicomImage));
    }

    return patient;
}
