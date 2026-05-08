#pragma once

#include "Model/AnnotationReportSummary.h"

#include <QWidget>

class QLabel;
class QVBoxLayout;

/**
 * @brief Editor panel for annotations on the active DICOM slice.
 *
 * Responsibilities:
 * - Display the current slice context selected in the main viewer.
 * - Allow user-editable annotation labels and body-region grouping.
 * - Emit delete requests for slice-scoped annotations.
 *
 * Assumptions:
 * - The panel does not persist changes directly.
 * - Rows are only for the active SOP Instance UID.
 */
class CurrentSliceAnnotationPanel final : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates the current-slice annotation panel.
     * @param parent Optional Qt parent.
     */
    explicit CurrentSliceAnnotationPanel(QWidget* parent = nullptr);

    /**
     * @brief Updates the active slice context.
     * @param context Current DICOM slice and series metadata.
     */
    void setSliceContext(const AnnotationCurrentSliceContext& context);

    /**
     * @brief Replaces annotations shown for the active slice.
     * @param rows Current-slice annotation rows.
     */
    void setRows(const AnnotationReportRows& rows);

signals:
    /**
     * @brief Requests metadata update for an annotation.
     * @param annotationId Stable annotation identifier.
     * @param label User-facing annotation label.
     * @param bodyRegion Body region/group value.
     * @param seriesInstanceUid DICOM Series Instance UID.
     */
    void metadataChanged(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& seriesInstanceUid);

    /**
     * @brief Requests deletion of an annotation.
     * @param annotationId Stable annotation identifier.
     * @param seriesInstanceUid DICOM Series Instance UID.
     * @param sopInstanceUid DICOM SOP Instance UID.
     */
    void deleteRequested(
        const QString& annotationId,
        const QString& seriesInstanceUid,
        const QString& sopInstanceUid,
        int frameIndex);

private:
    void buildUi();
    void clearRows();
    void appendRowCard(const AnnotationReportRow& row);
    void updateDicomInfo();
    [[nodiscard]] QString contextTitle() const;
    [[nodiscard]] QString contextSubtitle() const;
    [[nodiscard]] QString dicomInfoText() const;

private:
    AnnotationCurrentSliceContext m_context;
    QLabel* m_dicomInfoTitleLabel{nullptr};
    QLabel* m_dicomInfoLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_subtitleLabel{nullptr};
    QWidget* m_rowsWidget{nullptr};
    QVBoxLayout* m_rowsLayout{nullptr};
};
