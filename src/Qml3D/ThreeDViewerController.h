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
    explicit ThreeDViewerController(QObject* parent = nullptr);
    ~ThreeDViewerController() override;

    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] bool meshAvailable() const;
    [[nodiscard]] QString errorText() const;
    [[nodiscard]] QString profileName() const;
    [[nodiscard]] QString anatomyLabel() const;
    [[nodiscard]] QColor surfaceColor() const;
    [[nodiscard]] int vertexCount() const;
    [[nodiscard]] int triangleCount() const;

    void setPipelineService(std::shared_ptr<ThreeDimensionalPipelineService> pipelineService);
    void setProfile(std::shared_ptr<I3dPipelineProfile> profile);
    void setVisualStyle(const ThreeDProfileVisualStyle& visualStyle);
    void rebuildFromVolume(const std::shared_ptr<IVolumeData>& volume);

    Q_INVOKABLE void clearMesh();

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
