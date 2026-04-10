#pragma once

#include "MedicalImage.h"

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

    void setPixmap(const QPixmap& pixmap) { m_pixmap = pixmap; }
    void setFilePath(const QString& filePath) { m_filePath = filePath; }
    void setSopInstanceUid(const QString& sopInstanceUid) { m_sopInstanceUid = sopInstanceUid; }
    void setInstanceNumber(const QString& instanceNumber) { m_instanceNumber = instanceNumber; }
    void setDimensions(int width, int height)
    {
        m_width = width;
        m_height = height;
    }

private:
    QPixmap m_pixmap;
    QString m_filePath;
    QString m_sopInstanceUid;
    QString m_instanceNumber;
    int m_width{0};
    int m_height{0};
};
