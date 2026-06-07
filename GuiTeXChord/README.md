# GuiTeXChord

Qt6 desktop application for creating guitar chord diagrams as TikZ/LaTeX output.

## Features

- Interactive fretboard (6–8 strings, multiple tunings), vertical and horizontal orientation
- Open string + 5 fretted positions per diagram
- Per-diagram global downtune (Std / −1 .. −8 semitones)
- Barré chord support (checkbox + fret radio buttons)
- Live QPainter preview, aspect-correct
- Export: standalone `.tex`, compile to PDF via pdflatex, copy tikzpicture to clipboard
- Constant output size regardless of chord name / position label
- English / German UI (switchable at runtime)
- LaTeX installation check at startup

## Requirements

| | Windows | Linux |
|---|---|---|
| Compiler | MinGW-w64 (Qt installer) | GCC / Clang |
| CMake | ≥ 3.20 | ≥ 3.20 |
| Qt | 6.x (C:/Qt/…) | 6.x via apt |
| LaTeX | MiKTeX / TeX Live | `texlive-pictures` |

## Building

### Windows (native)
```bat
cmake -B build\debug   -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake -B build\release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build\release
```

### Linux (native)
```bash
chmod +x unix/config.sh && ./unix/config.sh   # first time only
./unix/build.sh                                # Release
./unix/build.sh --debug                        # Debug
./unix/build.sh --appimage                     # Release + AppImage
./unix/build.sh --qt-prefix ~/Qt/6.x/gcc_64   # custom Qt path
```

### Linux → Windows cross-compile
```bash
# Install MXE and build Qt6 for mingw (one-time, ~1-2h):
#   See https://mxe.cc/#tutorial
./unix/build.sh --cross
./unix/build.sh --cross --mxe-root ~/mxe
```

## Build outputs

| Variant | Output directory |
|---|---|
| Windows Release | `build/GuiTeXChord_vX_YY_win/` |
| Windows Debug | `build/debug_app/` |
| Linux Release | `build/GuiTeXChord_vX_YY_unix/` |
| Linux→Win Cross | `build/GuiTeXChord_vX_YY_win_cross/` |

## Project structure

```
GuiTeXChord/
├── CMakeLists.txt
├── check_os.cmake
├── README.md
├── win32/
│   ├── win32.cmake
│   └── SetupQt.cmake
├── unix/
│   ├── unix_native.cmake
│   ├── unix_cross.cmake
│   ├── UnixQt.cmake
│   ├── mingw-w64-toolchain.cmake
│   ├── config.sh
│   ├── build.sh
│   └── AppImageBuilder.yml
├── resources/
│   ├── resources.qrc
│   ├── help_inline_en.md
│   ├── help_inline_de.md
│   ├── help_dialog_en.md
│   └── help_dialog_de.md
└── src/
    ├── main.cpp
    ├── MainWindow.h/cpp
    ├── ChordWidget.h/cpp
    ├── ChordPreview.h/cpp
    ├── ChordData.h
    └── LatexGenerator.h/cpp
```

## Usage

1. Select tuning tab; choose Vertical or Horizontal orientation
2. Click fretboard cells: **○ Root → ● Tone → × Mute → (clear)**
3. Column 0 = open string / nut; columns 1–5 = fretted positions
4. Fret column headers show actual fret numbers based on Start fret
5. Set chord name, position, barré as needed
6. Export via buttons or keyboard shortcuts

## Keyboard shortcuts

| Action | Shortcut |
|---|---|
| Compile PDF | Ctrl+P |
| Save .tex | Ctrl+S |
| Copy TikZ | Ctrl+C |
| Reset | Ctrl+Z |

## Help text

The in-app help texts are Markdown files in `resources/`:
- `help_inline_*.md` – shown in the main window Help panel
- `help_dialog_*.md` – shown in About > Help dialog

Edit these files and recompile to update the help content.
