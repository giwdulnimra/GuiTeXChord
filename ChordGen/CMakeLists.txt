cmake_minimum_required(VERSION 3.20)
project(ChordGen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SetupQt.cmake liegt außerhalb des Projekts – Pfad ggf. anpassen
# include("../SetupQt.cmake")   # <-- Pfad zu SetupQt.cmake eintragen

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(SOURCES
    main.cpp
    MainWindow.cpp
    ChordWidget.cpp
    ChordPreview.cpp
    LatexGenerator.cpp
)

set(HEADERS
    MainWindow.h
    ChordWidget.h
    ChordPreview.h
    ChordData.h
    LatexGenerator.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Statt setup_qt_target() aus SetupQt.cmake – hier manuell, damit das Projekt
# auch ohne externen cmake-Include buildbar ist. Wenn SetupQt.cmake eingebunden
# ist, diese Zeilen durch setup_qt_target(${PROJECT_NAME}) ersetzen.
set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
