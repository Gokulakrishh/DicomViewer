#pragma once

#include "FileHandling.h"

#include <QHash>
#include <QList>
#include <QStringList>
#include <gdcmDirectory.h>
#include <gdcmImage.h>
#include <gdcmImageReader.h>
#include <gdcmFile.h>
#include <gdcmReader.h>
#include <gdcmScanner.h>
#include <gdcmStringFilter.h>

#include <map>
#include <vector>

/**
 * @brief GDCM-backed DICOM file handler.
 *
 * Responsibilities:
 * - Read DICOM metadata and pixel data through GDCM.
 * - Build lightweight patient/study/series hierarchy models.
 * - Normalize common DICOM fields used by the viewer and local database.
 *
 * Assumptions:
 * - Unsupported transfer syntaxes or malformed files should fail gracefully.
 * - The class does not persist data; it only loads and maps it.
 */
class GDCMFileHandling : public FileHandling
{
public:
    /**
     * @brief Creates a GDCM file handler with supported formats initialized.
     */
    GDCMFileHandling();

    /**
     * @brief Loads DICOM hierarchy from a folder.
     * @param folderPath Folder to scan recursively.
     * @param progressCallback Optional import progress callback.
     * @return Patient hierarchy list for discovered DICOM instances.
     */
    PatientList loadDicomFolder(const QString& folderPath, ProgressCallback progressCallback = {}) override;

    /**
     * @brief Loads a renderable DICOM image.
     * @param filePath Source DICOM file path.
     * @return Loaded medical image, or null on failure.
     */
    std::unique_ptr<MedicalImage> loadImage(const QString& filePath) override;

    /**
     * @brief Loads DICOM metadata and raw pixels for one file.
     * @param filePath Source DICOM file path.
     * @return Loaded DICOM image, or null on failure.
     */
    std::unique_ptr<DicomImage> loadImageData(const QString& filePath, int frameIndex = 0) const override;

    bool visitImageDataFrames(
        const QString& filePath,
        int firstFrameIndex,
        int lastFrameIndex,
        const ImageFrameVisitor& visitor,
        const CancellationCheck& isCancelled = {},
        QString* errorMessage = nullptr) const override;

    /**
     * @brief Loads multiple frames from one multi-frame DICOM using one decoded buffer.
     * @param filePath Source DICOM file path.
     * @param frameIndices Zero-based frame indices to load.
     * @return Loaded frames keyed by frame index.
     */
    QHash<int, std::shared_ptr<DicomImage>> loadImageDataFrames(
        const QString& filePath,
        const QList<int>& frameIndices) const;

    /**
     * @brief Loads hierarchy metadata for one DICOM file.
     * @param filePath Source DICOM file path.
     * @return Patient hierarchy containing the DICOM instance.
     */
    PatientPtr loadDicomHierarchy(const QString& filePath) override;

    /**
     * @brief Returns supported file formats.
     * @return Supported format labels/extensions.
     */
    QStringList getSupportedFormats() const override;

