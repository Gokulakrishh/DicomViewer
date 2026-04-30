#pragma once

#include <QString>
#include <array>
#include <memory>
#include <vector>

struct DicomWindowPreset
{
    double center{0.0};
    double width{0.0};
    QString explanation;
};

struct DicomPatientMetadata
{
    QString patientId;
    QString patientName;
    QString patientSex;
    QString patientBirthDate;
};

struct DicomStudyMetadata
{
    std::shared_ptr<const DicomPatientMetadata> patient;
    QString studyInstanceUid;
    QString patientPosition;

    QString studyDate;
    QString studyTime;
    QString studyDescription;
    QString referringPhysicianName;
};

struct DicomSeriesMetadata
{
    std::shared_ptr<const DicomStudyMetadata> study;
    QString seriesInstanceUid;
    QString frameOfReferenceUid;
    QString seriesDate;
    QString seriesTime;
    QString seriesDescription;
    QString seriesNumber;
    QString modality;
    QString bodyPartExamined;
    QString protocolName;

    QString manufacturer;
    QString manufacturerModelName;
    QString institutionName;
    QString stationName;
};

struct DicomInstanceMetadata
{
    std::shared_ptr<const DicomSeriesMetadata> series;
    QString sopClassUid;
    QString sopInstanceUid;
    QString instanceNumber;
    QString imageType;
    QString acquisitionDate;
    QString acquisitionTime;
    QString contentDate;
    QString contentTime;

    int rows{0};
    int columns{0};
    int samplesPerPixel{0};
    int bitsAllocated{0};
    int bitsStored{0};
    int highBit{0};
    int pixelRepresentation{0};
    QString photometricInterpretation;

    bool hasPixelSpacing{false};
    double pixelSpacingX{0.0};
    double pixelSpacingY{0.0};

    bool hasImagePositionPatient{false};
    std::array<double, 3> imagePositionPatient{0.0, 0.0, 0.0};

    bool hasImageOrientationPatient{false};
    std::array<double, 6> imageOrientationPatient{1.0, 0.0, 0.0, 0.0, 1.0, 0.0};

    bool hasSliceThickness{false};
    double sliceThickness{0.0};

    bool hasSpacingBetweenSlices{false};
    double spacingBetweenSlices{0.0};

    bool hasSliceLocation{false};
    double sliceLocation{0.0};

    bool hasGantryDetectorTilt{false};
    double gantryDetectorTilt{0.0};

    bool hasRescaleSlope{false};
    double rescaleSlope{1.0};

    bool hasRescaleIntercept{false};
    double rescaleIntercept{0.0};

    QString rescaleType;
    QString voiLutFunction;
    std::vector<DicomWindowPreset> windowPresets;
};
