#pragma once

#include "Services/VideoExport/VideoExportTypes.h"
#include "Utilities/AppDialogBase.h"

class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;

/**
 * @brief Collects the bounded Phase 1 cine/video export request.
 *
 * Responsibilities:
 * - Present frame range, frame rate, timing source, and output location.
 * - Enforce MP4/H.264 output and non-diagnostic/no-PHI policy.
 *
 * Assumptions:
 * - Source DICOM identity and pixel access remain outside the dialog.
 */
class VideoExportDialog final : public AppDialogBase
{
    Q_OBJECT

public:
    struct Context
    {
        QString suggestedBaseName;
        QString sourceSopInstanceUid;
        QString sourceSeriesInstanceUid;
        QString productVersion;
        VideoExportSourceKind sourceKind{VideoExportSourceKind::MultiFrameSop};
        int frameCount{0};
        int currentFrameIndex{0};
        double defaultFramesPerSecond{10.0};
        VideoExportTimingSource timingSource{VideoExportTimingSource::Manual};
    };

    /**
     * @brief Creates a request dialog for one source cine object.
     * @param context Lightweight source and timing context.
     * @param parent Optional parent widget.
     */
    explicit VideoExportDialog(Context context, QWidget* parent = nullptr);

    /**
     * @brief Returns the request selected by the user.
     * @return Zero-based frame range and output settings.
     */
    VideoExportRequest request() const;

private slots:
    void chooseOutputPath();
    void synchronizeFrameRange();
    void accept() override;

private:
    Context m_context;
    QSpinBox* m_firstFrameSpinBox{nullptr};
    QSpinBox* m_lastFrameSpinBox{nullptr};
    QDoubleSpinBox* m_framesPerSecondSpinBox{nullptr};
    QLineEdit* m_outputPathEdit{nullptr};
};
