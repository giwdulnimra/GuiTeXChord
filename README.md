# ChordGen

A Qt6 desktop application for creating guitar chord diagrams as TikZ/LaTeX output.

## Features

- Interactive fretboard input (6–8 strings, multiple tunings)
- Per-string downtune (up to −9 semitones)
- Barré chord support (manual or auto-detected)
- Live preview (QPainter, no LaTeX round-trip needed)
- Export as standalone `.tex` file or compile directly to PDF via `pdflatex`
- Copy `tikzpicture` block to clipboard for embedding in existing LaTeX documents
- Constant output size regardless of whether chord name / position label is shown
- English / German UI

## Requirements

| | Windows | Linux |
|---|---|---|
| Compiler | MinGW-w64 (via Qt installer) | GCC / Clang |
| CMake | ≥ 3.20 | ≥ 3.20 |
| Qt | 6.11+ (C:/Qt/…) | 6.x via apt |
| LaTeX | MiKTeX / TeX Live (for PDF export) | `texlive-pictures` |

## Building

### Windows

```bat
cmake -B build\debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build\debug

cmake -B build\release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build\release
```

Output: `build\debug_app\` and `build\ChordGen_vX_YY_win\`

Qt DLLs are deployed automatically by `windeployqt6` in Release mode.

### Linux (native or WSL2)

```bash
# First time only – install dependencies:
chmod +x unix/config.sh && ./unix/config.sh

# Build:
chmod +x unix/build.sh
./unix/build.sh                    # Release
./unix/build.sh --debug            # Debug
./unix/build.sh --appimage         # Release + AppImage
./unix/build.sh --wsl              # WSL2 variant (uses unix/unix_wsl.cmake)
./unix/build.sh --qt-prefix ~/Qt/6.x.x/gcc_64   # custom Qt path
```

Output: `build/debug_app/` and `build/ChordGen_vX_YY_unix/`

### WSL2 (Linux binary from Windows)

WSL2 is a native Linux environment — there is no cross-compilation involved.
The resulting binary is a standard Linux ELF executable.

```bash
# Inside WSL2 terminal, from the repo root:
./unix/build.sh --wsl
```

See `unix/unix_wsl.cmake` for details and custom Qt path configuration.

## Project Structure

```
ChordGen/
├── CMakeLists.txt          # project definition + source list
├── check_os.cmake          # platform switch (win32 / unix)
├── README.md
├── win32/
│   ├── win32.cmake         # Windows build orchestration
│   └── SetupQt.cmake       # Qt setup + windeployqt6
├── unix/
│   ├── unix_native.cmake   # Linux native build orchestration
│   ├── unix_wsl.cmake      # WSL2 variant (see above)
│   ├── UnixQt.cmake        # Qt setup (apt-based)
│   ├── config.sh           # one-time dependency installer
│   ├── build.sh            # build + optional AppImage packaging
│   └── AppImageBuilder.yml # appimage-builder recipe
└── src/
    ├── main.cpp
    ├── MainWindow.h/cpp
    ├── ChordWidget.h/cpp
    ├── ChordPreview.h/cpp
    ├── ChordData.h
    └── LatexGenerator.h/cpp
```

## Usage

1. Select a tuning tab (6-/7-/8-string, standard or drop)
2. Click fretboard cells to set notes: **○ Root → ● Tone → × Mute → (clear)**
3. Column 0 = open string / nut position
4. Set chord name, position (start fret), barré as needed
5. Export via **Save .tex**, **Compile PDF**, or **Copy TikZ**

### Keyboard shortcuts

| Action | Shortcut |
|---|---|
| Save .tex | Ctrl+S |
| Compile PDF | Ctrl+P |
| Copy TikZ | Ctrl+C |
| Reset | Ctrl+Z |
