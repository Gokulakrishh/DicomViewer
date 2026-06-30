#include "GDCMFileHandling.h"

#include "Model/DicomImage.h"
#include "Utilities/DiagnosticImageRenderer.h"

#include <QByteArray>
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QList>
#include <QPixmap>
#include <QSet>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <limits>
#include <new>
#include <queue>
#include <vector>
#include <gdcmImage.h>
#include <gdcmFile.h>
#include <gdcmFileMetaInformation.h>
#include <gdcmPhotometricInterpretation.h>
#include <gdcmReader.h>
#include <gdcmScanner.h>
#include <gdcmTag.h>
#include <gdcmTransferSyntax.h>

#if defined(Q_OS_WIN) && defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define CROSSAXIAL_HAS_WIN_SEH_GUARD 1
#else
#define CROSSAXIAL_HAS_WIN_SEH_GUARD 0
#endif

namespace
{
constexpr std::size_t kScannerBatchSize = 300;

const gdcm::Tag kPatientIdTag(0x0010, 0x0020);
const gdcm::Tag kPatientNameTag(0x0010, 0x0010);
const gdcm::Tag kIssuerOfPatientIdTag(0x0010, 0x0021);
const gdcm::Tag kPatientSexTag(0x0010, 0x0040);
const gdcm::Tag kPatientBirthDateTag(0x0010, 0x0030);
const gdcm::Tag kStudyInstanceUidTag(0x0020, 0x000D);
const gdcm::Tag kStudyDateTag(0x0008, 0x0020);
const gdcm::Tag kStudyDescriptionTag(0x0008, 0x1030);
const gdcm::Tag kReferringPhysicianTag(0x0008, 0x0090);
const gdcm::Tag kSeriesInstanceUidTag(0x0020, 0x000E);
const gdcm::Tag kSeriesDescriptionTag(0x0008, 0x103E);
const gdcm::Tag kSeriesNumberTag(0x0020, 0x0011);
const gdcm::Tag kSopInstanceUidTag(0x0008, 0x0018);
const gdcm::Tag kModalityTag(0x0008, 0x0060);
const gdcm::Tag kInstanceNumberTag(0x0020, 0x0013);
const gdcm::Tag kNumberOfFramesTag(0x0028, 0x0008);
const gdcm::Tag kRowsTag(0x0028, 0x0010);
const gdcm::Tag kColumnsTag(0x0028, 0x0011);

QString transferSyntaxUid(const gdcm::ImageReader& reader)
{
    const char* value = reader.GetFile().GetHeader().GetDataSetTransferSyntax().GetString();
    return value ? QString::fromLatin1(value).trimmed() : QString{};
}

QString transferSyntaxUid(const gdcm::File& file)
{
    const char* value = file.GetHeader().GetDataSetTransferSyntax().GetString();
    return value ? QString::fromLatin1(value).trimmed() : QString{};
}

qsizetype expectedFrameByteCount(
    unsigned int width,
    unsigned int height,
    unsigned int samplesPerPixel,
    unsigned int bitsAllocated)
{
    if (width == 0 || height == 0 || samplesPerPixel == 0 || bitsAllocated == 0)
    {
        return 0;
    }

    const unsigned int bytesPerSample = bitsAllocated <= 8 ? 1U : bitsAllocated <= 16 ? 2U : 0U;
    if (bytesPerSample == 0)
    {
        return 0;
    }

    const quint64 byteCount =
        static_cast<quint64>(width) *
        static_cast<quint64>(height) *
        static_cast<quint64>(samplesPerPixel) *
        static_cast<quint64>(bytesPerSample);
    if (byteCount > static_cast<quint64>(std::numeric_limits<qsizetype>::max()))
    {
        return 0;
    }

    return static_cast<qsizetype>(byteCount);
}

std::filesystem::path toFilesystemPath(const QString& path)
{
#if defined(Q_OS_WIN)
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

QString fromFilesystemPath(const std::filesystem::path& path)
{
#if defined(Q_OS_WIN)
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

QString scannedTagValue(const gdcm::Scanner::TagToValue& tagValues, const gdcm::Tag& tag)
{
    const auto it = tagValues.find(tag);
    if (it == tagValues.end() || it->second == nullptr)
    {
        return {};
    }

    return QString::fromUtf8(it->second).trimmed();
}

int scannedPositiveInt(const gdcm::Scanner::TagToValue& tagValues, const gdcm::Tag& tag, int fallbackValue = 0)
{
    bool ok = false;
    const int value = scannedTagValue(tagValues, tag).split('\\').value(0).trimmed().toInt(&ok);
    return ok && value > 0 ? value : fallbackValue;
}

void addFolderImportScannerTags(gdcm::Scanner& scanner)
{
    scanner.AddTag(kPatientIdTag);
    scanner.AddTag(kPatientNameTag);
    scanner.AddTag(kIssuerOfPatientIdTag);
    scanner.AddTag(kPatientSexTag);
    scanner.AddTag(kPatientBirthDateTag);
    scanner.AddTag(kStudyInstanceUidTag);
    scanner.AddTag(kStudyDateTag);
    scanner.AddTag(kStudyDescriptionTag);
    scanner.AddTag(kReferringPhysicianTag);
    scanner.AddTag(kSeriesInstanceUidTag);
    scanner.AddTag(kSeriesDescriptionTag);
    scanner.AddTag(kSeriesNumberTag);
    scanner.AddTag(kSopInstanceUidTag);
    scanner.AddTag(kModalityTag);
    scanner.AddTag(kInstanceNumberTag);
    scanner.AddTag(kNumberOfFramesTag);
    scanner.AddTag(kRowsTag);
    scanner.AddTag(kColumnsTag);
}

#if CROSSAXIAL_HAS_WIN_SEH_GUARD
void logWindowsHardwareExceptionForFile(
    const char* context,
    const QString& filePath,
    unsigned long exceptionCode)
{
    qCritical().noquote() << context
                          << "file=" << filePath
                          << "code=0x" + QString::number(exceptionCode, 16);
}

void logWindowsHardwareExceptionForFileDetail(
    const char* context,
    const QString& filePath,
    const QString& detail,
    unsigned long exceptionCode)
{
    qCritical().noquote() << context
                          << "file=" << filePath
                          << "detail=" << detail
                          << "code=0x" + QString::number(exceptionCode, 16);
}

void logWindowsHardwareExceptionForBatch(
    const char* context,
    std::size_t batchStart,
    std::size_t batchEnd,
    unsigned long exceptionCode)
{
    qCritical().noquote() << context
                          << "batch=" << static_cast<int>(batchStart) << "-" << static_cast<int>(batchEnd - 1)
                          << "code=0x" + QString::number(exceptionCode, 16);
}
#endif
}

GDCMFileHandling::GDCMFileHandling()
{
    m_supportedFormats
        << "*.dcm"
        << "*.dicom"
        << "*.ima"
        << "*.img";
}

bool GDCMFileHandling::readGdcmImageReader(gdcm::ImageReader& reader, const QString& filePath)
{
#if CROSSAXIAL_HAS_WIN_SEH_GUARD
    __try
    {
        return readGdcmImageReaderImpl(reader, filePath);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        const unsigned long exceptionCode = GetExceptionCode();
        logWindowsHardwareExceptionForFile("[DICOMRead] Windows hardware exception in GDCM image read:",
                                           filePath,
                                           exceptionCode);
        return false;
    }
#else
    return readGdcmImageReaderImpl(reader, filePath);
#endif
}

bool GDCMFileHandling::readGdcmImageReaderImpl(gdcm::ImageReader& reader, const QString& filePath)
{
    try
    {
        if (reader.Read())
        {
            return true;
        }

        qWarning().noquote() << "[DICOMRead] GDCM could not read image:"
                             << "file=" << filePath;
    }
    catch (const std::bad_alloc&)
    {
        qWarning().noquote() << "[DICOMRead] GDCM image read exhausted memory:"
                             << "file=" << filePath;
    }
    catch (const std::exception& exception)
    {
        qWarning().noquote() << "[DICOMRead] GDCM image read threw exception:"
                             << "file=" << filePath
                             << "error=" << exception.what();
    }
    catch (...)
    {
        qWarning().noquote() << "[DICOMRead] GDCM image read threw unknown exception:"
                             << "file=" << filePath;
    }

    return false;
}

bool GDCMFileHandling::readGdcmReader(gdcm::Reader& reader, const QString& filePath)
{
#if CROSSAXIAL_HAS_WIN_SEH_GUARD
    __try
    {
        return readGdcmReaderImpl(reader, filePath);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        const unsigned long exceptionCode = GetExceptionCode();
        logWindowsHardwareExceptionForFile("[DICOMRead] Windows hardware exception in GDCM hierarchy read:",
                                           filePath,
                                           exceptionCode);
        return false;
    }
#else
    return readGdcmReaderImpl(reader, filePath);
#endif
}

bool GDCMFileHandling::readGdcmReaderImpl(gdcm::Reader& reader, const QString& filePath)
{
    try
    {
        if (reader.Read())
        {
            return true;
        }

        qWarning().noquote() << "[DICOMRead] GDCM could not read hierarchy:"
                             << "file=" << filePath;
    }
    catch (const std::bad_alloc&)
    {
        qWarning().noquote() << "[DICOMRead] GDCM hierarchy read exhausted memory:"
                             << "file=" << filePath;
    }
    catch (const std::exception& exception)
    {
        qWarning().noquote() << "[DICOMRead] GDCM hierarchy read threw exception:"
                             << "file=" << filePath
                             << "error=" << exception.what();
    }
    catch (...)
    {
        qWarning().noquote() << "[DICOMRead] GDCM hierarchy read threw unknown exception:"
                             << "file=" << filePath;
    }

    return false;
}

bool GDCMFileHandling::scanGdcmMetadataBatch(
    gdcm::Scanner& scanner,
    const gdcm::Directory::FilenamesType& filenames,
    std::size_t batchStart,
    std::size_t batchEnd)
{
#if CROSSAXIAL_HAS_WIN_SEH_GUARD
    __try
    {
        return scanGdcmMetadataBatchImpl(scanner, filenames, batchStart, batchEnd);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        const unsigned long exceptionCode = GetExceptionCode();
        logWindowsHardwareExceptionForBatch("[DICOMImport] Windows hardware exception in GDCM metadata scan:",
                                            batchStart,
                                            batchEnd,
                                            exceptionCode);
        return false;
    }
#else
    return scanGdcmMetadataBatchImpl(scanner, filenames, batchStart, batchEnd);
#endif
}

bool GDCMFileHandling::scanGdcmMetadataBatchImpl(
    gdcm::Scanner& scanner,
    const gdcm::Directory::FilenamesType& filenames,
    std::size_t batchStart,
    std::size_t batchEnd)
{
    try
    {
        if (!scanner.Scan(filenames))
        {
            qWarning().noquote() << "[DICOMImport] GDCM metadata scan reported failure for batch"
                                 << static_cast<int>(batchStart) << "-" << static_cast<int>(batchEnd - 1);
        }
        return true;
    }
    catch (const std::bad_alloc&)
    {
        qWarning().noquote() << "[DICOMImport] GDCM metadata scan exhausted memory for batch"
                             << static_cast<int>(batchStart) << "-" << static_cast<int>(batchEnd - 1);
    }
    catch (const std::exception& exception)
    {
        qWarning().noquote() << "[DICOMImport] GDCM metadata scan threw exception for batch"
                             << static_cast<int>(batchStart) << "-" << static_cast<int>(batchEnd - 1)
                             << "error=" << exception.what();
    }
    catch (...)
    {
        qWarning().noquote() << "[DICOMImport] GDCM metadata scan threw unknown exception for batch"
                             << static_cast<int>(batchStart) << "-" << static_cast<int>(batchEnd - 1);
    }

    return false;
}

bool GDCMFileHandling::getGdcmImageBuffer(
    const gdcm::Image& gdcmImage,
    char* targetBuffer,
    const QString& filePath,
    const QString& transferSyntax)
{
#if CROSSAXIAL_HAS_WIN_SEH_GUARD
    __try
    {
        return getGdcmImageBufferImpl(gdcmImage, targetBuffer, filePath, transferSyntax);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        const unsigned long exceptionCode = GetExceptionCode();
        logWindowsHardwareExceptionForFileDetail("[DICOMDecode] Windows hardware exception in GDCM buffer decode:",
                                                 filePath,
                                                 transferSyntax,
                                                 exceptionCode);
        return false;
    }
#else
    return getGdcmImageBufferImpl(gdcmImage, targetBuffer, filePath, transferSyntax);
#endif
}

bool GDCMFileHandling::getGdcmImageBufferImpl(
    const gdcm::Image& gdcmImage,
    char* targetBuffer,
    const QString& filePath,
    const QString& transferSyntax)
{
    try
    {
        if (gdcmImage.GetBuffer(targetBuffer))
        {
            return true;
        }

        qWarning().noquote()
            << "[DICOMDecode] Failed to decode DICOM pixel buffer:"
            << "file=" << filePath
            << "transferSyntax=" << transferSyntax;
    }
    catch (const std::bad_alloc&)
    {
        qWarning().noquote()
            << "[DICOMDecode] Not enough memory while decoding DICOM pixel buffer:"
            << "file=" << filePath
            << "transferSyntax=" << transferSyntax;
    }
    catch (const std::exception& exception)
    {
        qWarning().noquote()
            << "[DICOMDecode] Exception while decoding DICOM pixel buffer:"
            << "file=" << filePath
            << "transferSyntax=" << transferSyntax
            << "error=" << exception.what();
    }
    catch (...)
    {
        qWarning().noquote()
            << "[DICOMDecode] Unknown exception while decoding DICOM pixel buffer:"
            << "file=" << filePath
            << "transferSyntax=" << transferSyntax;
    }

    return false;
}

FileHandling::PatientList GDCMFileHandling::loadDicomFolder(const QString& folderPath, ProgressCallback progressCallback)
{
    PatientList patients;
    std::map<QString, PatientPtr> patientMap;

    const std::vector<QString> regularFiles = collectRegularFilesBfs(folderPath);
    const std::vector<QString> dicomCandidates = filterDicomCandidates(regularFiles);
    scanDicomCandidatesInBatches(dicomCandidates, patientMap, std::move(progressCallback));

    for (const auto& [patientId, patient] : patientMap)
    {
        Q_UNUSED(patientId);
        patients.append(patient);
    }

    return patients;
}

std::vector<QString> GDCMFileHandling::collectRegularFilesBfs(const QString& folderPath) const
{
    std::vector<QString> files;

    std::error_code errorCode;
    const std::filesystem::path rootPath = toFilesystemPath(folderPath);
    if (!std::filesystem::exists(rootPath, errorCode) || !std::filesystem::is_directory(rootPath, errorCode))
    {
        if (errorCode)
        {
            qWarning().noquote() << "[DICOMImport] folder access failed:" << folderPath
                                 << QString::fromStdString(errorCode.message());
        }
        return files;
    }

    std::queue<std::filesystem::path> pendingDirectories;
    pendingDirectories.push(rootPath);

    while (!pendingDirectories.empty())
    {
        const std::filesystem::path currentDirectory = pendingDirectories.front();
        pendingDirectories.pop();

        std::error_code iteratorError;
        std::filesystem::directory_iterator iterator(currentDirectory, iteratorError);
        if (iteratorError)
        {
            qWarning().noquote() << "[DICOMImport] folder traversal stopped:"
                                 << fromFilesystemPath(currentDirectory)
                                 << QString::fromStdString(iteratorError.message());
            return files;
        }

        const std::filesystem::directory_iterator endIterator;
        for (; iterator != endIterator; iterator.increment(iteratorError))
        {
            if (iteratorError)
            {
                qWarning().noquote() << "[DICOMImport] folder traversal stopped:"
                                     << fromFilesystemPath(currentDirectory)
                                     << QString::fromStdString(iteratorError.message());
                return files;
            }

            std::error_code typeError;
            if (iterator->is_directory(typeError))
            {
                pendingDirectories.push(iterator->path());
                continue;
            }
            if (typeError)
            {
                qWarning().noquote() << "[DICOMImport] folder traversal stopped:"
                                     << fromFilesystemPath(iterator->path())
                                     << QString::fromStdString(typeError.message());
                return files;
            }

            if (iterator->is_regular_file(typeError))
            {
                files.push_back(fromFilesystemPath(iterator->path()));
                continue;
            }
            if (typeError)
            {
                qWarning().noquote() << "[DICOMImport] folder traversal stopped:"
                                     << fromFilesystemPath(iterator->path())
                                     << QString::fromStdString(typeError.message());
                return files;
            }
        }
    }

    return files;
}

std::vector<QString> GDCMFileHandling::filterDicomCandidates(const std::vector<QString>& files) const
{
    std::vector<QString> candidates;
    candidates.reserve(files.size());
    for (const QString& filePath : files)
    {
        if (looksLikePart10Dicom(filePath))
        {
            candidates.push_back(filePath);
        }
    }
    return candidates;
}

bool GDCMFileHandling::looksLikePart10Dicom(const QString& filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const bool result = (f.size() >= 132)
        && f.seek(128)
        && (f.read(4) == QByteArray("DICM"));
    f.close();
    return result;
}

void GDCMFileHandling::scanDicomCandidatesInBatches(
    const std::vector<QString>& candidates,
    std::map<QString, PatientPtr>& patientMap,
    ProgressCallback progressCallback) const
{
    const int totalCandidates = static_cast<int>(candidates.size());
    int processedCandidates = 0;

    for (std::size_t batchStart = 0; batchStart < candidates.size(); batchStart += kScannerBatchSize)
    {
        const std::size_t batchEnd = std::min(batchStart + kScannerBatchSize, candidates.size());

        gdcm::Scanner scanner;
        addFolderImportScannerTags(scanner);

        gdcm::Directory::FilenamesType filenames;
        filenames.reserve(batchEnd - batchStart);
        for (std::size_t index = batchStart; index < batchEnd; ++index)
        {
            filenames.push_back(candidates[index].toStdString());
        }

        if (!scanGdcmMetadataBatch(scanner, filenames, batchStart, batchEnd))
        {
            processedCandidates += static_cast<int>(filenames.size());
            if (progressCallback)
            {
                progressCallback(processedCandidates, totalCandidates);
            }
            continue;
        }

        for (std::size_t batchIndex = 0; batchIndex < filenames.size(); ++batchIndex)
        {
            const std::string& scannerPath = filenames[batchIndex];
            const QString& candidatePath = candidates[batchStart + batchIndex];
            if (scanner.IsKey(scannerPath.c_str()))
            {
                const gdcm::Scanner::TagToValue& tagValues = scanner.GetMapping(scannerPath.c_str());
                PatientPtr patient = buildHierarchyFromScannedTags(candidatePath, tagValues);
                if (patient)
                {
                    const QString issuerOfPatientId = scannedTagValue(tagValues, kIssuerOfPatientIdTag);
                    const QString patientKey = patient->patientId() + "|" + issuerOfPatientId;
                    auto it = patientMap.find(patientKey);
                    if (it == patientMap.end())
                    {
                        patientMap.emplace(patientKey, patient);
                    }
                    else
                    {
                        mergePatientHierarchy(patient, *it->second);
                    }
                }
            }

            ++processedCandidates;
            if (progressCallback)
            {
                progressCallback(processedCandidates, totalCandidates);
            }
        }
    }
}

std::unique_ptr<MedicalImage> GDCMFileHandling::loadImage(const QString& filePath)
{
    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!readGdcmImageReader(reader, filePath))
    {
        return nullptr;
    }

    return loadDicomImage(filePath, reader, false);
}

std::unique_ptr<DicomImage> GDCMFileHandling::loadImageData(const QString& filePath, int frameIndex) const
{
    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!readGdcmImageReader(reader, filePath))
    {
        return nullptr;
    }

