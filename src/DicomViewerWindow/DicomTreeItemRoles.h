#pragma once

#include <QtCore/Qt>

namespace DicomTreeItemRoles
{
/**
 * @brief Custom Qt item roles used by the DICOM study-browser model.
 *
 * Roles store stable DICOM identifiers and searchable metadata in tree rows
 * without exposing model-column text parsing to controller code.
 */
enum Value
{
    FilePath = Qt::UserRole + 1,
    SeriesInstanceUid,
    PatientName,
    PatientDob,
    DoctorName,
    Modality,
    StudyDate,
    SearchText,
    PatientId,
    StudyInstanceUid,
    NodeType,
    ChildrenLoaded
};

inline constexpr const char* NodeTypePatient = "patient";
inline constexpr const char* NodeTypeStudy = "study";
inline constexpr const char* NodeTypeSeries = "series";
}
