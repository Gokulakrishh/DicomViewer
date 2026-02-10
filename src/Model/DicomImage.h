#pragma once

#include "MedicalImage.h"

class DicomImage: public MedicalImage
{
    
public:
    public:
    // Constructor
    DicomImage() : m_width(0), m_height(0) {}

    
    const QPixmap& pixmap() const override { return m_pixmap; }
    bool isValid() const override { return !m_pixmap.isNull() && m_width > 0 && m_height > 0; }

    // DICOM-specific getters
    const QString& filePath() const { return m_filePath; }
    const QString& patientId() const { return m_patientId; }
    const QString& studyDate() const { return m_studyDate; }
    const QString& modality() const { return m_modality; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    // Setters
    void setPixmap(const QPixmap& pixmap) { m_pixmap = pixmap; }
    void setFilePath(const QString& path) { m_filePath = path; }
    void setPatientId(const QString& id) { m_patientId = id; }
    void setStudyDate(const QString& date) { m_studyDate = date; }
    void setModality(const QString& mod) { m_modality = mod; }
    void setDimensions(int w, int h) { m_width = w; m_height = h; }

    
private:
    QPixmap m_pixmap;
    QString m_filePath;
    QString m_patientId;
    QString m_studyDate;
    QString m_modality;
    int m_width{0}, m_height{0};
};