    return loadDicomImage(filePath, reader, false, frameIndex);
}

bool GDCMFileHandling::visitImageDataFrames(
    const QString& filePath,
    int firstFrameIndex,
    int lastFrameIndex,
    const ImageFrameVisitor& visitor,
    const CancellationCheck& isCancelled,
    QString* errorMessage) const
{
    if (!visitor || firstFrameIndex < 0 || lastFrameIndex < firstFrameIndex)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Invalid DICOM frame visitor request.");
        }
        return false;
    }

    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());
    if (!readGdcmImageReader(reader, filePath))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("GDCM could not read the selected cine source.");
        }
        return false;
    }

    QVector<char> decodedBuffer;
    if (!decodePixelBuffer(filePath, reader, decodedBuffer))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("GDCM could not decode the selected cine source.");
        }
        return false;
    }

    const std::shared_ptr<DicomInstanceMetadata> metadata = extractDicomInstanceMetadata(reader);
    const int frameCount = metadata ? std::max(1, metadata->numberOfFrames) : 1;
    if (lastFrameIndex >= frameCount)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("The selected cine frame range exceeds the source frame count.");
        }
        return false;
    }

    for (int frameIndex = firstFrameIndex; frameIndex <= lastFrameIndex; ++frameIndex)
    {
        if (isCancelled && isCancelled())
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("DICOM frame visitation was cancelled.");
            }
            return false;
        }

        std::unique_ptr<DicomImage> frame = loadDicomImage(
            filePath,
            reader,
            false,
            frameIndex,
            &decodedBuffer);
        if (!frame)
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("Failed to decode cine frame %1.").arg(frameIndex + 1);
            }
            return false;
        }

        QString visitorError;
        if (!visitor(frameIndex, *frame, &visitorError))
        {
            if (errorMessage)
            {
                *errorMessage = visitorError;
            }
            return false;
        }
    }

    return true;
}

