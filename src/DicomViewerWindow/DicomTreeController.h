#pragma once

#include <QModelIndex>
#include <QObject>

class DatabaseService;
class DicomTreePanel;

class DicomTreeController : public QObject
{
    Q_OBJECT

public:
    explicit DicomTreeController(QObject* parent = nullptr);

    void setDatabaseService(DatabaseService* databaseService);
    void bindPanel(DicomTreePanel* treePanel);
    void refreshHierarchy();

signals:
    void patientContextSelected(
        const QString& patientName,
        const QString& patientDob,
        const QString& doctorName,
        const QString& modality,
        const QString& studyDate);
    void seriesSelectionRequested(const QString& seriesInstanceUid);
    void fileSelectionRequested(const QString& filePath);

private slots:
    void onHierarchyItemActivated(const QModelIndex& index);
    void onHierarchyItemExpanded(const QModelIndex& index);
    void onGlobalSearchTextChanged(const QString& text);

private:
    DatabaseService* m_databaseService{nullptr};
    DicomTreePanel* m_treePanel{nullptr};
};
