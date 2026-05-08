#pragma once

#include "FileHandling.h"

#include <QHash>
#include <QList>
#include <QStringList>
#include <gdcmImageReader.h>
#include <gdcmStringFilter.h>

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
     * @return True when GDCM can read the file.
     */
    bool canLoad(const QString& filePath) const override;

private:
    /** @brief Merges one loaded patient hierarchy into an existing target. */
    void mergePatientHierarchy(const PatientPtr& sourcePatient, Patient& targetPatient) const;
    /** @brief Converts DICOM date strings into the application's display/storage form. */
    QString normalizeDicomDate(const QString& dicomDate) const;
    /** @brief Reads one DICOM string tag through GDCM. */
    QString readStringTag(const gdcm::StringFilter& stringFilter, uint16_t group, uint16_t element) const;
    /** @brief Maps a GDCM image reader into a DicomImage object. */
    std::unique_ptr<DicomImage> loadDicomImage(
        const QString& filePath,
        const gdcm::ImageReader& reader,
        bool renderPixmap,
        int frameIndex = 0,
        const QVector<char>* decodedBuffer = nullptr) const;
    /** @brief Builds patient/study/series metadata from one GDCM reader. */
    PatientPtr buildHierarchy(const QString& filePath, const gdcm::ImageReader& reader) const;

private:
    QStringList m_supportedFormats;
};