QHash<int, std::shared_ptr<DicomImage>> GDCMFileHandling::loadImageDataFrames(
    const QString& filePath,
    const QList<int>& frameIndices) const
{
    QHash<int, std::shared_ptr<DicomImage>> frames;
    if (frameIndices.isEmpty())
    {
        return frames;
    }

    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());
    if (!readGdcmImageReader(reader, filePath))
    {
        return frames;
    }

    QVector<char> decodedBuffer;
    if (!decodePixelBuffer(filePath, reader, decodedBuffer))
    {
        return frames;
    }

    QSet<int> uniqueFrameIndices;
    for (int frameIndex : frameIndices)
    {
        uniqueFrameIndices.insert(std::max(0, frameIndex));
    }

    for (int frameIndex : uniqueFrameIndices)
    {
        std::unique_ptr<DicomImage> loadedImage = loadDicomImage(
            filePath,
            reader,
            false,
            frameIndex,
            &decodedBuffer);
        if (loadedImage)
        {
            frames.insert(frameIndex, std::shared_ptr<DicomImage>(loadedImage.release()));
        }
    }

    return frames;
}

FileHandling::PatientPtr GDCMFileHandling::loadDicomHierarchy(const QString& filePath)
{
    gdcm::Reader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!readGdcmReader(reader, filePath))
    {
        return nullptr;
    }

    return buildHierarchy(filePath, reader.GetFile());
}

