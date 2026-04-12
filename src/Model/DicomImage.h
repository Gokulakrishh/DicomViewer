#pragma once

#include "MedicalImage.h"

#include <QImage>
#include <QPointF>
#include <QVector>

class DicomImage : public MedicalImage
{
public:
    DicomImage() = default;

    const QPixmap& pixmap() const override { return m_pixmap; }
    bool isValid() const override { return !m_pixmap.isNull() && m_width > 0 && m_height > 0; }

    const QString& filePath() const { return m_filePath; }
    const QString& sopInstanceUid() const { return m_sopInstanceUid; }
    const QString& instanceNumber() const { return m_instanceNumber; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool hasRawPixels() const { return !m_rawPixels.isEmpty() && m_width > 0 && m_height > 0; }
    bool isMonochrome() const { return m_isMonochrome; }
    bool isMonochrome1() const { return m_isMonochrome1; }
    int minimumStoredValue() const { return m_minimumStoredValue; }
    int maximumStoredValue() const { return m_maximumStoredValue; }
    int defaultWindowLevel() const { return m_defaultWindowLevel; }
    int defaultWindowWidth() const { return m_defaultWindowWidth; }
    bool hasPixelSpacing() const { return m_pixelSpacingX > 0.0 && m_pixelSpacingY > 0.0; }
    double pixelSpacingX() const { return m_pixelSpacingX; }
    double pixelSpacingY() const { return m_pixelSpacingY; }
    int rawPixelValueAt(int x, int y) const
    {
        if (!hasRawPixels() || x < 0 || y < 0 || x >= m_width || y >= m_height)
        {
            return 0;
        }

        return m_rawPixels[(y * m_width) + x];
    }

    void setPixmap(const QPixmap& pixmap) { m_pixmap = pixmap; }
    void setFilePath(const QString& filePath) { m_filePath = filePath; }
    void setSopInstanceUid(const QString& sopInstanceUid) { m_sopInstanceUid = sopInstanceUid; }
    void setInstanceNumber(const QString& instanceNumber) { m_instanceNumber = instanceNumber; }
    void setDimensions(int width, int height)
    {
        m_width = width;
        m_height = height;
    }
    void setRawPixels(const QVector<int>& rawPixels)
    {
        m_rawPixels = rawPixels;
    }
    void setMonochrome(bool isMonochrome)
    {
        m_isMonochrome = isMonochrome;
    }
    void setMonochrome1(bool isMonochrome1)
    {
        m_isMonochrome1 = isMonochrome1;
    }
    void setValueRange(int minimumStoredValue, int maximumStoredValue)
    {
        m_minimumStoredValue = minimumStoredValue;
        m_maximumStoredValue = maximumStoredValue;
    }
    void setDefaultWindow(int windowLevel, int windowWidth)
    {
        m_defaultWindowLevel = windowLevel;
        m_defaultWindowWidth = windowWidth;
    }
    void setPixelSpacing(double pixelSpacingX, double pixelSpacingY)
    {
        m_pixelSpacingX = pixelSpacingX;
        m_pixelSpacingY = pixelSpacingY;
    }
private:
    QPixmap m_pixmap;
    QString m_filePath;
    QString m_sopInstanceUid;
    QString m_instanceNumber;
    int m_width{0};
    int m_height{0};
    QVector<int> m_rawPixels;
    bool m_isMonochrome{false};
    bool m_isMonochrome1{false};
    int m_minimumStoredValue{0};
    int m_maximumStoredValue{255};
    int m_defaultWindowLevel{0};
    int m_defaultWindowWidth{255};
    double m_pixelSpacingX{0.0};
    double m_pixelSpacingY{0.0};
};
