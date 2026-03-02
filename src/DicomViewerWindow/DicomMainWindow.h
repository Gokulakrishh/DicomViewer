#pragma once

#include <QMainWindow>
#include <QAction>
#include <QGraphicsScene>
#include "ui_DicomMainWindow.h"
#include "DicomGraphicsView.h"

class MedicalImage;
class FileHandling;

class DicomMainWindow : public QMainWindow
{
    Q_OBJECT

    Q_DISABLE_COPY_MOVE(DicomMainWindow) //For a QMainWindow subclass, you usually don’t want copying:

public:
    explicit DicomMainWindow(QWidget *parent = nullptr);
    ~DicomMainWindow();

private:
    void setUiComponents();
    void setupMenuBar();
    void setupConnections();
    QPixmap displayImage(const QString& filePath);

private slots:
    void openImage();
    void onZoomChanged(int);

private:
    Ui::DicomMainWindow *m_ui;

    DicomGraphicsView* m_view;

    //File
    QAction* m_openFileAction;
    //QAction* m_saveFileAction;
    //QAction* m_undoFileAction;
    //QAction* m_redoFileAction;
    //View
    //QAction* m_patientDetailsViewAction;
    //QAction* m_llmViewAction;
    //QAction* m_3DViewAction;
    double m_currentZoom;

    std::unique_ptr<FileHandling> m_gdcmHandler;

};