QStringList GDCMFileHandling::getSupportedFormats() const
{
    return m_supportedFormats;
}

bool GDCMFileHandling::canLoad(const QString& filePath) const
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile() || !info.isReadable())
    {
        return false;
    }

    return looksLikePart10Dicom(filePath);
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
                copiedImage->setFrameCount(sourceImage->frameCount());
                copiedImage->setFrameIndex(sourceImage->frameIndex());
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

bool GDCMFileHandling::decodePixelBuffer(
    const QString& filePath,
    const gdcm::ImageReader& reader,
    QVector<char>& decodedBuffer) const
{
    const gdcm::Image& gdcmImage = reader.GetImage();
    const unsigned long bufferLength = gdcmImage.GetBufferLength();
    if (bufferLength == 0)
    {
        qWarning().noquote()
            << "[DICOMDecode] Empty decoded pixel buffer requested:"
            << "file=" << filePath
            << "transferSyntax=" << transferSyntaxUid(reader);
        decodedBuffer.clear();
        return false;
    }

    if (static_cast<unsigned long long>(bufferLength) >
        static_cast<unsigned long long>(std::numeric_limits<qsizetype>::max()))
    {
        qWarning().noquote()
            << "[DICOMDecode] Decoded pixel buffer is too large for this build:"
            << "file=" << filePath
            << "bytes=" << QString::number(static_cast<qulonglong>(bufferLength))
            << "transferSyntax=" << transferSyntaxUid(reader);
        decodedBuffer.clear();
        return false;
    }

    try
    {
        decodedBuffer.resize(static_cast<qsizetype>(bufferLength));
        const QString transferSyntax = transferSyntaxUid(reader);
        if (!getGdcmImageBuffer(gdcmImage, decodedBuffer.data(), filePath, transferSyntax))
        {
            decodedBuffer.clear();
            return false;
        }
    }
    catch (const std::bad_alloc&)
    {
        qWarning().noquote()
            << "[DICOMDecode] Not enough memory to decode DICOM pixel buffer:"
            << "file=" << filePath
            << "bytes=" << QString::number(static_cast<qulonglong>(bufferLength))
            << "transferSyntax=" << transferSyntaxUid(reader);
        decodedBuffer.clear();
        return false;
    }
    catch (const std::exception& exception)
    {
        qWarning().noquote()
            << "[DICOMDecode] Exception while decoding DICOM pixel buffer:"
            << "file=" << filePath
            << "transferSyntax=" << transferSyntaxUid(reader)
            << "error=" << exception.what();
        decodedBuffer.clear();
        return false;
    }

    return true;
}

