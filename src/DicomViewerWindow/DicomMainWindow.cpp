#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QDebug>

#include "DicomMainWindow.h"
#include "Model/MedicalImage.h"
#include "FileHandling/GDCMFileHandling.h"

DicomMainWindow::DicomMainWindow(QWidget *parent)
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

    // Create a scene and set it to the view
    m_view = new DicomGraphicsView(this);
    m_ui->graphicsView->layout()->addWidget(m_view);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
}

void DicomMainWindow::setupMenuBar()
{

    m_openFileAction = new QAction("Open", this);
    m_ui->fileMenu->addAction(m_openFileAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
    /*
    m_saveFileAction = new QAction("Save", this);
    m_ui->fileMenu->addAction(m_saveFileAction);
    m_undoFileAction = new QAction("Undo", this);
    m_ui->fileMenu->addAction(m_undoFileAction);
    m_redoFileAction = new QAction("Redo", this);
    m_ui->fileMenu->addAction(m_redoFileAction);
*/

}

void DicomMainWindow::setupConnections()
{

    //connect(zoomSlider_, &QSlider::valueChanged, this, &MainWindow::onZoomChanged);
}

void DicomMainWindow::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open Image", QString(),
        "DICOM Images (*.png *.jpg *.bmp *.dcm);;All Files (*)");

    if (fileName.isEmpty())
        return;

    // Load the image
    QPixmap pixmap = displayImage(fileName);
    if (pixmap.isNull())
    {
        //qDebug() << "Failed to load image:" << fileName;
        return;
    }
    else
    {
       //displayImage(fileName);
    }
}

QPixmap DicomMainWindow::displayImage(const QString& filePath)
{

    gdcm::ImageReader reader;
    reader.SetFileName(filePath.toStdString().c_str());

    if (!reader.Read()) {
        qDebug() << "Failed to read DICOM file:" << filePath;
        return QPixmap();
    }

    const gdcm::Image& gdcmImage = reader.GetImage();
    unsigned int width = gdcmImage.GetDimensions()[0];
    unsigned int height = gdcmImage.GetDimensions()[1];
    unsigned long bufferLength = gdcmImage.GetBufferLength();

    std::vector<char> buffer(bufferLength);
    if (!gdcmImage.GetBuffer(&buffer[0])) {
        qDebug() << "Failed to get pixel buffer!";
        return QPixmap();
    }

    // Determine the pixel type
    QImage::Format format = QImage::Format_Grayscale8;
    if (gdcmImage.GetPixelFormat().GetSamplesPerPixel() == 3) {
        format = QImage::Format_RGB888;
    }

    // Create QImage from buffer
    QImage image(reinterpret_cast<const uchar*>(&buffer[0]), width, height, format);

    // For RGB images, swap RGB bytes
    if (format == QImage::Format_RGB888) {
        image = image.rgbSwapped();
    }

    return QPixmap::fromImage(image);

    /*QPixmap scaled = image.getPixmap().scaled(
        image.getPixmap().size() * m_currentZoom,
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel_->setPixmap(scaled);*/
}

void DicomMainWindow::onZoomChanged(int value)
{
   /* m_currentZoom = value / 100.0;
    if (imageManager_->hasImage()) {
        displayImage(*imageManager_->getCurrentImage());
    }*/
}

