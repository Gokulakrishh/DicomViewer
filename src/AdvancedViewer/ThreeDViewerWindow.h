#pragma once

#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"

#include <QMainWindow>
#include <memory>

class QComboBox;
class I3dPipelineProfile;
class IVolumeData;
class QQuickWidget;
class ThreeDViewerController;
class QmlMeshGeometry;

/**
 * @brief Standalone 3D viewer window for reconstructed DICOM volume data.
 *
 * Responsibilities:
 * - Own the Qt/QML 3D viewing surface.
 * - Apply selected 3D pipeline profiles and rebuild mesh output.
 * - Synchronize generated mesh geometry into the QML scene.
 *
 * Assumptions:
 * - Input volume is already loaded from a single coherent DICOM series.
 * - 3D rendering is a visualization aid and depends on validated metadata and
 *   segmentation settings.
 */
class ThreeDViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Creates a 3D viewer window.
     * @param diagnosticVolume Volume data to render.
     * @param profileSelection Initial 3D reconstruction profile.
     * @param parent Optional Qt parent.
     */
    explicit ThreeDViewerWindow(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr);

    ~ThreeDViewerWindow() override;

    /**
     * @brief Applies a profile mode from QML/UI controls.
     * @param modeValue Integer value corresponding to ProfileMode.
     */
    Q_INVOKABLE void setProfileMode(int modeValue);

private:
    enum class ProfileMode
    {
        Auto,
        Bone,
        Lung
    };

    /** @brief Builds the window layout and QML scene bridge. */
    void setupUi();
    /** @brief Re-runs the selected 3D reconstruction pipeline. */
    void rebuildMesh();
    /** @brief Copies generated mesh buffers into the QML geometry object. */
    void syncGeometryFromController();
    /** @brief Selects and applies a concrete 3D profile mode. */
    void applyProfileMode(ProfileMode mode);

private:
    std::shared_ptr<IVolumeData> m_diagnosticVolume;
    std::shared_ptr<I3dPipelineProfile> m_profile;
    ThreeDProfileSelection m_autoProfileSelection;
    QQuickWidget* m_quickWidget{nullptr};
    ThreeDViewerController* m_viewerController{nullptr};
    std::unique_ptr<QmlMeshGeometry> m_meshGeometry;
};
