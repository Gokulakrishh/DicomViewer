#pragma once

/**
 * @brief Orthogonal slice planes used by MPR.
 */
enum class MprSlicePlane
{
    Axial,
    Coronal,
    Sagittal
};

/**
 * @brief Active interaction tool in the MPR viewer.
 */
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

/**
 * @brief Crosshair/cursor position in volume world coordinates.
 */
struct MprCursorPositionWorld
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};
