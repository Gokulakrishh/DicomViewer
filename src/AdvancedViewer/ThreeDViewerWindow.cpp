#include "AdvancedViewer/ThreeDViewerWindow.h"

#include "Qml3D/QmlMeshGeometry.h"
#include "Qml3D/ThreeDViewerController.h"
#include "Services/ThreeDProfiles/Bone3dPipelineProfile.h"
#include "Services/ThreeDProfiles/Lung3dPipelineProfile.h"
#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "Services/ThreeDimensionalPipelineService.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QColor>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

ThreeDViewerWindow::ThreeDViewerWindow(
    std::shared_ptr<IVolumeData> diagnosticVolume,
    ThreeDProfileSelection profileSelection,
    QWidget* parent)
    : QMainWindow(parent),
      m_diagnosticVolume(std::move(diagnosticVolume)),
      m_profile(profileSelection.pipelineProfile),
      m_autoProfileSelection(std::move(profileSelection)),
      m_viewerController(new ThreeDViewerController(this)),
      m_meshGeometry(std::make_unique<QmlMeshGeometry>())
{
    setupUi();
    m_viewerController->setVisualStyle(m_autoProfileSelection.visualStyle);
    QTimer::singleShot(0, this, &ThreeDViewerWindow::rebuildMesh);
}

ThreeDViewerWindow::~ThreeDViewerWindow() = default;

void ThreeDViewerWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_quickWidget = new QQuickWidget(centralWidget);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setClearColor(QColor("#10151c"));
    m_quickWidget->rootContext()->setContextProperty("viewerController", m_viewerController);
    m_quickWidget->rootContext()->setContextProperty("meshGeometry", m_meshGeometry.get());
    m_quickWidget->rootContext()->setContextProperty("threeDWindow", this);
    m_quickWidget->setSource(QUrl(QStringLiteral("qrc:/qml/ThreeDViewer.qml")));
    layout->addWidget(m_quickWidget);

    setCentralWidget(centralWidget);
    resize(1200, 820);

    m_viewerController->setPipelineService(std::make_shared<ThreeDimensionalPipelineService>());
    m_viewerController->setProfile(m_profile);

    connect(m_viewerController, &ThreeDViewerController::meshAvailableChanged, this, &ThreeDViewerWindow::syncGeometryFromController);
    connect(m_viewerController, &ThreeDViewerController::meshStatsChanged, this, &ThreeDViewerWindow::syncGeometryFromController);
}

void ThreeDViewerWindow::rebuildMesh()
{
    m_viewerController->rebuildFromVolume(m_diagnosticVolume);
    syncGeometryFromController();
}

void ThreeDViewerWindow::syncGeometryFromController()
{
    if (!m_viewerController->meshAvailable())
    {
        m_meshGeometry->clearMesh();
        return;
    }

    m_meshGeometry->setMesh(m_viewerController->sceneAdapter());
}

void ThreeDViewerWindow::applyProfileMode(ProfileMode mode)
{
    switch (mode)
    {
    case ProfileMode::Auto:
        m_profile = m_autoProfileSelection.pipelineProfile;
        m_viewerController->setProfile(m_profile);
        m_viewerController->setVisualStyle(m_autoProfileSelection.visualStyle);
        break;
    case ProfileMode::Bone:
    {
        ThreeDProfileSelection selection;
        selection.pipelineProfile = std::make_shared<Bone3dPipelineProfile>();
        selection.visualStyle = ThreeDProfileSelector::visualStyleForKind(ThreeDAnatomyKind::Bone);
        m_profile = selection.pipelineProfile;
        m_viewerController->setProfile(m_profile);
        m_viewerController->setVisualStyle(selection.visualStyle);
        break;
    }
    case ProfileMode::Lung:
    {
        ThreeDProfileSelection selection;
        selection.pipelineProfile = std::make_shared<Lung3dPipelineProfile>();
        selection.visualStyle = ThreeDProfileSelector::visualStyleForKind(ThreeDAnatomyKind::Lung);
        m_profile = selection.pipelineProfile;
        m_viewerController->setProfile(m_profile);
        m_viewerController->setVisualStyle(selection.visualStyle);
        break;
    }
    }

    rebuildMesh();
}

void ThreeDViewerWindow::setProfileMode(int modeValue)
{
    applyProfileMode(static_cast<ProfileMode>(modeValue));
}
