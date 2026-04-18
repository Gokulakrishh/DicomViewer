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

class ThreeDViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ThreeDViewerWindow(
        std::shared_ptr<IVolumeData> diagnosticVolume,
        ThreeDProfileSelection profileSelection,
        QWidget* parent = nullptr);
    ~ThreeDViewerWindow() override;

    Q_INVOKABLE void setProfileMode(int modeValue);

private:
    enum class ProfileMode
    {
        Auto,
        Bone,
        Lung
    };

    void setupUi();
    void rebuildMesh();
    void syncGeometryFromController();
    void applyProfileMode(ProfileMode mode);

private:
    std::shared_ptr<IVolumeData> m_diagnosticVolume;
    std::shared_ptr<I3dPipelineProfile> m_profile;
    ThreeDProfileSelection m_autoProfileSelection;
    QQuickWidget* m_quickWidget{nullptr};
    ThreeDViewerController* m_viewerController{nullptr};
    std::unique_ptr<QmlMeshGeometry> m_meshGeometry;
};
