#include "Qml3D/ThreeDViewerController.h"

#include "Model/IMeshData.h"
#include "Services/ThreeDProfiles/I3dPipelineProfile.h"
#include "Services/ThreeDProfiles/ThreeDProfileSelection.h"
#include "Services/ThreeDimensionalPipelineService.h"

#include <QtConcurrent/QtConcurrent>
#include <stdexcept>
#include <utility>

ThreeDViewerController::ThreeDViewerController(QObject* parent)
    : QObject(parent),
      m_rebuildWatcher(new QFutureWatcher<ThreeDimensionalPipelineResult>(this))
{
    connect(m_rebuildWatcher, &QFutureWatcher<ThreeDimensionalPipelineResult>::finished, this, &ThreeDViewerController::handleRebuildFinished);
}

ThreeDViewerController::~ThreeDViewerController() = default;

bool ThreeDViewerController::isBusy() const
{
    return m_isBusy;
}

bool ThreeDViewerController::meshAvailable() const
{
    return m_sceneAdapter.hasMesh();
}

QString ThreeDViewerController::errorText() const
{
    return m_errorText;
}

QString ThreeDViewerController::profileName() const
{
    return m_profile ? QString::fromUtf8(m_profile->name().data(), static_cast<qsizetype>(m_profile->name().size()))
                     : QString{};
}

QString ThreeDViewerController::anatomyLabel() const
{
    return m_anatomyLabel;
}

QColor ThreeDViewerController::surfaceColor() const
{
    return m_surfaceColor;
}

int ThreeDViewerController::vertexCount() const
{
    return m_sceneAdapter.hasMesh() ? static_cast<int>(m_sceneAdapter.mesh()->vertexCount()) : 0;
}

int ThreeDViewerController::triangleCount() const
{
    return m_sceneAdapter.hasMesh() ? static_cast<int>(m_sceneAdapter.mesh()->triangleCount()) : 0;
}

void ThreeDViewerController::setPipelineService(std::shared_ptr<ThreeDimensionalPipelineService> pipelineService)
{
    m_pipelineService = std::move(pipelineService);
}

void ThreeDViewerController::setProfile(std::shared_ptr<I3dPipelineProfile> profile)
{
    m_profile = std::move(profile);
    emit profileChanged();
}

void ThreeDViewerController::setVisualStyle(const ThreeDProfileVisualStyle& visualStyle)
{
    const bool labelChanged = m_anatomyLabel != visualStyle.anatomyLabel;
    const bool colorChanged = m_surfaceColor != visualStyle.surfaceColor;

    m_anatomyLabel = visualStyle.anatomyLabel;
    m_surfaceColor = visualStyle.surfaceColor;

    if (labelChanged || colorChanged)
    {
        emit visualStyleChanged();
    }
}

void ThreeDViewerController::rebuildFromVolume(const std::shared_ptr<IVolumeData>& volume)
{
    if (!m_pipelineService || !m_profile || !volume)
    {
        setErrorText("3D viewer is missing pipeline service, profile, or volume");
        return;
    }

    m_currentVolume = volume;
    ++m_rebuildGeneration;

    setErrorText({});
    setBusy(true);

    const std::shared_ptr<ThreeDimensionalPipelineService> pipelineService = m_pipelineService;
    const std::shared_ptr<I3dPipelineProfile> profile = m_profile;
    const std::shared_ptr<IVolumeData> diagnosticVolume = volume;

    m_rebuildWatcher->setFuture(QtConcurrent::run([pipelineService, profile, diagnosticVolume]() {
        if (!pipelineService || !profile || !diagnosticVolume)
        {
            throw std::runtime_error("3D viewer is missing pipeline service, profile, or volume");
        }

        ThreeDimensionalPipelineResult result = pipelineService->buildMesh(*diagnosticVolume, *profile);
        if (!result.isValid())
        {
            throw std::runtime_error("3D pipeline returned an invalid result");
        }

        return result;
    }));
}

void ThreeDViewerController::clearMesh()
{
    m_sceneAdapter.clear();
    emitMeshStateChanged();
    setErrorText({});
}

const MeshSceneAdapter& ThreeDViewerController::sceneAdapter() const
{
    return m_sceneAdapter;
}

void ThreeDViewerController::setBusy(bool busy)
{
    if (m_isBusy == busy)
    {
        return;
    }

    m_isBusy = busy;
    emit isBusyChanged();
}

void ThreeDViewerController::setErrorText(QString errorText)
{
    if (m_errorText == errorText)
    {
        return;
    }

    m_errorText = std::move(errorText);
    emit errorTextChanged();
}

void ThreeDViewerController::emitMeshStateChanged()
{
    emit meshAvailableChanged();
    emit meshStatsChanged();
}

void ThreeDViewerController::handleRebuildFinished()
{
    try
    {
        const ThreeDimensionalPipelineResult result = m_rebuildWatcher->result();
        if (!result.isValid())
        {
            throw std::runtime_error("3D pipeline returned an invalid result");
        }

        m_sceneAdapter.setMesh(result.mesh);
        emitMeshStateChanged();
        setErrorText({});
    }
    catch (const std::exception& exception)
    {
        m_sceneAdapter.clear();
        emitMeshStateChanged();
        setErrorText(QString::fromUtf8(exception.what()));
    }

    setBusy(false);
}
