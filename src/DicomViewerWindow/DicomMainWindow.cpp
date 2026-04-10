#include "DicomMainWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStringList>
#include <QTreeView>

#include "Config/QSettingsDatabaseConfigService.h"
#include "Database/PostgreService.h"
#include "FileHandling/GDCMFileHandling.h"
#include "Model/MedicalImage.h"
#include "UI/WarningDialogService.h"

constexpr int kFilePathRole = Qt::UserRole + 1;

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

    m_treeView = new QTreeView(this);
    m_treeModel = new QStandardItemModel(this);
    m_treeModel->setHorizontalHeaderLabels({"DICOM Database"});
    m_treeView->setModel(m_treeModel);
    m_treeView->setHeaderHidden(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setUniformRowHeights(true);
    m_ui->horizontalLayout->replaceWidget(m_ui->listFileView, m_treeView);
    m_ui->listFileView->deleteLater();

    m_view = new DicomGraphicsView(this);
    m_ui->horizontalLayout->replaceWidget(m_ui->graphicsView, m_view);
    m_ui->graphicsView->deleteLater();

    m_ui->contrastVerticalSlider->setEnabled(false);
    m_ui->imageVerticalSlider->setEnabled(false);
    m_ui->playButton->setEnabled(false);

    m_gdcmHandler = std::make_unique<GDCMFileHandling>();
    m_warningDialogService = std::make_unique<WarningDialogService>(this);
    QSettingsDatabaseConfigService databaseConfigService;
    m_databaseService = std::make_unique<PostgreService>(databaseConfigService.loadDatabaseSettings());

    if (!m_databaseService->initialize())
    {
        const QString warningMessage =
            m_databaseService->lastErrorText() + " Config file: " + databaseConfigService.configFilePath();
        statusBar()->showMessage(warningMessage, 10000);
        m_warningDialogService->showWarning("Database Configuration", warningMessage);
    }

    setupConnections();
    refreshHierarchyTree();
}

void DicomMainWindow::setupMenuBar()
{
    m_openFileAction = new QAction("Open File", this);
    m_ui->fileMenu->addAction(m_openFileAction);

    m_openFolderAction = new QAction("Open Folder", this);
    m_ui->fileMenu->addAction(m_openFolderAction);

    connect(m_openFileAction, &QAction::triggered, this, &DicomMainWindow::openImage);
    connect(m_openFolderAction, &QAction::triggered, this, &DicomMainWindow::openFolder);
}

void DicomMainWindow::setupConnections()
{
    if (m_treeView)
    {
        connect(m_treeView, &QTreeView::clicked, this, &DicomMainWindow::onHierarchyItemActivated);
    }
}

void DicomMainWindow::refreshHierarchyTree()
{
    if (!m_treeModel)
    {
        return;
    }

    m_treeModel->removeRows(0, m_treeModel->rowCount());
    if (!m_databaseService)
    {
        return;
    }

    const QList<DatabaseService::PatientPtr> patients = m_databaseService->getAllPatients();
    for (const auto& patient : patients)
    {
        addPatientToTree(patient);
    }

    m_treeView->expandToDepth(2);
}

void DicomMainWindow::addPatientToTree(const std::shared_ptr<Patient>& patient)
{
    if (!patient || !m_treeModel)
    {
        return;
    }

    QString patientLabel = patient->patientId();
    if (!patient->patientName().isEmpty())
    {
        patientLabel += " | " + patient->patientName();
    }

    auto* patientItem = new QStandardItem(patientLabel);
    m_treeModel->invisibleRootItem()->appendRow(patientItem);

    for (const auto& [studyInstanceUid, study] : patient->studyMap())
    {
        Q_UNUSED(studyInstanceUid);
        QString studyLabel = study->studyInstanceUid();
        if (!study->studyDescription().isEmpty())
        {
            studyLabel += " | " + study->studyDescription();
        }
        if (!study->studyDate().isEmpty())
        {
            studyLabel += " | " + study->studyDate();
        }

        auto* studyItem = new QStandardItem(studyLabel);
        patientItem->appendRow(studyItem);

        for (const auto& [seriesInstanceUid, series] : study->seriesMap())
        {
            Q_UNUSED(seriesInstanceUid);
            QString seriesLabel = series->seriesInstanceUid();
            if (!series->seriesDescription().isEmpty())
            {
                seriesLabel += " | " + series->seriesDescription();
            }
            if (!series->modality().isEmpty())
            {
                seriesLabel += " | " + series->modality();
            }

            auto* seriesItem = new QStandardItem(seriesLabel);
            studyItem->appendRow(seriesItem);

            for (const auto& image : series->images())
            {
                if (!image)
                {
                    continue;
                }

                QString imageLabel = image->instanceNumber().isEmpty() ? image->sopInstanceUid() : image->instanceNumber();
                imageLabel += " | " + QFileInfo(image->filePath()).fileName();

                auto* imageItem = new QStandardItem(imageLabel);
                imageItem->setData(image->filePath(), kFilePathRole);
                seriesItem->appendRow(imageItem);
            }
        }
    }
}

