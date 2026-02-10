cmake_minimum_required(VERSION 3.21)

project(DicomViewer
    VERSION 1.0
    DESCRIPTION "Dicom Image Viewer (2D/3D)"
    LANGUAGES C CXX
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
add_compile_definitions(__DicomViewer_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

# ----------------------------
# Global C++ settings
# ----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ----------------------------
# Find dependencies
# ----------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
message(STATUS "Qt version: ${QT_VERSION}")

find_package(VTK REQUIRED)
message(STATUS "VTK")

find_package(GDCM REQUIRED)
message(STATUS "GDCM")

find_package(OpenCV 4.6 REQUIRED core imgproc imgcodecs)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
 
# ----------------------------
# Source files
# ----------------------------
set(SRC_FILES
    src/Main/main.cpp
    src/FileHandling/GDCMFileHandling.cpp
    src/DicomViewerWindow/DicomViewerWindow.cpp
    src/DicomViewerWindow/DicomViewerWindow.ui
    src/VTKwidget/VTKwidget.cpp
)

set(HEADER_FILES
    src/Model/MedicalImage.h
    src/Model/DicomImage.h
    src/FileHandling/FileHandling.h
    src/FileHandling/GDCMFileHandling.h
    src/DicomViewerWindow/DicomViewerWindow.h
    src/VTKwidget/VTKwidget.h
)

# ----------------------------
# Qt executable
# ----------------------------
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HEADER_FILES}
)

# ----------------------------
# Include directories
# ----------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ----------------------------
# Link libraries
# ----------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        ${OpenCV_LIBS}
        ${VTK_LIBRARIES}
    	${GDCM_LIBRARIES}
)

# ---------------------------
# Auto-initializes VTK module factories at runtime
# Modern modular VTK requires it to avoid crashes
# ---------------------------
vtk_module_autoinit(
    TARGETS DicomViewer
    MODULES ${VTK_LIBRARIES}
)

# ----------------------------
# Build output folders
# ----------------------------
# Separate output for Debug and Release
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)

# Optional: also for libraries
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)

# ----------------------------
# Platform-specific settings //to do
# ----------------------------
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
    )
endif()


# ----------------------------
# Install (optional)
# ----------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    )
