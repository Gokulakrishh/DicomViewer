#include "DicomViewerWindow.h"

DicomViewerWindow::DicomViewerWindow(QWidget *parent)
    : QMainWindow(parent),
      m_ui(new Ui::DicomViewerWindow)
{
    m_ui->setupUi(this);
    InitGui();
}

DicomViewerWindow::~DicomViewerWindow()
{
    delete m_ui;
}

void DicomViewerWindow::InitGui()
{
  
}
