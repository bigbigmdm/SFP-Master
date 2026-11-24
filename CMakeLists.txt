cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

project(SFP-Master LANGUAGES C CXX)

# --- Force /usr prefix on Linux by default ---
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Default install prefix for Linux" FORCE)
    endif()
endif()

# Standard cross-platform installation paths (bin, share, etc.)
include(GNUInstallDirs)

# --- Automatic path detection on macOS (Homebrew / MacPorts) ---
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    if(EXISTS "/opt/local")
        # MacPorts
        list(APPEND CMAKE_PREFIX_PATH "/opt/local")
        list(APPEND PKG_CONFIG_PATH "/opt/local/lib/pkgconfig")
        message(STATUS "Configured for MacPorts at /opt/local")
    elseif(EXISTS "/opt/homebrew")
        # Homebrew Apple Silicon (M1/M2/M3/M4)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
        list(APPEND PKG_CONFIG_PATH "/opt/homebrew/lib/pkgconfig")
        message(STATUS "Configured for Homebrew Apple Silicon at /opt/homebrew")
    elseif(EXISTS "/usr/local/opt")
        # Homebrew Intel
        list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt")
        message(STATUS "Configured for Homebrew Intel at /usr/local")
    endif()
endif()

include(FindPkgConfig)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_INSTALL_DO_STRIP FALSE)
endif()

# Set the CMAKE_INSTALL_BINDIR to /bin if not specified
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()

# Set the CMAKE_INSTALL_LIBDIR to /lib if not specified
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "/lib" CACHE PATH "read-only single-machine data (lib)")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Option to force using Qt5
option(FORCE_QT5 "Force using Qt5 instead of Qt6" OFF)

if(FORCE_QT5)
    message(STATUS "Forced Qt5 mode")
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets LinguistTools)
    set(QT_VERSION_MAJOR 5)
else()
    find_package(Qt6 QUIET COMPONENTS Core Widgets)
    if(TARGET Qt6::Core AND TARGET Qt6::Widgets)
        message(STATUS "Using Qt6")
        set(QT_VERSION_MAJOR 6)
        find_package(Qt6LinguistTools REQUIRED)
    else()
        message(STATUS "Qt6 not found, using Qt5")
        find_package(Qt5 REQUIRED COMPONENTS Core Widgets LinguistTools)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()

# Udev rules configuration for Linux system installations
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(UDEVDIR "/usr/lib/udev")
    if (CMAKE_INSTALL_PREFIX STREQUAL "/usr" OR CMAKE_INSTALL_PREFIX STREQUAL "/" AND NOT CMAKE_CROSSCOMPILING)
        if (PKG_CONFIG_FOUND)
            pkg_check_modules(PKGCONFIG_UDEV udev)
            if (PKGCONFIG_UDEV_FOUND)
                execute_process(
                    COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=udevdir udev
                    OUTPUT_VARIABLE PKGCONFIG_UDEVDIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                if(PKGCONFIG_UDEVDIR)
                    file(TO_CMAKE_PATH "${PKGCONFIG_UDEVDIR}" UDEVDIR)
                endif()
            endif()
        endif()
    endif()
endif()

# --- Translation files ---
file(GLOB TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/language/*.ts"
)

# --- extract locals from supported translation files ---
set(BUNDLE_LOCALIZATIONS en)
foreach(ts ${TS_FILES})
    get_filename_component(_name "${ts}" NAME_WE)
    string(REGEX MATCH "[a-z][a-z](_[A-Z][A-Z])?$" _loc "${_name}")
    if(_loc)
        string(REPLACE "_" "-" _loc "${_loc}")   # de_DE → de-DE
        list(APPEND BUNDLE_LOCALIZATIONS "${_loc}")
    endif()
endforeach()
list(REMOVE_DUPLICATES BUNDLE_LOCALIZATIONS)
message(STATUS "BUNDLE_LOCALIZATIONS=${BUNDLE_LOCALIZATIONS}")

if (QT_VERSION_MAJOR EQUAL 5)
    qt5_add_translation(QM_FILES ${TS_FILES})
else()
    qt_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(translations DEPENDS ${QM_FILES})

# --- Executable target ---
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    ch34x_i2c.c
    qhexedit.cpp
    chunks.cpp
    commands.cpp
    dialogabout.cpp
    dialogpass.cpp
    mainwindow.h
    qhexedit.h
    ch34x_i2c.h
    chunks.h
    commands.h
    dialogpass.h
    dialogabout.h
    mainwindow.ui
    dialogpass.ui
    resource.qrc
    ${QM_FILES}
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    configure_macos_bundle(${PROJECT_NAME}
        "img/icons/apple/${PROJECT_NAME}.icns"
        "${BUNDLE_LOCALIZATIONS}"
    )    
    if(BUILD_ON_MACPORTS)
        set(BUNDLE_DEST "/Applications/MacPorts")
    else()
        set(BUNDLE_DEST "/Applications")
    endif()    
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})

if (QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Widgets
        PkgConfig::LIBUSB
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt::Core
        Qt::Widgets
        PkgConfig::LIBUSB
    )
endif()


# --- Installation rules ---
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION ${BUNDLE_DEST}               # Bundles into .app on macOS
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}     # Standard binary for Linux/Flatpak
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Automatically bundle database and .qm files inside .app/Contents/share/imsprog for local installation
    set_source_files_properties(${QM_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "share/imsprog")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/database/IMSProg.Dat" PROPERTIES MACOSX_PACKAGE_LOCATION "share/imsprog")    
else()
    # --- Specific rules for Linux / Flatpak ---
    
    # Installing different sizes icons in Linux
    foreach(SIZE 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512 scalable)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/img/icons/${SIZE}/"
                DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${SIZE}/apps"
                FILES_MATCHING PATTERN "*.png" PATTERN "*.svg")
    endforeach()

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/io.github.bigbigmdm.sfp-master.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/SFP-Master.1.gz" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/man/man1")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/io.github.bigbigmdm.sfp-master.metainfo.xml" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/index.html" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/screenshot.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/my_sfp_adapter_sch.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/my_sfp_adapter_3d.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/connection.jpg" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/undo64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/redo64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/read64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/write64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/password64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/save64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/open64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/exit64.png" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
    install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/sfp-master")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/70-CH341.rules" DESTINATION "${UDEVDIR}/rules.d")
endif()
