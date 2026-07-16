#pragma once

#include "Model/MeasurementAnnotationRecord.h"

#include <QHash>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief Lightweight MPR annotation review panel.
 *
 * Responsibilities:
 * - Present active-series MPR annotations grouped by derived plane context.
 * - Forward navigation and deletion intents without owning persistence.
 *
 * Assumptions:
 * - Rows are already scoped to the active MPR series.
 * - The panel displays derived MPR records only and does not mix source-slice
 *   annotations from the main viewer.
 */
class VtkMprAnnotationDock final : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Creates the dock content widget.
     * @param parent Optional Qt parent.
     */
    explicit VtkMprAnnotationDock(QWidget* parent = nullptr);

    /**
     * @brief Replaces the displayed MPR annotation records.
     * @param records Active-series MPR annotation records.
     */
    void setRecords(const QList<MprMeasurementAnnotationRecord>& records);

signals:
    /**
     * @brief Requests navigation to a stored MPR annotation.
     * @param annotationId Stable annotation identifier.
     */
    void goToAnnotationRequested(const QString& annotationId);

    /**
     * @brief Requests soft deletion of a stored MPR annotation.
     * @param annotationId Stable annotation identifier.
     */
    void deleteAnnotationRequested(const QString& annotationId);

    /**
     * @brief Requests editable metadata update for an MPR annotation.
     * @param annotationId Stable annotation identifier.
     * @param label User-visible annotation label.
     * @param bodyRegion Body region/group.
     * @param note Optional note text.
     */
    void metadataChanged(
        const QString& annotationId,
        const QString& label,
        const QString& bodyRegion,
        const QString& note);

private:
    void buildUi();
    void refreshActions();
    void refreshEditor();
    [[nodiscard]] QString selectedAnnotationId() const;
    [[nodiscard]] static QString displayValue(const MprMeasurementAnnotationRecord& record);
    [[nodiscard]] static QString measurementTypeName(MeasurementType type);
    [[nodiscard]] static QString groupKey(const MprMeasurementAnnotationRecord& record);

private:
    QLabel* m_summaryLabel{nullptr};
    QTreeWidget* m_treeWidget{nullptr};
    QLineEdit* m_labelEdit{nullptr};
    QComboBox* m_bodyRegionComboBox{nullptr};
    QLineEdit* m_noteEdit{nullptr};
    QPushButton* m_goButton{nullptr};
    QPushButton* m_applyButton{nullptr};
    QPushButton* m_deleteButton{nullptr};
    QHash<QString, MprMeasurementAnnotationRecord> m_recordsById;
};
