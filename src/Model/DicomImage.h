#pragma once

#include "DicomMetadata.h"
#include "MedicalImage.h"

#include <array>
#include <cstdint>
#include <vector>

/**
 * @brief DICOM image instance metadata, rendered pixmap, and optional raw pixels.
 *
 * Responsibilities:
 * - Store SOP-level identifiers and geometry metadata needed by tools.
 * - Carry rendered display pixmap and raw pixel data when loaded.
 * - Support WL/WW rendering, measurement calibration, MPR, and 3D preparation.
 *
 * Assumptions:
 * - Raw pixels may be cleared for memory control and reloaded from file on
 *   demand.
 * - File path remains the link to the canonical source DICOM instance.
 */
class DicomImage : public MedicalImage
{
public:
    DicomImage() = default;

    /**
     * @brief Returns the rendered display pixmap.
     * @return Current pixmap.
     */
    const QPixmap& pixmap() const override;

    /**
     * @brief Reports whether the image has displayable data.
     * @return True when the pixmap or image data is valid.
     */
    bool isValid() const override;

    /** @brief Returns the source DICOM file path. */
    const QString& filePath() const;
    /** @brief Returns shared immutable DICOM metadata. */
    std::shared_ptr<const DicomInstanceMetadata> metadata() const;
    /** @brief Returns the DICOM SOP Instance UID. */
    const QString& sopInstanceUid() const;
    /** @brief Returns the DICOM Instance Number. */
    const QString& instanceNumber() const;
    /** @brief Returns image width in pixels. */
    int width() const;
    /** @brief Returns image height in pixels. */
    int height() const;
    /** @brief Returns DICOM Number of Frames, defaulting to one for single-frame instances. */
    int frameCount() const;
    /** @brief Returns zero-based frame index within a multi-frame DICOM instance. */
    int frameIndex() const;
    /** @brief Returns playback interval for this frame in milliseconds. */
    double cineFrameIntervalMs() const;
    /** @brief Returns DICOM Frame Time in milliseconds when present. */
    double frameTimeMs() const;
    /** @brief Returns DICOM Cine Rate in frames per second when present. */
    double cineRateFps() const;
    /** @brief Reports whether raw pixels are currently loaded. */
    bool hasRawPixels() const;
    /** @brief Reports whether the image is monochrome. */
    bool isMonochrome() const;
    /** @brief Reports whether MONOCHROME1 inversion is needed. */
    bool isMonochrome1() const;
    /** @brief Returns minimum stored pixel value. */
    int minimumStoredValue() const;
    /** @brief Returns maximum stored pixel value. */
    int maximumStoredValue() const;
    /** @brief Returns default DICOM window level. */
    int defaultWindowLevel() const;
    /** @brief Returns default DICOM window width. */
    int defaultWindowWidth() const;
    /** @brief Reports whether pixel spacing is available. */
    bool hasPixelSpacing() const;
    /** @brief Returns row/column pixel spacing in X direction. */
    double pixelSpacingX() const;
    /** @brief Returns row/column pixel spacing in Y direction. */
    double pixelSpacingY() const;
    /** @brief Reports whether Image Position Patient is available. */
    bool hasImagePositionPatient() const;
    /** @brief Reports whether Image Orientation Patient is available. */
    bool hasImageOrientationPatient() const;
    /** @brief Returns Image Position Patient. */
    const std::array<double, 3>& imagePositionPatient() const;
    /** @brief Returns Image Orientation Patient. */
    const std::array<double, 6>& imageOrientationPatient() const;
    /** @brief Returns DICOM Slice Thickness. */
    double sliceThickness() const;
    /** @brief Returns DICOM Spacing Between Slices. */
    double spacingBetweenSlices() const;
    /**
     * @brief Reads one raw pixel value.
     * @param x Pixel x coordinate.
     * @param y Pixel y coordinate.
     * @return Stored pixel value, or implementation-defined fallback out of range.
     */
    int rawPixelValueAt(int x, int y) const;
    /**
     * @brief Returns memory used by loaded raw pixels.
     * @return Raw pixel byte count.
     */
    std::size_t rawPixelByteCount() const;

