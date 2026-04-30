#pragma once

enum class MprSlicePlane
{
    Axial,
    Coronal,
    Sagittal
};

enum class MprToolType
{
    None,
    Crosshair,
    WindowLevel,
    Zoom,
    Pan,
    DistanceMeasurement,
    PolylineMeasurement,
    AngleMeasurement,
    RectangleRoiMeasurement,
    Slice
};

struct MprCursorPositionWorld
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};
