#pragma once

#include <QString>
#include <array>
#include <memory>
#include <vector>

/**
 * @brief DICOM VOI window preset extracted from an image instance.
 *
 * Presets are stored separately from built-in viewer presets so DICOM-provided
 * WL/WW options can be presented consistently across main and MPR views.
 */
struct DicomWindowPreset
{
    double center{0.0};
    double width{0.0};
    QString explanation;
};

/**
 * @brief Patient-level DICOM metadata shared by studies.
 *
 * The structure is reference-shared by lower hierarchy levels to avoid copying
 * duplicate patient text into every image instance.
 */
struct DicomPatientMetadata
{
    QString patientId;
    QString patientName;
    QString patientSex;
    QString patientBirthDate;
};

/**
 * @brief Study-level DICOM metadata shared by series.
 *
 * Responsibilities:
 * - Hold study identifiers and acquisition context.
 * - Reference patient metadata without duplicating it.
 */
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

/**
 * @brief Series-level DICOM metadata shared by instances.
 *
 * Responsibilities:
 * - Hold series identifiers, modality, protocol, and equipment details.
 * - Reference study metadata without duplicating it for every slice.
 */
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

/**
 * @brief SOP instance-level DICOM metadata needed by viewer workflows.
 *
 * Responsibilities:
 * - Store image geometry, pixel calibration, rescale, and window presets.
 * - Link to series/study/patient metadata through shared immutable references.
 *
 * Assumptions:
 * - This structure stores standardized metadata used by the application, not a
 *   complete copy of every DICOM tag.
 */
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
    int numberOfFrames{1};
    double frameTimeMs{0.0};
    double cineRateFps{0.0};
    double frameIntervalMs{100.0};
    std::vector<double> frameTimeVectorMs;
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
