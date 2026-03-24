#include "DicomMainWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QStatusBar>
#include <QStringList>

#include "FileHandling/GDCMFileHandling.h"
#include "Model/MedicalImage.h"

DicomMainWindow::DicomMainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_ui(new Ui::DicomMainWindow)
{
    m_ui->setupUi(this);
    setUiComponents();
    resize(1200, 800);
}

DicomMainWindow::~DicomMainWindow()
{
    delete m_ui;
}

void DicomMainWindow::setUiComponents()
{
    setupMenuBar();
    setupConnections();

    m_view = new DicomGraphicsView(this);
    m_ui->horizontalLayout->replaceWidget(m_ui->graphicsView, m_view);
    m_ui->graphicsView->deleteLater();

    m_ui->listFileView->setEnabled(false);
    m_ui->contrastVerticalSlider->setEnabled(false);
    m_ui->imageVerticalSlider->setEnabled(false);
    m_ui->playButton->setEnabled(false);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
}

void DicomMainWindow::setupMenuBar()
{
    m_openFileAction = new QAction("Open", this);
    m_ui->fileMenu->addAction(m_openFileAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
}

void DicomMainWindow::setupConnections()
{
}

void DicomMainWindow::openImage()
{
    QStringList supportedFormats;
    if (m_gdcmHandler)
    {
        supportedFormats = m_gdcmHandler->getSupportedFormats();
    }

    QString filter = "DICOM/Image Files (*.dcm *.png *.jpg *.jpeg *.bmp)";
    if (!supportedFormats.isEmpty())
    {
        filter = "Supported Files (" + supportedFormats.join(' ') + ")";
    }

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QString(),
        filter + ";;All Files (*)");

    if (fileName.isEmpty())
    {
        return;
    }

    if (!m_gdcmHandler)
    {
        statusBar()->showMessage("File handler is not available.", 4000);
        return;
    }

    std::unique_ptr<MedicalImage> loadedImage = m_gdcmHandler->loadImage(fileName);
    if (!loadedImage || !loadedImage->isValid())
    {
        statusBar()->showMessage("Failed to load image: " + QFileInfo(fileName).fileName(), 4000);
        m_view->clearImage();
        return;
    }

    std::shared_ptr<MedicalImage> image = std::move(loadedImage);
    m_view->setImage(std::move(image));
    statusBar()->showMessage("Loaded: " + QFileInfo(fileName).fileName(), 3000);
}