void DicomMainWindow::loadAndDisplayImage(const QString& filePath)
{
    if (!m_gdcmHandler)
    {
        statusBar()->showMessage("File handler is not available.", 4000);
        m_warningDialogService->showWarning("File Loading", "File handler is not available.");
        return;
    }

    std::unique_ptr<MedicalImage> loadedImage = m_gdcmHandler->loadImage(filePath);
    if (!loadedImage || !loadedImage->isValid())
    {
        const QString warningMessage = "Failed to load image: " + QFileInfo(filePath).fileName();
        statusBar()->showMessage(warningMessage, 4000);
        m_warningDialogService->showWarning("Image Loading", warningMessage);
        m_view->clearImage();
        return;
    }

    std::shared_ptr<MedicalImage> image = std::move(loadedImage);
    m_view->setImage(std::move(image));
    statusBar()->showMessage("Loaded: " + QFileInfo(filePath).fileName(), 4000);
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

    loadAndDisplayImage(fileName);

    QString statusMessage = "Loaded: " + QFileInfo(fileName).fileName();
    if (m_databaseService && m_gdcmHandler)
    {
        FileHandling::PatientPtr patientHierarchy = m_gdcmHandler->loadDicomHierarchy(fileName);
        if (patientHierarchy)
        {
            if (m_databaseService->savePatient(patientHierarchy))
            {
                statusMessage += " | Imported into PostgreSQL";
                refreshHierarchyTree();
            }
            else
            {
                statusMessage += " | PostgreSQL save failed";
                m_warningDialogService->showWarning("Database Import", "Failed to save the selected DICOM into PostgreSQL.");
            }
        }
    }

    statusBar()->showMessage(statusMessage, 5000);
}

void DicomMainWindow::openFolder()
{
    const QString folderPath = QFileDialog::getExistingDirectory(this, "Open DICOM Folder");
    if (folderPath.isEmpty())
    {
        return;
    }

    if (!m_gdcmHandler || !m_databaseService)
    {
        statusBar()->showMessage("Folder import is not available.", 4000);
        m_warningDialogService->showWarning("Folder Import", "Folder import is not available.");
        return;
    }

    const FileHandling::PatientList patients = m_gdcmHandler->loadDicomFolder(folderPath);
    if (patients.isEmpty())
    {
        statusBar()->showMessage("No importable DICOM files found in folder.", 5000);
        m_warningDialogService->showWarning("Folder Import", "No importable DICOM files found in the selected folder.");
        return;
    }

    int importedPatientCount = 0;
    for (const auto& patient : patients)
    {
        if (m_databaseService->savePatient(patient))
        {
            ++importedPatientCount;
        }
    }

    if (importedPatientCount == 0)
    {
        m_warningDialogService->showWarning("Database Import", "The folder was scanned, but no patient hierarchy could be saved into PostgreSQL.");
    }

    refreshHierarchyTree();
    m_treeView->expandAll();
    statusBar()->showMessage(
        QString("Imported folder %1 | Patients saved: %2").arg(QFileInfo(folderPath).fileName()).arg(importedPatientCount),
        6000);
}

void DicomMainWindow::onHierarchyItemActivated(const QModelIndex& index)
{
    if (!index.isValid() || !m_treeModel)
    {
        return;
    }

    const QString filePath = m_treeModel->itemFromIndex(index)->data(kFilePathRole).toString();
    if (filePath.isEmpty())
    {
        return;
    }

    loadAndDisplayImage(filePath);
}
