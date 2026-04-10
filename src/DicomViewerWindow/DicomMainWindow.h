#pragma once

#include <QAction>
#include <QMainWindow>
#include <QModelIndex>
#include <memory>

#include "DicomGraphicsView.h"
#include "ui_DicomMainWindow.h"

class FileHandling;
class DatabaseService;
class Patient;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class WarningDialogService;

class DicomMainWindow : public QMainWindow
{
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DicomMainWindow)

public:
    explicit DicomMainWindow(QWidget* parent = nullptr);
    ~DicomMainWindow();

private:
    void setUiComponents();
    void setupMenuBar();
    void setupConnections();
    void refreshHierarchyTree();
    void addPatientToTree(const std::shared_ptr<Patient>& patient);
    void loadAndDisplayImage(const QString& filePath);

private slots:
    void openImage();
    void openFolder();
    void onHierarchyItemActivated(const QModelIndex& index);

private:
    Ui::DicomMainWindow* m_ui{nullptr};
    DicomGraphicsView* m_view{nullptr};
    QTreeView* m_treeView{nullptr};
    QStandardItemModel* m_treeModel{nullptr};
    QAction* m_openFileAction{nullptr};
    QAction* m_openFolderAction{nullptr};
    std::unique_ptr<FileHandling> m_gdcmHandler;
    std::unique_ptr<DatabaseService> m_databaseService;
    std::unique_ptr<WarningDialogService> m_warningDialogService;
};
