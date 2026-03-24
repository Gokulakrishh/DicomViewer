#pragma once

#include <QAction>
#include <QMainWindow>
#include <memory>

#include "DicomGraphicsView.h"
#include "ui_DicomMainWindow.h"

class FileHandling;

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

private slots:
    void openImage();

private:
    Ui::DicomMainWindow* m_ui{nullptr};
    DicomGraphicsView* m_view{nullptr};
    QAction* m_openFileAction{nullptr};
    std::unique_ptr<FileHandling> m_gdcmHandler;
};
