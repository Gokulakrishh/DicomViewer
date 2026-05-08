#pragma once

#include <QString>
#include <QStringList>

inline QStringList annotationBodyRegionOptions()
{
    return {
        "Other",
        "Brain",
        "Head/Neck",
        "Chest",
        "Lung",
        "Heart",
        "Liver",
        "Abdomen",
        "Pelvis",
        "Spine",
        "Upper Limb",
        "Lower Limb"};
}

inline QString annotationMeasurementTypeFilterValue(int index)
{
    switch (index)
    {
    case 1:
        return "distance";
    case 2:
        return "polyline";
    case 3:
        return "angle";
    case 4:
        return "rectangle_roi";
    default:
        return "all";
    }
}

