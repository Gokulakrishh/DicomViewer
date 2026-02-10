#pragma once

#include <QMainWindow>
#include "ui_DicomViewerWindow.h"

class DicomViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
     DicomViewerWindow(QWidget *parent = nullptr);
    ~DicomViewerWindow();
    
    void InitGui(void);

private:
    Ui::DicomViewerWindow *m_ui;
};
