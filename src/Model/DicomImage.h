#pragma once

#include "MedicalImage.h"

#include <array>
#include <cstdint>
#include <vector>

class DicomImage : public MedicalImage
{
public:
    DicomImage() = default;

    const QPixmap& pixmap() const override;
    bool isValid() const override;

    const QString& filePath() const;
    const QString& sopInstanceUid() const;
    const QString& instanceNumber() const;
    int width() const;
    int height() const;
    bool hasRawPixels() const;
    bool isMonochrome() const;
    bool isMonochrome1() const;
    int minimumStoredValue() const;
    int maximumStoredValue() const;
    int defaultWindowLevel() const;
    int defaultWindowWidth() const;
    bool hasPixelSpacing() const;
    double pixelSpacingX() const;
    double pixelSpacingY() const;
    bool hasImagePositionPatient() const;
    bool hasImageOrientationPatient() const;
    const std::array<double, 3>& imagePositionPatient() const;
    const std::array<double, 6>& imageOrientationPatient() const;
    double sliceThickness() const;
    double spacingBetweenSlices() const;
    int rawPixelValueAt(int x, int y) const;
    std::size_t rawPixelByteCount() const;

    void setPixmap(const QPixmap& pixmap);
    void setFilePath(const QString& filePath);
    void setSopInstanceUid(const QString& sopInstanceUid);
    void setInstanceNumber(const QString& instanceNumber);
    void setDimensions(int width, int height);
    void setRawPixels(const std::vector<int16_t>& rawPixels);
    void clearRawPixels();
    void setMonochrome(bool isMonochrome);
    void setMonochrome1(bool isMonochrome1);
    void setValueRange(int minimumStoredValue, int maximumStoredValue);
    void setDefaultWindow(int windowLevel, int windowWidth);
    void setPixelSpacing(double pixelSpacingX, double pixelSpacingY);
    void setImagePositionPatient(const std::array<double, 3>& imagePositionPatient);
    void setImageOrientationPatient(const std::array<double, 6>& imageOrientationPatient);
    void setSliceThickness(double sliceThickness);
    void setSpacingBetweenSlices(double spacingBetweenSlices);
private:
    QPixmap m_pixmap;
    QString m_filePath;
    QString m_sopInstanceUid;
    QString m_instanceNumber;
    int m_width{0};
    int m_height{0};
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
