cmake_minimum_required(VERSION 3.20)
project(ChordGen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Version ───────────────────────────────────────────────────────────────────
if(${PROJECT_VERSION_MINOR} LESS 10)
    set(DISP_MINOR "0${PROJECT_VERSION_MINOR}")
else()
    set(DISP_MINOR "${PROJECT_VERSION_MINOR}")
endif()
if(${PROJECT_VERSION_PATCH} LESS 10)
    set(DISP_PATCH "0${PROJECT_VERSION_PATCH}")
else()
    set(DISP_PATCH "${PROJECT_VERSION_PATCH}")
endif()

set(APPVERSION         "v${PROJECT_VERSION_MAJOR}_${DISP_MINOR}${DISP_PATCH}")
set(APPVERSION_DISPLAY "v${PROJECT_VERSION}")

# Macht die Versionen auch im C++ Code als Makros verfügbar
add_compile_definitions(
        CPP_APPVERSION="${APPVERSION}"
        CPP_APPVERSION_DISPLAY="${APPVERSION_DISPLAY}"
        #CPP_SEMVER="${PROJECT_VERSION}"
)

# ── SetupQt ───────────────────────────────────────────────────────────────────
include("${CMAKE_SOURCE_DIR}/SetupQt.cmake")

# ── Sources & Headers ─────────────────────────────────────────────────────────
set(SOURCES
        src/main.cpp
        src/MainWindow.cpp
        src/ChordWidget.cpp
        src/ChordPreview.cpp
        src/LatexGenerator.cpp
)

set(HEADERS
        src/MainWindow.h
        src/ChordWidget.h
        src/ChordPreview.h
        src/ChordData.h
        src/LatexGenerator.h
)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ── Target Konfiguration ──────────────────────────────────────────────────────
setup_qt_target(${PROJECT_NAME})