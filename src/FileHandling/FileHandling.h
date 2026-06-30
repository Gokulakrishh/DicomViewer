#pragma once

#include "Model/DicomParameters.h"
#include "Model/MedicalImage.h"

#include <QList>
#include <memory>
#include <QStringList>
#include <functional>

/**
 * @brief Abstract DICOM file-loading boundary.
 *
 * Responsibilities:
 * - Load DICOM hierarchy metadata from files or folders.
 * - Load rendered images and raw DICOM image data on demand.
 * - Hide concrete parsing libraries from viewer and persistence code.
 *
 * Assumptions:
 * - Implementations should avoid preloading all pixel data for large series.
 * - Source DICOM files remain the canonical image source.
 */
class FileHandling
{
public:
    using PatientPtr = std::shared_ptr<Patient>;
    using PatientList = QList<PatientPtr>;
    using ProgressCallback = std::function<void(int current, int total)>;
    using ImageFrameVisitor = std::function<bool(
        int frameIndex,
        const DicomImage& image,
        QString* errorMessage)>;
    using CancellationCheck = std::function<bool()>;

    virtual ~FileHandling() = default;

    /**
     * @brief Loads all importable DICOM files from a folder.
     * @param folderPath Folder to scan.
     * @param progressCallback Optional progress callback with current/total counts.
     * @return Patient hierarchy list built from discovered files.
     */
    virtual PatientList loadDicomFolder(const QString& folderPath, ProgressCallback progressCallback = {}) = 0;

    /**
     * @brief Loads a renderable medical image.
     * @param filePath Source DICOM file path.
     * @return Loaded image, or null when unsupported/unreadable.
     */
    virtual std::unique_ptr<MedicalImage> loadImage(const QString& filePath) = 0;

    /**
     * @brief Loads DICOM metadata and raw pixel data for one image/frame.
     * @param filePath Source DICOM file path.
     * @param frameIndex Zero-based frame index for multi-frame instances.
     * @return Loaded DicomImage, or null when unsupported/unreadable.
     */
    virtual std::unique_ptr<DicomImage> loadImageData(const QString& filePath, int frameIndex = 0) const = 0;

    /**
     * @brief Decodes one multi-frame DICOM once and visits a selected frame range.
     * @param filePath Source multi-frame DICOM path.
     * @param firstFrameIndex First zero-based frame to visit.
     * @param lastFrameIndex Last zero-based frame to visit.
     * @param visitor Consumer called for each temporary decoded frame.
     * @param isCancelled Optional cancellation callback.
     * @param errorMessage Receives a recoverable failure description.
     * @return True when every requested frame was visited successfully.
     */
    virtual bool visitImageDataFrames(
        const QString& filePath,
        int firstFrameIndex,
        int lastFrameIndex,
        const ImageFrameVisitor& visitor,
        const CancellationCheck& isCancelled = {},
        QString* errorMessage = nullptr) const = 0;

    /**
     * @brief Loads patient/study/series hierarchy for one DICOM file.
     * @param filePath Source DICOM file path.
     * @return Patient hierarchy containing the file metadata.
     */
    virtual PatientPtr loadDicomHierarchy(const QString& filePath) = 0;

    /**
     * @brief Returns supported file extensions/formats.
     * @return Supported format labels.
     */
    virtual QStringList getSupportedFormats() const = 0;

    /**
     * @brief Checks whether a file can be loaded by this handler.
     * @param filePath File path to inspect.
     * @return True when the handler supports the file.
     */
    virtual bool canLoad(const QString& filePath) const = 0;
};
