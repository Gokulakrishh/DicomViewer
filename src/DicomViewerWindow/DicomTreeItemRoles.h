#pragma once

#include <QtCore/Qt>

namespace DicomTreeItemRoles
{
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
