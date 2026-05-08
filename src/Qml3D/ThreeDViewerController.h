#pragma once

#include "Qml3D/MeshSceneAdapter.h"
#include "Services/ThreeDimensionalPipelineResult.h"

#include <QColor>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <memory>

class I3dPipelineProfile;
class IVolumeData;
class ThreeDimensionalPipelineService;
struct ThreeDProfileVisualStyle;

/**
 * @brief Qt/QML controller for asynchronous 3D mesh reconstruction and display state.
 *
 * Responsibilities:
 * - Run the 3D pipeline off the UI thread.
 * - Expose mesh availability, visual style, and diagnostics to QML.
 * - Maintain generation checks so stale async results do not replace newer ones.
 */
class ThreeDViewerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(bool meshAvailable READ meshAvailable NOTIFY meshAvailableChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(QString profileName READ profileName NOTIFY profileChanged)
    Q_PROPERTY(QString anatomyLabel READ anatomyLabel NOTIFY visualStyleChanged)
    Q_PROPERTY(QColor surfaceColor READ surfaceColor NOTIFY visualStyleChanged)
    Q_PROPERTY(int vertexCount READ vertexCount NOTIFY meshStatsChanged)
    Q_PROPERTY(int triangleCount READ triangleCount NOTIFY meshStatsChanged)

public:
    /** @brief Creates the 3D viewer controller. */
    explicit ThreeDViewerController(QObject* parent = nullptr);
    ~ThreeDViewerController() override;

    /** @brief Reports whether a rebuild is running. */
    [[nodiscard]] bool isBusy() const;
    /** @brief Reports whether mesh data is available. */
    [[nodiscard]] bool meshAvailable() const;
    /** @brief Returns the current error text. */
    [[nodiscard]] QString errorText() const;
    /** @brief Returns the active profile name. */
    [[nodiscard]] QString profileName() const;
    /** @brief Returns the anatomy label used by the UI. */
    [[nodiscard]] QString anatomyLabel() const;
    /** @brief Returns the surface color used by the QML material. */
    [[nodiscard]] QColor surfaceColor() const;
    /** @brief Returns current mesh vertex count. */
    [[nodiscard]] int vertexCount() const;
    /** @brief Returns current mesh triangle count. */
    [[nodiscard]] int triangleCount() const;

    /** @brief Sets the pipeline service used for reconstruction. */
    void setPipelineService(std::shared_ptr<ThreeDimensionalPipelineService> pipelineService);
    /** @brief Sets the active 3D pipeline profile. */
    void setProfile(std::shared_ptr<I3dPipelineProfile> profile);
    /** @brief Sets visual style metadata for the active profile. */
    void setVisualStyle(const ThreeDProfileVisualStyle& visualStyle);
    /** @brief Starts asynchronous rebuild from a diagnostic volume. */
    void rebuildFromVolume(const std::shared_ptr<IVolumeData>& volume);

    /** @brief Clears current mesh data from the scene. */
    Q_INVOKABLE void clearMesh();

    /** @brief Returns the current scene adapter buffers. */
    [[nodiscard]] const MeshSceneAdapter& sceneAdapter() const;

signals:
    void isBusyChanged();
    void meshAvailableChanged();
    void errorTextChanged();
    void profileChanged();
    void visualStyleChanged();
    void meshStatsChanged();

private:
    void setBusy(bool busy);
    void setErrorText(QString errorText);
    void emitMeshStateChanged();
    void handleRebuildFinished();

private:
    std::shared_ptr<ThreeDimensionalPipelineService> m_pipelineService;
    std::shared_ptr<I3dPipelineProfile> m_profile;
    std::shared_ptr<IVolumeData> m_currentVolume;
    MeshSceneAdapter m_sceneAdapter;
    QString m_anatomyLabel;
    QColor m_surfaceColor;
    QString m_errorText;
    QFutureWatcher<ThreeDimensionalPipelineResult>* m_rebuildWatcher{nullptr};
    int m_rebuildGeneration{0};
    bool m_isBusy{false};
};
