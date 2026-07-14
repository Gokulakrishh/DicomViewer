#include "DicomViewerWindow/VideoExportController.h"

#include "Audit/AuditEvent.h"
#include "Audit/IAuditService.h"
#include "DicomViewerWindow/VideoExportDialog.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Services/VideoExport/DicomCineFrameProvider.h"
#include "Services/VideoExport/DicomSeriesFrameProvider.h"
#include "Services/VideoExport/IVideoExportService.h"
#include "Services/VideoExport/IVideoFrameProvider.h"

#include <QApplication>
#include <QDialog>
#include <QFileInfo>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>

VideoExportController::VideoExportController(
    std::shared_ptr<IVideoExportService> exportService,
    IAuditService* auditService,
    QObject* parent)
    : QObject(parent),
      m_exportService(std::move(exportService)),
      m_auditService(auditService)
{
    connect(
        &m_watcher,
        &QFutureWatcher<VideoExportResult>::finished,
        this,
        &VideoExportController::finishExport);
    connect(
        this,
        &VideoExportController::progressChanged,
        this,
        [this](int completedFrames, int totalFrames) {
            if (!m_progressDialog)
            {
                return;
            }
            m_progressDialog->setMaximum(totalFrames);
            m_progressDialog->setValue(completedFrames);
            m_progressDialog->setLabelText(
                QStringLiteral("Encoding frame %1 of %2...")
                    .arg(completedFrames)
                    .arg(totalFrames));
        },
        Qt::QueuedConnection);
}

VideoExportController::~VideoExportController()
{
    if (m_cancellation)
    {
        m_cancellation->store(true);
    }
    if (m_watcher.isRunning())
    {
        m_watcher.waitForFinished();
    }
}

bool VideoExportController::isRunning() const
{
    return m_watcher.isRunning();
}

