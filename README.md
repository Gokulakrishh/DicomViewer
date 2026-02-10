

## ✨ Overview

**DicomViewer** is a C++ medical imaging application designed with **clean architecture**, **modern C++**, and **extensibility** in mind.

The project focuses on **separating UI from business logic**, making it easy to support:
- new medical image formats
- multiple rendering backends
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
│   ├── Model/                 # Domain models (MedicalImage, DicomImage)
│   ├── FileHandling/          # File loaders (GDCMFileHandler, interfaces)
│   ├── DicomViewerWindow/     # Qt UI (MainWindow, .ui files)
│   └── Main/                  # Application entry point
├── build/                     # Build directory (ignored)
├── CMakeLists.txt
├── README.md
└── .gitignore

````

---

## 🏗️ Architecture

### Model Layer
```cpp
MedicalImage   (abstract)
   ▲
   │
DicomImage     (concrete)
````

* `MedicalImage` defines the **minimal contract**
* `DicomImage` implements DICOM-specific logic
* UI never depends on concrete image types

---

### File Handling Layer

```cpp
IMedicalFileHandler   (abstract)
   ▲
   │
GDCMFileHandler       (DICOM)
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
* **Qt 5** (Core, Gui, Widgets)
* **VTK**
* **GDCM**
* **OpenCV ≥ 4.6**

---

## 🧪 Build Instructions

```bash
git clone https://github.com/<your-username>/DicomViewer.git
cd DicomViewer

mkdir build
cd build

cmake ..
cmake --build .
./DicomViewer
```

---

## 🎯 Motivation

This project is built as a **portfolio-quality system**, focusing on:

* modern C++ practices
* clean, testable architecture
* medical imaging workflows
* long-term extensibility toward AI-assisted tools

---

## 📄 License

MIT License

```

---

