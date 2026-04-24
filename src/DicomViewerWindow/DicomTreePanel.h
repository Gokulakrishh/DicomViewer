#pragma once

#include <QModelIndex>
#include <QPixmap>
#include <QWidget>
#include <memory>

class Patient;
class QLineEdit;
class QLabel;
class QSplitter;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class Series;
class Study;

class DicomTreePanel : public QWidget
{
    Q_OBJECT

public:
    explicit DicomTreePanel(QWidget* parent = nullptr);

    QStandardItem* itemFromIndex(const QModelIndex& index) const;
    QString localFilterText() const;
    QString globalFilterText() const;
    void refreshHierarchy(const QList<std::shared_ptr<Patient>>& patients);
    void appendStudies(QStandardItem* patientItem, const std::shared_ptr<Patient>& patient, const QList<std::shared_ptr<Study>>& studies);
    void appendSeries(QStandardItem* studyItem, const std::shared_ptr<Patient>& patient, const std::shared_ptr<Study>& study, const QList<std::shared_ptr<Series>>& seriesList);
    void updatePreviewPane(const QPixmap& pixmap);
    void updateSeriesPreview(const std::shared_ptr<Series>& series);

signals:
    void hierarchyItemActivated(const QModelIndex& index);
    void hierarchyItemExpanded(const QModelIndex& index);
    void localSearchTextChanged(const QString& text);
    void globalSearchTextChanged(const QString& text);

private:
    void buildUi();
    void applyTreeFilter(const QString& filterText);
    bool updateItemVisibility(QStandardItem* item, const QString& filterText);
    void addPatientToTree(const std::shared_ptr<Patient>& patient);
    void addStudyToTree(QStandardItem* patientItem, const std::shared_ptr<Patient>& patient, const std::shared_ptr<Study>& study);
    void addSeriesToTree(QStandardItem* studyItem, const std::shared_ptr<Patient>& patient, const std::shared_ptr<Study>& study, const std::shared_ptr<Series>& series);

private:
    QSplitter* m_splitter{nullptr};
    QTreeView* m_treeView{nullptr};
    QStandardItemModel* m_treeModel{nullptr};
    QLineEdit* m_searchLineEdit{nullptr};
    QLineEdit* m_globalSearchLineEdit{nullptr};
    QLabel* m_previewTitleLabel{nullptr};
    QLabel* m_previewImageLabel{nullptr};
};