bool VideoExportController::startExport(const Context& context, QWidget* parentWidget)
{
    const bool hasSource = context.sourceKind == VideoExportSourceKind::SliceSeries
        ? !context.frameSources.empty()
        : !context.sourceFilePath.trimmed().isEmpty();
    if (!m_exportService || isRunning() || !hasSource || context.frameCount <= 1 ||
        !context.frameSize.isValid())
    {
        return false;
    }

    VideoExportDialog::Context dialogContext;
    dialogContext.suggestedBaseName = context.suggestedBaseName;
    dialogContext.sourceSopInstanceUid = context.sourceSopInstanceUid;
    dialogContext.sourceSeriesInstanceUid = context.sourceSeriesInstanceUid;
    dialogContext.productVersion = context.productVersion;
    dialogContext.sourceKind = context.sourceKind;
    dialogContext.frameCount = context.frameCount;
    dialogContext.currentFrameIndex = context.currentFrameIndex;
    dialogContext.defaultFramesPerSecond = context.defaultFramesPerSecond;
    dialogContext.timingSource = context.timingSource;

    VideoExportDialog dialog(dialogContext, parentWidget);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    m_activeRequest = dialog.request();
    m_cancellation = std::make_shared<std::atomic_bool>(false);
    const std::shared_ptr<std::atomic_bool> cancellation = m_cancellation;

    m_progressDialog = new QProgressDialog(
        QStringLiteral("Preparing video export..."),
        QStringLiteral("Cancel"),
        0,
        m_activeRequest.lastFrameIndex - m_activeRequest.firstFrameIndex + 1,
        parentWidget);
    m_progressDialog->setWindowTitle(QStringLiteral("Export Cine"));
    m_progressDialog->setWindowModality(Qt::ApplicationModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->setAutoClose(false);
    m_progressDialog->setAutoReset(false);
    m_progressDialog->setValue(0);
    connect(m_progressDialog, &QProgressDialog::canceled, this, [cancellation]() {
        cancellation->store(true);
    });
    m_progressDialog->show();

    const VideoExportRequest request = m_activeRequest;
    const std::shared_ptr<IVideoExportService> exportService = m_exportService;
    emit runningChanged(true);

    m_watcher.setFuture(QtConcurrent::run(
        [this, request, context, exportService, cancellation]() {
            GDCMFileHandling fileHandling;
            std::unique_ptr<IVideoFrameProvider> frameProvider;
            if (context.sourceKind == VideoExportSourceKind::SliceSeries)
            {
                frameProvider = std::make_unique<DicomSeriesFrameProvider>(
                    fileHandling,
                    context.frameSources,
                    context.frameSize,
                    context.windowLevel,
                    context.windowWidth);
            }
            else
            {
                frameProvider = std::make_unique<DicomCineFrameProvider>(
                    fileHandling,
                    context.sourceFilePath,
                    context.frameCount,
                    context.frameSize,
                    context.windowLevel,
                    context.windowWidth);
            }

            return exportService->exportVideo(
                request,
                *frameProvider,
                [this](int completedFrames, int totalFrames) {
                    emit progressChanged(completedFrames, totalFrames);
                },
                [cancellation]() {
                    return cancellation->load();
                });
        }));

    return true;
}

void VideoExportController::finishExport()
{
    closeProgressDialog();

    const VideoExportResult result = m_watcher.result();
    recordAudit(m_activeRequest, result);
    m_cancellation.reset();
    emit runningChanged(false);

    if (result.success)
    {
        emit exportSucceeded(
            QStringLiteral("Exported %1 frames to %2")
                .arg(result.exportedFrameCount)
                .arg(QFileInfo(result.outputPath).fileName()));
    }
    else if (result.cancelled)
    {
        emit exportCancelled();
    }
    else
    {
        emit exportFailed(
            result.errorMessage.trimmed().isEmpty()
                ? QStringLiteral("Video export failed.")
                : result.errorMessage);
    }
}

void VideoExportController::recordAudit(
    const VideoExportRequest& request,
    const VideoExportResult& result)
{
    if (!m_auditService)
    {
        return;
    }

    AuditEvent event;
    event.type = AuditEventType::UserAction;
    event.severity = result.success
        ? AuditSeverity::Info
        : (result.cancelled ? AuditSeverity::Warning : AuditSeverity::Error);
    event.module = QStringLiteral("VideoExport");
    event.action = result.success
        ? QStringLiteral("ExportCompleted")
        : (result.cancelled ? QStringLiteral("ExportCancelled") : QStringLiteral("ExportFailed"));
    event.subjectId = request.sourceKind == VideoExportSourceKind::SliceSeries
        ? request.sourceSeriesInstanceUid
        : request.sourceSopInstanceUid;
    event.message = result.success
        ? QStringLiteral("Derived non-diagnostic cine video export completed.")
        : (result.cancelled
               ? QStringLiteral("Derived non-diagnostic cine video export was cancelled.")
               : QStringLiteral("Derived non-diagnostic cine video export failed."));
    event.attributes.insert(QStringLiteral("sourceKind"), videoExportSourceKindName(request.sourceKind));
    event.attributes.insert(QStringLiteral("sourceSopInstanceUid"), request.sourceSopInstanceUid);
    event.attributes.insert(QStringLiteral("sourceSeriesInstanceUid"), request.sourceSeriesInstanceUid);
    event.attributes.insert(
        QStringLiteral("frameRange"),
        QStringLiteral("%1-%2").arg(request.firstFrameIndex).arg(request.lastFrameIndex));
    event.attributes.insert(
        QStringLiteral("framesPerSecond"),
        QString::number(request.framesPerSecond, 'f', 3));
    event.attributes.insert(QStringLiteral("timingSource"), result.timingSource);
    event.attributes.insert(QStringLiteral("container"), result.container);
    event.attributes.insert(QStringLiteral("codec"), result.codec);
    event.attributes.insert(QStringLiteral("encoder"), result.encoder);
    event.attributes.insert(QStringLiteral("h264Profile"), result.h264Profile);
    event.attributes.insert(QStringLiteral("pixelFormat"), result.pixelFormat);
    event.attributes.insert(
        QStringLiteral("targetBitrateKbps"),
        QString::number(result.targetBitrateKbps));
    event.attributes.insert(
        QStringLiteral("keyframeIntervalFrames"),
        QString::number(result.keyframeIntervalFrames));
    event.attributes.insert(QStringLiteral("gstreamerVersion"), result.gstreamerVersion);
    event.attributes.insert(QStringLiteral("productVersion"), request.productVersion);
    event.attributes.insert(QStringLiteral("outputFormat"), QStringLiteral("mp4"));
    event.attributes.insert(
        QStringLiteral("derivedNonDiagnostic"),
        request.derivedNonDiagnosticOutput ? QStringLiteral("true") : QStringLiteral("false"));
    event.attributes.insert(
        QStringLiteral("patientOverlaysIncluded"),
        request.includePatientOverlays ? QStringLiteral("true") : QStringLiteral("false"));
    m_auditService->record(event);
}

void VideoExportController::closeProgressDialog()
{
    if (!m_progressDialog)
    {
        return;
    }

    m_progressDialog->close();
    m_progressDialog->deleteLater();
    m_progressDialog = nullptr;
}
