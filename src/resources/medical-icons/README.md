# Medical Toolbar Icons

Qt-safe SVG icon pack for a C++/Qt/VTK medical imaging desktop toolbar.

## Files

- `dark/*.svg`: dark stroke icons for light toolbars.
- `light/*.svg`: light stroke icons for dark toolbars.
- `medical-icons.qrc`: Qt resource file with aliases under `:/medical-icons/`.
- `manifest.json`: icon ids, labels, intended use, and file paths.
- `contact-sheet.svg`: quick visual preview.
- `preview.html`: browser preview grid.

## Qt Usage

Add the resource file to your Qt project:

```qmake
RESOURCES += medical-icons/medical-icons.qrc
```

Or for CMake:

```cmake
qt_add_resources(APP_RESOURCES medical-icons/medical-icons.qrc)
target_sources(YourTarget PRIVATE ${APP_RESOURCES})
```

Use icons from C++:

```cpp
auto wlAction = new QAction(
    QIcon(":/medical-icons/light/window-level.svg"),
    tr("WL/WW"),
    this
);
toolbar->addAction(wlAction);
```

For a dark viewer toolbar, start with `light/` icons. For a light toolbar, use `dark/` icons.

## Icon Names

- `open-file`
- `open-folder`
- `window-level`
- `zoom`
- `pan`
- `distance`
- `polyline`
- `angle`
- `roi`
- `export-cine`
- `open-mpr`
- `open-3d`
- `play`
- `pause`
- `stop`
- `previous-slice`
- `next-slice`
- `mpr-slab-mode`
- `slab-thickness`
- `oblique-mode`
- `oblique-angle`
- `study-browser`
- `annotations`
- `dicom-info`
- `delete-annotation`
- `go-to-annotation`
- `edit-label`
- `search`
- `expand`
- `collapse`

## Style Notes

These SVGs intentionally avoid CSS variables, filters, masks, gradients, and external fonts so Qt's SVG renderer can handle them consistently across platforms.
