cmake_minimum_required(VERSION 3.20)
project(ChordGen VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(PART MINOR PATCH)
    if(PROJECT_VERSION_${PART} LESS 10)
        set(DISP_${PART} "0${PROJECT_VERSION_${PART}}")
    else()
        set(DISP_${PART} "${PROJECT_VERSION_${PART}}")
    endif()
endforeach()
set(APPVERSION         "v${PROJECT_VERSION_MAJOR}_${DISP_MINOR}${DISP_PATCH}")
set(APPVERSION_DISPLAY "v${PROJECT_VERSION}")
add_compile_definitions(
    CPP_APPVERSION="${APPVERSION}"
    CPP_APPVERSION_DISPLAY="${APPVERSION_DISPLAY}"
)

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
include("${CMAKE_SOURCE_DIR}/check_os.cmake")
setup_qt_target(${PROJECT_NAME})
