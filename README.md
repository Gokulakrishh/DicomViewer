

## ✨ Overview

**DicomViewer** is a C++ medical imaging application designed with **clean architecture**, **modern C++**, and **extensibility** in mind.

The project focuses on **separating UI from business logic**, making it easy to support:
- new medical image formats
- multiple rendering backends
- PostgreSQL-backed DICOM indexing
- future AI / LLM-assisted workflows

---

## 🧠 Design Philosophy

- 🔹 **Abstract interfaces first**
- 🔹 **UI completely decoupled from data**
- 🔹 **Concrete implementations hidden behind abstractions**
- 🔹 **Designed for growth, not just “working”**

This is not a monolithic viewer — it’s a **foundation**.

---

## 🗂️ Project Structure

```

DicomViewer/
├── src/
│   ├── Model/                 # Domain models (Patient, Study, Series, DicomImage)
│   ├── FileHandling/          # File loaders (GDCMFileHandler, interfaces)
│   ├── Database/              # PostgreSQL persistence services
│   ├── DicomViewerWindow/     # Qt UI (MainWindow, .ui files)
│   └── Main/                  # Application entry point
├── build/                     # Build directory (ignored)
├── CMakeLists.txt
├── README.md
└── .gitignore

````

---

## 🏗️ Architecture

### Domain Layer
```cpp
Patient
  └── Study
        └── Series
              └── DicomImage
````

* `MedicalImage` stays focused on image concerns
* `DicomImage` stores only image-level data
* patient / study / series are modeled separately to match DICOM hierarchy
* UI depends on abstractions, not PostgreSQL details

---

### File Handling Layer

```cpp
FileHandling   (abstract)
   ▲
   │
GDCMFileHandling       (DICOM)
```

* File loaders return `std::unique_ptr<MedicalImage>`
* Adding new formats does **not** affect UI code

---

### UI Layer

* Qt Widgets–based
* Talks only to **interfaces**
* No file-format or backend knowledge

---

## 🔬 Current Features

* ✅ DICOM loading via **GDCM**
* ✅ PostgreSQL persistence for patient / study / series / image hierarchy
* ✅ Qt Widgets UI
* ✅ Clean separation of concerns
* ✅ CMake-based cross-platform build
* ✅ Extensible image and file-handler architecture

---

## 🚀 Planned Features

* 🧩 Additional medical formats (NIfTI, NRRD)
* 🧊 3D volume rendering using VTK
* 📊 Metadata exploration & filtering
* 🤖 Local LLM integration (Ollama) for:

  * Image summaries
  * Metadata interpretation
  * Workflow assistance

---

## 🛠️ Build Requirements

* **C++20**
* **CMake ≥ 3.21**
* **Qt 6** (Core, Gui, Widgets, Sql)
* **GDCM**
* **OpenCV ≥ 4.6**
* **PostgreSQL** with Qt `QPSQL` driver

---

## 🧪 Build Instructions

```bash
git clone https://github.com/Gokulakrishh/DicomViewer.git
cd DicomViewer

mkdir build
cd build

cmake ..
cmake --build .
./DicomViewer
```

If Qt 6 is installed in a custom location, configure CMake with `CMAKE_PREFIX_PATH`, for example:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/home/dust/Qt/6.5.3/gcc_64
cmake --build build
```

## 🗄️ PostgreSQL Configuration

The application stores imported DICOM files in a normalized hierarchy:

* `patients`
* `studies`
* `series`
* `dicom_images`

PostgreSQL settings are read at runtime through `QSettings` from `config.ini`.

When running from `build-qt6`, the app looks for:

* `build-qt6/config.ini`
* then `../config.ini` relative to the executable, which resolves to the repo-root [`config.ini`](/home/dust/Documents/DicomViewer/config.ini)

Fill in the database section before launching:

```ini
[database]
hostName=127.0.0.1
port=5432
databaseName=dicomviewer
userName=postgres
password=your_password
```

When an image is opened, the app:

* loads the pixel data for viewing
* parses the DICOM hierarchy as `Patient -> Study -> Series -> DicomImage`
* upserts the hierarchy into PostgreSQL
* stores a PNG preview in `dicom_images` for quick reload

---

## 🎯 Motivation

This project is built as a hobby project, focusing on:

* modern C++ practices
* clean, testable architecture
* medical imaging workflows
* long-term extensibility toward AI-assisted tools

---

## 📄 License

MIT License

```

---