    /**
     * @brief Checks whether the file can be loaded as DICOM.
     * @param filePath File path to inspect.
     * @return True when the file is readable and has a DICOM Part-10 preamble.
     */
    bool canLoad(const QString& filePath) const override;

private:
    /** @brief Executes `gdcm::ImageReader::Read()` behind the class-level DICOM safety boundary. */
    static bool readGdcmImageReader(gdcm::ImageReader& reader, const QString& filePath);
    /** @brief Executes `gdcm::ImageReader::Read()` with C++ exception handling. */
    static bool readGdcmImageReaderImpl(gdcm::ImageReader& reader, const QString& filePath);
    /** @brief Executes `gdcm::Reader::Read()` behind the class-level DICOM safety boundary. */
    static bool readGdcmReader(gdcm::Reader& reader, const QString& filePath);
    /** @brief Executes `gdcm::Reader::Read()` with C++ exception handling. */
    static bool readGdcmReaderImpl(gdcm::Reader& reader, const QString& filePath);
    /** @brief Executes a bounded GDCM scanner batch behind the class-level DICOM safety boundary. */
    static bool scanGdcmMetadataBatch(
        gdcm::Scanner& scanner,
        const gdcm::Directory::FilenamesType& filenames,
        std::size_t batchStart,
        std::size_t batchEnd);
    /** @brief Executes a bounded GDCM scanner batch with C++ exception handling. */
    static bool scanGdcmMetadataBatchImpl(
        gdcm::Scanner& scanner,
        const gdcm::Directory::FilenamesType& filenames,
        std::size_t batchStart,
        std::size_t batchEnd);
    /** @brief Decodes a GDCM image buffer behind the class-level DICOM safety boundary. */
    static bool getGdcmImageBuffer(
        const gdcm::Image& gdcmImage,
        char* targetBuffer,
        const QString& filePath,
        const QString& transferSyntax);
    /** @brief Decodes a GDCM image buffer with C++ exception handling. */
    static bool getGdcmImageBufferImpl(
        const gdcm::Image& gdcmImage,
        char* targetBuffer,
        const QString& filePath,
        const QString& transferSyntax);
    /** @brief Collects regular files using explicit breadth-first traversal. */
    std::vector<QString> collectRegularFilesBfs(const QString& folderPath) const;
    /** @brief Filters files to DICOM Part-10 candidates. */
    std::vector<QString> filterDicomCandidates(const std::vector<QString>& files) const;
    /** @brief Checks the DICOM Part-10 preamble and magic bytes. */
    bool looksLikePart10Dicom(const QString& filePath) const;
    /** @brief Scans DICOM candidates in bounded metadata-only batches. */
    void scanDicomCandidatesInBatches(
        const std::vector<QString>& candidates,
        std::map<QString, PatientPtr>& patientMap,
        ProgressCallback progressCallback) const;
    /** @brief Merges one loaded patient hierarchy into an existing target. */
    void mergePatientHierarchy(const PatientPtr& sourcePatient, Patient& targetPatient) const;
    /** @brief Converts DICOM date strings into the application's display/storage form. */
    QString normalizeDicomDate(const QString& dicomDate) const;
    /** @brief Reads one DICOM string tag through GDCM. */
    QString readStringTag(const gdcm::StringFilter& stringFilter, uint16_t group, uint16_t element) const;
    /** @brief Decodes a DICOM pixel buffer with diagnostics for malformed compressed streams. */
    bool decodePixelBuffer(const QString& filePath, const gdcm::ImageReader& reader, QVector<char>& decodedBuffer) const;
    /** @brief Extracts standardized metadata without touching pixel data. */
    std::shared_ptr<DicomInstanceMetadata> extractDicomInstanceMetadata(const gdcm::File& file) const;
    /** @brief Extracts standardized metadata from an image reader after explicit pixel/image loading. */
    std::shared_ptr<DicomInstanceMetadata> extractDicomInstanceMetadata(const gdcm::ImageReader& reader) const;
    /** @brief Applies standardized metadata to a DicomImage container. */
    void applyMetadataToDicomImage(
        DicomImage& dicomImage,
        const std::shared_ptr<DicomInstanceMetadata>& metadata,
        int frameIndex) const;
    /** @brief Maps a GDCM image reader into a DicomImage object. */
    std::unique_ptr<DicomImage> loadDicomImage(
        const QString& filePath,
        const gdcm::ImageReader& reader,
        bool renderPixmap,
        int frameIndex = 0,
        const QVector<char>* decodedBuffer = nullptr) const;
    /** @brief Builds a metadata-only image entry for hierarchy import. */
    std::unique_ptr<DicomImage> createMetadataOnlyDicomImage(
        const QString& filePath,
        const gdcm::File& file) const;
    /** @brief Builds a metadata-only image entry from an image reader. */
    std::unique_ptr<DicomImage> createMetadataOnlyDicomImage(
        const QString& filePath,
        const gdcm::ImageReader& reader) const;
    /** @brief Builds patient/study/series metadata from one GDCM reader. */
    PatientPtr buildHierarchy(const QString& filePath, const gdcm::File& file) const;
    /** @brief Builds patient/study/series metadata from one GDCM image reader. */
    PatientPtr buildHierarchy(const QString& filePath, const gdcm::ImageReader& reader) const;
    /** @brief Builds patient/study/series metadata from scanner tag values. */
    PatientPtr buildHierarchyFromScannedTags(
        const QString& filePath,
        const gdcm::Scanner::TagToValue& tagValues) const;

private:
    QStringList m_supportedFormats;
};
