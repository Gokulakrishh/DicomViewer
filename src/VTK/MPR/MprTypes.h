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
 * @brief Orthogonal MPR slab projection modes.
 */
enum class MprSlabMode
{
    Thin,
    MaximumIntensity,
    MinimumIntensity,
    Average
};

/**
 * @brief Shared slab projection settings for orthogonal MPR panes.
 */
struct MprSlabSettings
{
    MprSlabMode mode{MprSlabMode::Thin};
    double thicknessMm{1.0};
};

/**
 * @brief Controlled free-oblique MPR settings for the first Phase C increment.
 *
 * The initial implementation keeps oblique reformatting explicit and bounded:
 * one selected pane may be tilted by a numeric angle while measurement and
 * annotation semantics remain gated until arbitrary-plane verification exists.
 */
struct MprObliqueSettings
{
    bool enabled{false};
    MprSlicePlane basePlane{MprSlicePlane::Axial};
    double angleDegrees{0.0};
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
