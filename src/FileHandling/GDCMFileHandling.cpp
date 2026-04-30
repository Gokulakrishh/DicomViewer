#include "GDCMFileHandling.h"

#include "Model/DicomImage.h"
#include "Utilities/DiagnosticImageRenderer.h"

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
#include <array>
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

FileHandling::PatientList GDCMFileHandling::loadDicomFolder(const QString& folderPath, ProgressCallback progressCallback)
{
    PatientList patients;
    std::map<QString, PatientPtr> patientMap;

    std::error_code errorCode;
    const std::filesystem::path rootPath = std::filesystem::path(folderPath.toStdString());
    if (!std::filesystem::exists(rootPath, errorCode))
    {
        return patients;
    }

    int totalRegularFiles = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath, errorCode))
    {
        if (errorCode)
        {
            break;
        }

        if (entry.is_regular_file())
        {
            ++totalRegularFiles;
        }
    }
    errorCode.clear();

    int processedRegularFiles = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath, errorCode))
    {
        if (errorCode || !entry.is_regular_file())
        {
            continue;
        }

        ++processedRegularFiles;
        if (progressCallback)
        {
            progressCallback(processedRegularFiles, totalRegularFiles);
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

    return loadDicomImage(filePath, reader, false);
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
            if (targetSeries.previewPixmap().isNull() && !sourceSeries->previewPixmap().isNull())
            {
                targetSeries.setPreviewPixmap(sourceSeries->previewPixmap());
            }
            if (targetSeries.representativeFilePath().isEmpty())
            {
                targetSeries.setRepresentativeFilePath(sourceSeries->representativeFilePath());
            }

            for (const auto& sourceImage : sourceSeries->images())
            {
                if (!sourceImage)
                {
                    continue;
                }

                auto copiedImage = std::make_unique<DicomImage>();
                copiedImage->setFilePath(sourceImage->filePath());
                if (const auto sourceMetadata = sourceImage->metadata())
                {
                    auto copiedMetadata = std::make_shared<DicomInstanceMetadata>(*sourceMetadata);
                    for (const auto& existingImage : targetSeries.images())
                    {
                        if (existingImage && existingImage->metadata() && existingImage->metadata()->series)
                        {
                            copiedMetadata->series = existingImage->metadata()->series;
                            break;
                        }
                    }
                    copiedImage->setMetadata(copiedMetadata);
                }
                copiedImage->setSopInstanceUid(sourceImage->sopInstanceUid());
                copiedImage->setInstanceNumber(sourceImage->instanceNumber());
                copiedImage->setDimensions(sourceImage->width(), sourceImage->height());
                if (sourceImage->hasPixelSpacing())
                {
                    copiedImage->setPixelSpacing(sourceImage->pixelSpacingX(), sourceImage->pixelSpacingY());
                }
                if (sourceImage->hasImagePositionPatient())
                {
                    copiedImage->setImagePositionPatient(sourceImage->imagePositionPatient());
                }
                if (sourceImage->hasImageOrientationPatient())
                {
                    copiedImage->setImageOrientationPatient(sourceImage->imageOrientationPatient());
                }
                copiedImage->setSliceThickness(sourceImage->sliceThickness());
                copiedImage->setSpacingBetweenSlices(sourceImage->spacingBetweenSlices());
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
    const auto readOptionalNumericTagValue = [this, &stringFilter](uint16_t group, uint16_t element) {
        const QString tagValue = readStringTag(stringFilter, group, element);
        if (tagValue.isEmpty())
        {
            return std::pair<bool, double>(false, 0.0);
        }

        const QString firstComponent = tagValue.split('\\').value(0).trimmed();
        bool isDouble = false;
        const double parsedValue = firstComponent.toDouble(&isDouble);
        return std::pair<bool, double>(isDouble, isDouble ? parsedValue : 0.0);
    };
    const auto readNumericComponents = [this, &stringFilter](uint16_t group, uint16_t element) {
        std::vector<double> values;
        const QString tagValue = readStringTag(stringFilter, group, element);
        const QStringList components = tagValue.split('\\', Qt::SkipEmptyParts);
        values.reserve(static_cast<std::size_t>(components.size()));

        for (const QString& component : components)
        {
            bool ok = false;
            const double value = component.trimmed().toDouble(&ok);
            if (ok)
            {
                values.push_back(value);
            }
        }

        return values;
    };
    const auto readStringComponents = [this, &stringFilter](uint16_t group, uint16_t element) {
        std::vector<QString> values;
        const QString tagValue = readStringTag(stringFilter, group, element);
        const QStringList components = tagValue.split('\\', Qt::SkipEmptyParts);
        values.reserve(static_cast<std::size_t>(components.size()));

        for (const QString& component : components)
        {
            const QString value = component.trimmed();
            if (!value.isEmpty())
            {
                values.push_back(value);
            }
        }

        return values;
    };
    const auto readWindowPresets = [&readNumericComponents, &readStringComponents]() {
        std::vector<DicomWindowPreset> presets;
        const std::vector<double> centers = readNumericComponents(0x0028, 0x1050);
        const std::vector<double> widths = readNumericComponents(0x0028, 0x1051);
        const std::vector<QString> explanations = readStringComponents(0x0028, 0x1055);
        const std::size_t presetCount = std::min(centers.size(), widths.size());
        presets.reserve(presetCount);

        for (std::size_t index = 0; index < presetCount; ++index)
        {
            if (widths[index] <= 0.0)
            {
                continue;
            }

            presets.push_back({
                centers[index],
                widths[index],
                index < explanations.size() ? explanations[index] : QString{}});
        }

        return presets;
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
    const auto readVector3Tag = [this, &stringFilter](uint16_t group, uint16_t element) {
        std::array<double, 3> values{0.0, 0.0, 0.0};
        const QString tagValue = readStringTag(stringFilter, group, element);
        const QStringList components = tagValue.split('\\', Qt::SkipEmptyParts);
        if (components.size() < 3)
        {
            return std::pair<bool, std::array<double, 3>>(false, values);
        }

        bool allValid = true;
        for (int index = 0; index < 3; ++index)
        {
            bool ok = false;
            values[static_cast<std::size_t>(index)] = components.at(index).trimmed().toDouble(&ok);
            allValid = allValid && ok;
        }

        return std::pair<bool, std::array<double, 3>>(allValid, values);
    };
    const auto readVector6Tag = [this, &stringFilter](uint16_t group, uint16_t element) {
        std::array<double, 6> values{1.0, 0.0, 0.0, 0.0, 1.0, 0.0};
        const QString tagValue = readStringTag(stringFilter, group, element);
        const QStringList components = tagValue.split('\\', Qt::SkipEmptyParts);
        if (components.size() < 6)
        {
            return std::pair<bool, std::array<double, 6>>(false, values);
        }

        bool allValid = true;
        for (int index = 0; index < 6; ++index)
        {
            bool ok = false;
            values[static_cast<std::size_t>(index)] = components.at(index).trimmed().toDouble(&ok);
            allValid = allValid && ok;
        }

        return std::pair<bool, std::array<double, 6>>(allValid, values);
    };

    QImage image;
    std::vector<int16_t> rawPixels;
    bool isMonochromeImage = false;
    int minimumStoredValue = 0;
    int maximumStoredValue = 255;
    int defaultWindowLevel = 0;
    int defaultWindowWidth = 255;
    const auto [hasRescaleSlope, rescaleSlope] = readOptionalNumericTagValue(0x0028, 0x1053);
    const auto [hasRescaleIntercept, rescaleIntercept] = readOptionalNumericTagValue(0x0028, 0x1052);
    const double appliedRescaleSlope = hasRescaleSlope ? rescaleSlope : 1.0;
    const double appliedRescaleIntercept = hasRescaleIntercept ? rescaleIntercept : 0.0;
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
        rawPixels.resize(static_cast<std::size_t>(pixelCount));
        minimumStoredValue = std::numeric_limits<int>::max();
        maximumStoredValue = std::numeric_limits<int>::min();
        const auto* pixelData = reinterpret_cast<const uint8_t*>(buffer.constData());
        for (int index = 0; index < pixelCount; ++index)
        {
            const int value = static_cast<int>(pixelData[index]);
            rawPixels[static_cast<std::size_t>(index)] = static_cast<int16_t>(value);
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
        rawPixels.resize(static_cast<std::size_t>(pixelCount));
        if (isSignedPixelData)
        {
            const auto* pixelData = reinterpret_cast<const int16_t*>(buffer.constData());
            for (int index = 0; index < pixelCount; ++index)
            {
                const int value = static_cast<int>(std::lround((static_cast<double>(pixelData[index]) * appliedRescaleSlope) + appliedRescaleIntercept));
                const int clampedValue = std::clamp(value, -32768, 32767);
                rawPixels[static_cast<std::size_t>(index)] = static_cast<int16_t>(clampedValue);
                minimumStoredValue = std::min(minimumStoredValue, clampedValue);
                maximumStoredValue = std::max(maximumStoredValue, clampedValue);
            }
        }
        else
        {
            const auto* pixelData = reinterpret_cast<const uint16_t*>(buffer.constData());
            for (int index = 0; index < pixelCount; ++index)
            {
                const int value = static_cast<int>(std::lround((static_cast<double>(pixelData[index]) * appliedRescaleSlope) + appliedRescaleIntercept));
                const int clampedValue = std::clamp(value, -32768, 32767);
                rawPixels[static_cast<std::size_t>(index)] = static_cast<int16_t>(clampedValue);
                minimumStoredValue = std::min(minimumStoredValue, clampedValue);
                maximumStoredValue = std::max(maximumStoredValue, clampedValue);
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
    auto patientMetadata = std::make_shared<DicomPatientMetadata>();
    patientMetadata->patientId = readStringTag(stringFilter, 0x0010, 0x0020);
    patientMetadata->patientName = readStringTag(stringFilter, 0x0010, 0x0010);
    patientMetadata->patientSex = readStringTag(stringFilter, 0x0010, 0x0040);
    patientMetadata->patientBirthDate = normalizeDicomDate(readStringTag(stringFilter, 0x0010, 0x0030));

    auto studyMetadata = std::make_shared<DicomStudyMetadata>();
    studyMetadata->patient = patientMetadata;
    studyMetadata->studyInstanceUid = readStringTag(stringFilter, 0x0020, 0x000D);
    studyMetadata->patientPosition = readStringTag(stringFilter, 0x0018, 0x5100);
    studyMetadata->studyDate = normalizeDicomDate(readStringTag(stringFilter, 0x0008, 0x0020));
    studyMetadata->studyTime = readStringTag(stringFilter, 0x0008, 0x0030);
    studyMetadata->studyDescription = readStringTag(stringFilter, 0x0008, 0x1030);
    studyMetadata->referringPhysicianName = readStringTag(stringFilter, 0x0008, 0x0090);

    auto seriesMetadata = std::make_shared<DicomSeriesMetadata>();
    seriesMetadata->study = studyMetadata;
    seriesMetadata->seriesInstanceUid = readStringTag(stringFilter, 0x0020, 0x000E);
    seriesMetadata->frameOfReferenceUid = readStringTag(stringFilter, 0x0020, 0x0052);
    seriesMetadata->seriesDate = normalizeDicomDate(readStringTag(stringFilter, 0x0008, 0x0021));
    seriesMetadata->seriesTime = readStringTag(stringFilter, 0x0008, 0x0031);
    seriesMetadata->seriesDescription = readStringTag(stringFilter, 0x0008, 0x103E);
    seriesMetadata->seriesNumber = readStringTag(stringFilter, 0x0020, 0x0011);
    seriesMetadata->modality = readStringTag(stringFilter, 0x0008, 0x0060);
    seriesMetadata->bodyPartExamined = readStringTag(stringFilter, 0x0018, 0x0015);
    seriesMetadata->protocolName = readStringTag(stringFilter, 0x0018, 0x1030);
    seriesMetadata->manufacturer = readStringTag(stringFilter, 0x0008, 0x0070);
    seriesMetadata->manufacturerModelName = readStringTag(stringFilter, 0x0008, 0x1090);
    seriesMetadata->institutionName = readStringTag(stringFilter, 0x0008, 0x0080);
    seriesMetadata->stationName = readStringTag(stringFilter, 0x0008, 0x1010);

    auto metadata = std::make_shared<DicomInstanceMetadata>();
    metadata->series = seriesMetadata;
    metadata->sopClassUid = readStringTag(stringFilter, 0x0008, 0x0016);
    metadata->sopInstanceUid = readStringTag(stringFilter, 0x0008, 0x0018);
    metadata->instanceNumber = readStringTag(stringFilter, 0x0020, 0x0013);
    metadata->imageType = readStringTag(stringFilter, 0x0008, 0x0008);
    metadata->acquisitionDate = normalizeDicomDate(readStringTag(stringFilter, 0x0008, 0x0022));
    metadata->acquisitionTime = readStringTag(stringFilter, 0x0008, 0x0032);
    metadata->contentDate = normalizeDicomDate(readStringTag(stringFilter, 0x0008, 0x0023));
    metadata->contentTime = readStringTag(stringFilter, 0x0008, 0x0033);
    metadata->rows = static_cast<int>(height);
    metadata->columns = static_cast<int>(width);
    metadata->samplesPerPixel = static_cast<int>(samplesPerPixel);
    metadata->bitsAllocated = static_cast<int>(bitsAllocated);
    metadata->bitsStored = static_cast<int>(gdcmImage.GetPixelFormat().GetBitsStored());
    metadata->highBit = static_cast<int>(gdcmImage.GetPixelFormat().GetHighBit());
    metadata->pixelRepresentation = isSignedPixelData ? 1 : 0;
    metadata->photometricInterpretation = readStringTag(stringFilter, 0x0028, 0x0004);
    metadata->rescaleType = readStringTag(stringFilter, 0x0028, 0x1054);
    metadata->voiLutFunction = readStringTag(stringFilter, 0x0028, 0x1056);
    metadata->hasRescaleSlope = hasRescaleSlope;
    metadata->rescaleSlope = appliedRescaleSlope;
    metadata->hasRescaleIntercept = hasRescaleIntercept;
    metadata->rescaleIntercept = appliedRescaleIntercept;
    metadata->windowPresets = readWindowPresets();

    const auto [pixelSpacingX, pixelSpacingY] = readPixelSpacingValues();
    metadata->pixelSpacingX = pixelSpacingX;
    metadata->pixelSpacingY = pixelSpacingY;
    metadata->hasPixelSpacing = pixelSpacingX > 0.0 && pixelSpacingY > 0.0;
    const auto [hasImagePositionPatient, imagePositionPatient] = readVector3Tag(0x0020, 0x0032);
    if (hasImagePositionPatient)
    {
        metadata->hasImagePositionPatient = true;
        metadata->imagePositionPatient = imagePositionPatient;
    }
    const auto [hasImageOrientationPatient, imageOrientationPatient] = readVector6Tag(0x0020, 0x0037);
    if (hasImageOrientationPatient)
    {
        metadata->hasImageOrientationPatient = true;
        metadata->imageOrientationPatient = imageOrientationPatient;
    }
    const auto [hasSliceThickness, sliceThickness] = readOptionalNumericTagValue(0x0018, 0x0050);
    metadata->hasSliceThickness = hasSliceThickness && sliceThickness > 0.0;
    metadata->sliceThickness = metadata->hasSliceThickness ? sliceThickness : 0.0;
    const auto [hasSpacingBetweenSlices, spacingBetweenSlices] = readOptionalNumericTagValue(0x0018, 0x0088);
    metadata->hasSpacingBetweenSlices = hasSpacingBetweenSlices && spacingBetweenSlices > 0.0;
    metadata->spacingBetweenSlices = metadata->hasSpacingBetweenSlices ? spacingBetweenSlices : 0.0;
    const auto [hasSliceLocation, sliceLocation] = readOptionalNumericTagValue(0x0020, 0x1041);
    metadata->hasSliceLocation = hasSliceLocation;
    metadata->sliceLocation = sliceLocation;
    const auto [hasGantryDetectorTilt, gantryDetectorTilt] = readOptionalNumericTagValue(0x0018, 0x1120);
    metadata->hasGantryDetectorTilt = hasGantryDetectorTilt;
    metadata->gantryDetectorTilt = gantryDetectorTilt;

    dicomImage->setMetadata(metadata);
    dicomImage->setSopInstanceUid(metadata->sopInstanceUid);
    dicomImage->setInstanceNumber(metadata->instanceNumber);
    dicomImage->setPixelSpacing(pixelSpacingX, pixelSpacingY);
    if (hasImagePositionPatient)
    {
        dicomImage->setImagePositionPatient(imagePositionPatient);
    }
    if (hasImageOrientationPatient)
    {
        dicomImage->setImageOrientationPatient(imageOrientationPatient);
    }
    dicomImage->setSliceThickness(metadata->sliceThickness);
    dicomImage->setSpacingBetweenSlices(metadata->spacingBetweenSlices);

    if (isMonochromeImage && !rawPixels.empty())
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
            dicomImage->setPixmap(createDicomPreviewPixmap(*dicomImage));
        }
    }
    else
    {
        dicomImage->setPixmap(QPixmap::fromImage(image));
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

    std::unique_ptr<DicomImage> dicomImage = loadDicomImage(filePath, reader, false);
    if (dicomImage)
    {
        if (series.previewPixmap().isNull())
        {
            series.setPreviewPixmap(createDicomPreviewPixmap(*dicomImage));
        }
        if (series.representativeFilePath().isEmpty())
        {
            series.setRepresentativeFilePath(filePath);
        }
        series.addImage(std::move(dicomImage));
    }

    return patient;
}