std::shared_ptr<DicomInstanceMetadata> GDCMFileHandling::extractDicomInstanceMetadata(const gdcm::File& file) const
{
    gdcm::StringFilter stringFilter;
    stringFilter.SetFile(file);

    const auto readUnsignedIntTagValue = [this, &stringFilter](uint16_t group, uint16_t element) {
        const QString tagValue = readStringTag(stringFilter, group, element);
        bool ok = false;
        const uint value = tagValue.split('\\').value(0).trimmed().toUInt(&ok);
        return ok ? value : 0U;
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
    const auto calculateFrameIntervalMs = [](double frameTimeMs, double cineRateFps, const std::vector<double>& frameTimeVectorMs) {
        if (!frameTimeVectorMs.empty() && frameTimeVectorMs.front() > 0.0)
        {
            return frameTimeVectorMs.front();
        }
        if (frameTimeMs > 0.0)
        {
            return frameTimeMs;
        }
        if (cineRateFps > 0.0)
        {
            return 1000.0 / cineRateFps;
        }
        return 100.0;
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

    const unsigned int width = readUnsignedIntTagValue(0x0028, 0x0011);
    const unsigned int height = readUnsignedIntTagValue(0x0028, 0x0010);
    const unsigned int samplesPerPixel = readUnsignedIntTagValue(0x0028, 0x0002);
    const unsigned int bitsAllocated = readUnsignedIntTagValue(0x0028, 0x0100);
    const unsigned int bitsStored = readUnsignedIntTagValue(0x0028, 0x0101);
    const unsigned int highBit = readUnsignedIntTagValue(0x0028, 0x0102);
    const bool isSignedPixelData = readUnsignedIntTagValue(0x0028, 0x0103) != 0;
    const int numberOfFrames = std::max(1, readStringTag(stringFilter, 0x0028, 0x0008).trimmed().toInt());
    const auto [hasRescaleSlope, rescaleSlope] = readOptionalNumericTagValue(0x0028, 0x1053);
    const auto [hasRescaleIntercept, rescaleIntercept] = readOptionalNumericTagValue(0x0028, 0x1052);
    const auto [hasFrameTime, frameTimeMs] = readOptionalNumericTagValue(0x0018, 0x1063);
    const auto [hasCineRate, cineRateFps] = readOptionalNumericTagValue(0x0018, 0x0040);
    std::vector<double> frameTimeVectorMs = readNumericComponents(0x0018, 0x1065);
    frameTimeVectorMs.erase(
        std::remove_if(
            frameTimeVectorMs.begin(),
            frameTimeVectorMs.end(),
            [](double value) { return value <= 0.0; }),
        frameTimeVectorMs.end());
    const double frameIntervalMs = calculateFrameIntervalMs(
        hasFrameTime ? frameTimeMs : 0.0,
        hasCineRate ? cineRateFps : 0.0,
        frameTimeVectorMs);

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
    metadata->numberOfFrames = numberOfFrames;
    metadata->frameTimeMs = hasFrameTime && frameTimeMs > 0.0 ? frameTimeMs : 0.0;
    metadata->cineRateFps = hasCineRate && cineRateFps > 0.0 ? cineRateFps : 0.0;
    metadata->frameIntervalMs = frameIntervalMs;
    metadata->frameTimeVectorMs = frameTimeVectorMs;
    metadata->bitsAllocated = static_cast<int>(bitsAllocated);
    metadata->bitsStored = static_cast<int>(bitsStored);
    metadata->highBit = static_cast<int>(highBit);
    metadata->pixelRepresentation = isSignedPixelData ? 1 : 0;
    metadata->transferSyntaxUid = transferSyntaxUid(file);
    metadata->photometricInterpretation = readStringTag(stringFilter, 0x0028, 0x0004);
    metadata->rescaleType = readStringTag(stringFilter, 0x0028, 0x1054);
    metadata->voiLutFunction = readStringTag(stringFilter, 0x0028, 0x1056);
    metadata->hasRescaleSlope = hasRescaleSlope;
    metadata->rescaleSlope = hasRescaleSlope ? rescaleSlope : 1.0;
    metadata->hasRescaleIntercept = hasRescaleIntercept;
    metadata->rescaleIntercept = hasRescaleIntercept ? rescaleIntercept : 0.0;
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

    return metadata;
}

std::shared_ptr<DicomInstanceMetadata> GDCMFileHandling::extractDicomInstanceMetadata(const gdcm::ImageReader& reader) const
{
    std::shared_ptr<DicomInstanceMetadata> metadata = extractDicomInstanceMetadata(reader.GetFile());
    if (!metadata)
    {
        return nullptr;
    }

    const gdcm::Image& gdcmImage = reader.GetImage();
    metadata->columns = static_cast<int>(gdcmImage.GetDimensions()[0]);
    metadata->rows = static_cast<int>(gdcmImage.GetDimensions()[1]);
    metadata->samplesPerPixel = static_cast<int>(gdcmImage.GetPixelFormat().GetSamplesPerPixel());
    metadata->bitsAllocated = static_cast<int>(gdcmImage.GetPixelFormat().GetBitsAllocated());
    metadata->bitsStored = static_cast<int>(gdcmImage.GetPixelFormat().GetBitsStored());
    metadata->highBit = static_cast<int>(gdcmImage.GetPixelFormat().GetHighBit());
    metadata->pixelRepresentation = gdcmImage.GetPixelFormat().GetPixelRepresentation() != 0 ? 1 : 0;
    metadata->transferSyntaxUid = transferSyntaxUid(reader);
    return metadata;
}

void GDCMFileHandling::applyMetadataToDicomImage(
    DicomImage& dicomImage,
    const std::shared_ptr<DicomInstanceMetadata>& metadata,
    int frameIndex) const
{
    if (!metadata)
    {
        return;
    }

    const int frameCount = std::max(1, metadata->numberOfFrames);
    dicomImage.setMetadata(metadata);
    dicomImage.setSopInstanceUid(metadata->sopInstanceUid);
    dicomImage.setInstanceNumber(metadata->instanceNumber);
    dicomImage.setFrameCount(frameCount);
    dicomImage.setFrameIndex(std::clamp(frameIndex, 0, frameCount - 1));
    dicomImage.setCineTiming(metadata->frameTimeMs, metadata->cineRateFps, metadata->frameIntervalMs);
    dicomImage.setPixelSpacing(metadata->pixelSpacingX, metadata->pixelSpacingY);
    if (metadata->hasImagePositionPatient)
    {
        dicomImage.setImagePositionPatient(metadata->imagePositionPatient);
    }
    if (metadata->hasImageOrientationPatient)
    {
        dicomImage.setImageOrientationPatient(metadata->imageOrientationPatient);
    }
    dicomImage.setSliceThickness(metadata->sliceThickness);
    dicomImage.setSpacingBetweenSlices(metadata->spacingBetweenSlices);
}

std::unique_ptr<DicomImage> GDCMFileHandling::loadDicomImage(
    const QString& filePath,
    const gdcm::ImageReader& reader,
    bool renderPixmap,
    int frameIndex,
    const QVector<char>* decodedBuffer) const
{
    const gdcm::Image& gdcmImage = reader.GetImage();
    const unsigned int width = gdcmImage.GetDimensions()[0];
    const unsigned int height = gdcmImage.GetDimensions()[1];
    const unsigned int samplesPerPixel = gdcmImage.GetPixelFormat().GetSamplesPerPixel();
    const unsigned int bitsAllocated = gdcmImage.GetPixelFormat().GetBitsAllocated();
    const bool isSignedPixelData = gdcmImage.GetPixelFormat().GetPixelRepresentation() != 0;
    const bool isMonochrome1 =
        gdcmImage.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::MONOCHROME1;

    QVector<char> ownedBuffer;
    const QVector<char>* buffer = decodedBuffer;
    if (!buffer)
    {
        if (!decodePixelBuffer(filePath, reader, ownedBuffer))
        {
            return nullptr;
        }
        buffer = &ownedBuffer;
    }

    gdcm::StringFilter stringFilter;
    stringFilter.SetFile(reader.GetFile());
    const std::shared_ptr<DicomInstanceMetadata> metadata = extractDicomInstanceMetadata(reader);
    const int numberOfFrames = metadata ? std::max(1, metadata->numberOfFrames) : 1;
    const int selectedFrameIndex = std::clamp(frameIndex, 0, numberOfFrames - 1);
    const qsizetype frameByteCount = expectedFrameByteCount(width, height, std::max(1U, samplesPerPixel), bitsAllocated);
    if (frameByteCount <= 0)
    {
        qWarning().noquote()
            << "[DICOMDecode] Invalid decoded frame size:"
            << "file=" << filePath
            << "frame=" << selectedFrameIndex
            << "frames=" << numberOfFrames
            << "frameBytes=" << frameByteCount
            << "bufferBytes=" << buffer->size()
            << "transferSyntax=" << transferSyntaxUid(reader);
        return nullptr;
    }
    if (selectedFrameIndex > 0 &&
        frameByteCount > std::numeric_limits<qsizetype>::max() / selectedFrameIndex)
    {
        qWarning().noquote()
            << "[DICOMDecode] Frame offset overflow:"
            << "file=" << filePath
            << "frame=" << selectedFrameIndex
            << "frames=" << numberOfFrames
            << "frameBytes=" << frameByteCount
            << "bufferBytes=" << buffer->size()
            << "transferSyntax=" << transferSyntaxUid(reader);
        return nullptr;
    }

    const qsizetype frameOffset = frameByteCount * selectedFrameIndex;
    if (frameOffset < 0 ||
        frameOffset > std::numeric_limits<qsizetype>::max() - frameByteCount ||
        frameOffset + frameByteCount > buffer->size())
    {
        qWarning().noquote()
            << "[DICOMDecode] Decoded buffer does not contain the requested frame:"
            << "file=" << filePath
            << "frame=" << selectedFrameIndex
            << "frames=" << numberOfFrames
            << "frameBytes=" << frameByteCount
            << "bufferBytes=" << buffer->size()
            << "transferSyntax=" << transferSyntaxUid(reader);
        return nullptr;
    }
    const char* frameData = buffer->constData() + frameOffset;

    constexpr quint64 maxInt = static_cast<quint64>(std::numeric_limits<int>::max());
    const quint64 width64 = static_cast<quint64>(width);
    const quint64 height64 = static_cast<quint64>(height);
    const quint64 pixelCount64 = width64 * height64;
    if (width64 == 0 ||
        height64 == 0 ||
        width64 > maxInt ||
        height64 > maxInt ||
        pixelCount64 == 0 ||
        pixelCount64 > maxInt)
    {
        qWarning().noquote()
            << "[DICOMDecode] Image dimensions out of range:"
            << "file=" << filePath
            << "size=" << QStringLiteral("%1x%2").arg(width).arg(height)
            << "transferSyntax=" << transferSyntaxUid(reader);
        return nullptr;
    }
    const int imageWidth = static_cast<int>(width);
    const int imageHeight = static_cast<int>(height);
    const int pixelCount = static_cast<int>(pixelCount64);

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
    QImage image;
    std::vector<int16_t> rawPixels;
    bool isMonochromeImage = false;
    int minimumStoredValue = 0;
    int maximumStoredValue = 255;
    int defaultWindowLevel = 0;
    int defaultWindowWidth = 255;
    const double appliedRescaleSlope = metadata && metadata->hasRescaleSlope ? metadata->rescaleSlope : 1.0;
    const double appliedRescaleIntercept = metadata && metadata->hasRescaleIntercept ? metadata->rescaleIntercept : 0.0;
    if (samplesPerPixel == 3 && bitsAllocated <= 8)
    {
        const quint64 rgbBytesPerLine64 = static_cast<quint64>(width) * 3U;
        if (rgbBytesPerLine64 > maxInt)
        {
            qWarning().noquote()
                << "[DICOMDecode] RGB bytes-per-line out of range:"
                << "file=" << filePath
                << "width=" << width
                << "transferSyntax=" << transferSyntaxUid(reader);
            return nullptr;
        }

        image = QImage(
                    reinterpret_cast<const uchar*>(frameData),
                    imageWidth,
                    imageHeight,
                    static_cast<int>(rgbBytesPerLine64),
                    QImage::Format_RGB888)
                    .rgbSwapped()
                    .copy();
    }
    else if (samplesPerPixel == 1 && bitsAllocated <= 8)
    {
        isMonochromeImage = true;
        image = QImage(
                    reinterpret_cast<const uchar*>(frameData),
                    imageWidth,
                    imageHeight,
                    imageWidth,
                    QImage::Format_Grayscale8)
                    .copy();

        rawPixels.resize(static_cast<std::size_t>(pixelCount));
        minimumStoredValue = std::numeric_limits<int>::max();
        maximumStoredValue = std::numeric_limits<int>::min();
        const auto* pixelData = reinterpret_cast<const uint8_t*>(frameData);
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
        image = QImage(imageWidth, imageHeight, QImage::Format_Grayscale8);

        minimumStoredValue = std::numeric_limits<int>::max();
        maximumStoredValue = std::numeric_limits<int>::min();
        rawPixels.resize(static_cast<std::size_t>(pixelCount));
        const auto storePixel = [&](int index, int storedValue) {
            const int value = static_cast<int>(
                std::lround((static_cast<double>(storedValue) * appliedRescaleSlope) + appliedRescaleIntercept));
            const int clampedValue = std::clamp(value, -32768, 32767);
            rawPixels[static_cast<std::size_t>(index)] = static_cast<int16_t>(clampedValue);
            minimumStoredValue = std::min(minimumStoredValue, clampedValue);
            maximumStoredValue = std::max(maximumStoredValue, clampedValue);
        };

        const bool isAligned =
            reinterpret_cast<std::uintptr_t>(frameData) % alignof(uint16_t) == 0;
        if (!isAligned)
        {
            qWarning().noquote()
                << "[DICOMDecode] Unaligned 16-bit pixel buffer, using safe copy path:"
                << "file=" << filePath
                << "transferSyntax=" << transferSyntaxUid(reader);
        }

        if (isSignedPixelData)
        {
            if (isAligned)
            {
                const auto* pixelData = reinterpret_cast<const int16_t*>(frameData);
                for (int index = 0; index < pixelCount; ++index)
                {
                    storePixel(index, static_cast<int>(pixelData[index]));
                }
            }
            else
            {
                for (int index = 0; index < pixelCount; ++index)
                {
                    int16_t storedValue = 0;
                    std::memcpy(&storedValue, frameData + (static_cast<std::size_t>(index) * sizeof(storedValue)), sizeof(storedValue));
                    storePixel(index, static_cast<int>(storedValue));
                }
            }
        }
        else
        {
            if (isAligned)
            {
                const auto* pixelData = reinterpret_cast<const uint16_t*>(frameData);
                for (int index = 0; index < pixelCount; ++index)
                {
                    storePixel(index, static_cast<int>(pixelData[index]));
                }
            }
            else
            {
                for (int index = 0; index < pixelCount; ++index)
                {
                    uint16_t storedValue = 0;
                    std::memcpy(&storedValue, frameData + (static_cast<std::size_t>(index) * sizeof(storedValue)), sizeof(storedValue));
                    storePixel(index, static_cast<int>(storedValue));
                }
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
        return nullptr;
    }

    auto dicomImage = std::make_unique<DicomImage>();
    dicomImage->setFilePath(filePath);
    dicomImage->setDimensions(imageWidth, imageHeight);
    applyMetadataToDicomImage(*dicomImage, metadata, selectedFrameIndex);

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

std::unique_ptr<DicomImage> GDCMFileHandling::createMetadataOnlyDicomImage(
    const QString& filePath,
    const gdcm::File& file) const
{
    const std::shared_ptr<DicomInstanceMetadata> metadata = extractDicomInstanceMetadata(file);
    if (!metadata)
    {
        return nullptr;
    }

    auto dicomImage = std::make_unique<DicomImage>();
    dicomImage->setFilePath(filePath);
    dicomImage->setDimensions(metadata->columns, metadata->rows);
    applyMetadataToDicomImage(*dicomImage, metadata, 0);
    if (!metadata->windowPresets.empty())
    {
        dicomImage->setDefaultWindow(
            static_cast<int>(std::lround(metadata->windowPresets.front().center)),
            std::max(1, static_cast<int>(std::lround(metadata->windowPresets.front().width))));
    }

    if (dicomImage->sopInstanceUid().isEmpty())
    {
        const QFileInfo fileInfo(filePath);
        dicomImage->setSopInstanceUid(fileInfo.completeBaseName());
    }

    return dicomImage;
}

std::unique_ptr<DicomImage> GDCMFileHandling::createMetadataOnlyDicomImage(
    const QString& filePath,
    const gdcm::ImageReader& reader) const
{
    return createMetadataOnlyDicomImage(filePath, reader.GetFile());
}

FileHandling::PatientPtr GDCMFileHandling::buildHierarchy(const QString& filePath, const gdcm::File& file) const
{
    gdcm::StringFilter stringFilter;
    stringFilter.SetFile(file);

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

    std::unique_ptr<DicomImage> dicomImage = createMetadataOnlyDicomImage(filePath, file);
    if (dicomImage)
    {
        if (series.representativeFilePath().isEmpty())
        {
            series.setRepresentativeFilePath(filePath);
        }
        series.addImage(std::move(dicomImage));
    }

    return patient;
}

FileHandling::PatientPtr GDCMFileHandling::buildHierarchy(const QString& filePath, const gdcm::ImageReader& reader) const
{
    return buildHierarchy(filePath, reader.GetFile());
}

FileHandling::PatientPtr GDCMFileHandling::buildHierarchyFromScannedTags(
    const QString& filePath,
    const gdcm::Scanner::TagToValue& tagValues) const
{
    auto patientMetadata = std::make_shared<DicomPatientMetadata>();
    patientMetadata->patientId = scannedTagValue(tagValues, kPatientIdTag);
    patientMetadata->patientName = scannedTagValue(tagValues, kPatientNameTag);
    patientMetadata->patientSex = scannedTagValue(tagValues, kPatientSexTag);
    patientMetadata->patientBirthDate = normalizeDicomDate(scannedTagValue(tagValues, kPatientBirthDateTag));
    if (patientMetadata->patientId.isEmpty())
    {
        patientMetadata->patientId = QStringLiteral("UNKNOWN_PATIENT");
    }

    auto patient = std::make_shared<Patient>();
    patient->setPatientId(patientMetadata->patientId);
    patient->setPatientName(patientMetadata->patientName);
    patient->setPatientSex(patientMetadata->patientSex);
    patient->setDateOfBirth(patientMetadata->patientBirthDate);

    auto studyMetadata = std::make_shared<DicomStudyMetadata>();
    studyMetadata->patient = patientMetadata;
    studyMetadata->studyInstanceUid = scannedTagValue(tagValues, kStudyInstanceUidTag);
    if (studyMetadata->studyInstanceUid.isEmpty())
    {
        studyMetadata->studyInstanceUid = patientMetadata->patientId + QStringLiteral("_STUDY");
    }
    studyMetadata->studyDate = normalizeDicomDate(scannedTagValue(tagValues, kStudyDateTag));
    studyMetadata->studyDescription = scannedTagValue(tagValues, kStudyDescriptionTag);
    studyMetadata->referringPhysicianName = scannedTagValue(tagValues, kReferringPhysicianTag);

    Study& study = patient->getOrCreateStudy(studyMetadata->studyInstanceUid);
    study.setStudyDescription(studyMetadata->studyDescription);
    study.setStudyDate(studyMetadata->studyDate);
    study.setDoctorName(studyMetadata->referringPhysicianName);

    auto seriesMetadata = std::make_shared<DicomSeriesMetadata>();
    seriesMetadata->study = studyMetadata;
    seriesMetadata->seriesInstanceUid = scannedTagValue(tagValues, kSeriesInstanceUidTag);
    if (seriesMetadata->seriesInstanceUid.isEmpty())
    {
        seriesMetadata->seriesInstanceUid = studyMetadata->studyInstanceUid + QStringLiteral("_SERIES");
    }
    seriesMetadata->seriesDescription = scannedTagValue(tagValues, kSeriesDescriptionTag);
    seriesMetadata->seriesNumber = scannedTagValue(tagValues, kSeriesNumberTag);
    seriesMetadata->modality = scannedTagValue(tagValues, kModalityTag);

    Series& series = study.getOrCreateSeries(seriesMetadata->seriesInstanceUid);
    series.setSeriesDescription(seriesMetadata->seriesDescription);
    series.setModality(seriesMetadata->modality);
    series.setSeriesNumber(seriesMetadata->seriesNumber);
    if (series.representativeFilePath().isEmpty())
    {
        series.setRepresentativeFilePath(filePath);
    }

    auto instanceMetadata = std::make_shared<DicomInstanceMetadata>();
    instanceMetadata->series = seriesMetadata;
    instanceMetadata->sopInstanceUid = scannedTagValue(tagValues, kSopInstanceUidTag);
    if (instanceMetadata->sopInstanceUid.isEmpty())
    {
        instanceMetadata->sopInstanceUid = QFileInfo(filePath).completeBaseName();
    }
    instanceMetadata->instanceNumber = scannedTagValue(tagValues, kInstanceNumberTag);
    instanceMetadata->numberOfFrames = std::max(1, scannedPositiveInt(tagValues, kNumberOfFramesTag, 1));
    instanceMetadata->rows = scannedPositiveInt(tagValues, kRowsTag);
    instanceMetadata->columns = scannedPositiveInt(tagValues, kColumnsTag);

    auto dicomImage = std::make_unique<DicomImage>();
    dicomImage->setFilePath(filePath);
    dicomImage->setMetadata(instanceMetadata);
    dicomImage->setSopInstanceUid(instanceMetadata->sopInstanceUid);
    dicomImage->setInstanceNumber(instanceMetadata->instanceNumber);
    dicomImage->setDimensions(instanceMetadata->columns, instanceMetadata->rows);
    dicomImage->setFrameCount(instanceMetadata->numberOfFrames);
    series.addImage(std::move(dicomImage));

    return patient;
}