    /** @brief Sets the rendered display pixmap. */
    void setPixmap(const QPixmap& pixmap);
    /** @brief Sets the source DICOM file path. */
    void setFilePath(const QString& filePath);
    /** @brief Sets mutable shared DICOM metadata. */
    void setMetadata(const std::shared_ptr<DicomInstanceMetadata>& metadata);
    /** @brief Copies immutable shared DICOM metadata into this image. */
    void setMetadata(const std::shared_ptr<const DicomInstanceMetadata>& metadata);
    /** @brief Copies DICOM metadata into this image. */
    void setMetadata(const DicomInstanceMetadata& metadata);
    /** @brief Sets the DICOM SOP Instance UID. */
    void setSopInstanceUid(const QString& sopInstanceUid);
    /** @brief Sets the DICOM Instance Number. */
    void setInstanceNumber(const QString& instanceNumber);
    /** @brief Sets image dimensions in pixels. */
    void setDimensions(int width, int height);
    /** @brief Sets DICOM Number of Frames for multi-frame instances. */
    void setFrameCount(int frameCount);
    /** @brief Sets zero-based frame index within a multi-frame DICOM instance. */
    void setFrameIndex(int frameIndex);
    /** @brief Sets DICOM cine timing metadata. */
    void setCineTiming(double frameTimeMs, double cineRateFps, double frameIntervalMs);
    /** @brief Stores raw pixel values for measurement/rendering workflows. */
    void setRawPixels(const std::vector<int16_t>& rawPixels);
    /** @brief Releases loaded raw pixels to reduce memory usage. */
    void clearRawPixels();
    /** @brief Sets whether the image should be treated as monochrome. */
    void setMonochrome(bool isMonochrome);
    /** @brief Sets whether MONOCHROME1 inversion is required. */
    void setMonochrome1(bool isMonochrome1);
    /** @brief Sets stored pixel value range. */
    void setValueRange(int minimumStoredValue, int maximumStoredValue);
    /** @brief Sets default DICOM WL/WW values. */
    void setDefaultWindow(int windowLevel, int windowWidth);
    /** @brief Sets calibrated pixel spacing in millimeters. */
    void setPixelSpacing(double pixelSpacingX, double pixelSpacingY);
    /** @brief Sets DICOM Image Position Patient. */
    void setImagePositionPatient(const std::array<double, 3>& imagePositionPatient);
    /** @brief Sets DICOM Image Orientation Patient. */
    void setImageOrientationPatient(const std::array<double, 6>& imageOrientationPatient);
    /** @brief Sets DICOM Slice Thickness. */
    void setSliceThickness(double sliceThickness);
    /** @brief Sets DICOM Spacing Between Slices. */
    void setSpacingBetweenSlices(double spacingBetweenSlices);
private:
    DicomInstanceMetadata& mutableMetadata();

private:
    QPixmap m_pixmap;
    QString m_filePath;
    std::shared_ptr<DicomInstanceMetadata> m_metadata;
    int m_width{0};
    int m_height{0};
    int m_frameCount{1};
    int m_frameIndex{0};
    std::vector<int16_t> m_rawPixels;
    bool m_isMonochrome{false};
    bool m_isMonochrome1{false};
    int m_minimumStoredValue{0};
    int m_maximumStoredValue{255};
    int m_defaultWindowLevel{0};
    int m_defaultWindowWidth{255};
    double m_pixelSpacingX{0.0};
    double m_pixelSpacingY{0.0};
    std::array<double, 3> m_imagePositionPatient{0.0, 0.0, 0.0};
    std::array<double, 6> m_imageOrientationPatient{1.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    bool m_hasImagePositionPatient{false};
    bool m_hasImageOrientationPatient{false};
    double m_sliceThickness{0.0};
    double m_spacingBetweenSlices{0.0};
};
