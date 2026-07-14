#pragma once

#include "Services/VideoExport/VideoExportTypes.h"

#include <QFutureWatcher>
#include <QObject>
#include <QSize>

#include <atomic>
#include <memory>
#include <vector>

class IAuditService;
class IVideoExportService;
class QProgressDialog;
class QWidget;

/**
 * @brief Coordinates the main-view cine/video export presentation workflow.
 *
 * Responsibilities:
 * - Collect an export request through the dedicated dialog.
 * - Execute decoding/encoding outside the UI thread.
 * - Own progress, cancellation, result presentation signals, and audit events.
 *
 * Assumptions:
 * - The supplied context contains no patient name or patient ID.
 * - The controller does not retain DICOM pixels or modify viewer state.
 */
class VideoExportController final : public QObject
{
    Q_OBJECT

public:
    struct Context
    {
        QString sourceFilePath;
        QString sourceSopInstanceUid;
        QString sourceSeriesInstanceUid;
        QString suggestedBaseName;
        QString productVersion;
        VideoExportSourceKind sourceKind{VideoExportSourceKind::MultiFrameSop};
        std::vector<VideoExportFrameSource> frameSources;
        int frameCount{0};
        int currentFrameIndex{0};
        QSize frameSize;
        int windowLevel{0};
        int windowWidth{1};
        double defaultFramesPerSecond{10.0};
        VideoExportTimingSource timingSource{VideoExportTimingSource::Manual};
    };

    /**
     * @brief Creates the export workflow controller.
     * @param exportService Encoder service shared with worker tasks.
     * @param auditService Optional structured audit service; not owned.
     * @param parent Optional Qt parent.
     */
    VideoExportController(
        std::shared_ptr<IVideoExportService> exportService,
        IAuditService* auditService,
        QObject* parent = nullptr);
    ~VideoExportController() override;

    /** @brief Reports whether an export task is currently active. */
    bool isRunning() const;

    /**
     * @brief Opens the request dialog and starts export when accepted.
     * @param context Lightweight source/timing/windowing context.
     * @param parentWidget Parent for modal export presentation.
     * @return True when an asynchronous export was started.
     */
    bool startExport(const Context& context, QWidget* parentWidget);

signals:
    void runningChanged(bool running);
    void progressChanged(int completedFrames, int totalFrames);
    void exportSucceeded(const QString& message);
    void exportCancelled();
    void exportFailed(const QString& message);

private slots:
    void finishExport();

private:
    void recordAudit(const VideoExportRequest& request, const VideoExportResult& result);
    void closeProgressDialog();

private:
    std::shared_ptr<IVideoExportService> m_exportService;
    IAuditService* m_auditService{nullptr};
    QFutureWatcher<VideoExportResult> m_watcher;
    QProgressDialog* m_progressDialog{nullptr};
    std::shared_ptr<std::atomic_bool> m_cancellation;
    VideoExportRequest m_activeRequest;
};
