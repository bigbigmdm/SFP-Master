cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

project(SFP-Master LANGUAGES C CXX)

# Set the CMAKE_INSTALL_PREFIX to /usr if not specified
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_INSTALL_PREFIX
        "/usr/local"
        CACHE PATH "The default install prefix on macOS" FORCE)
    # Set Qt5 path for macOS
    set(CMAKE_PREFIX_PATH
        "${CMAKE_INSTALL_PREFIX}/opt/qt@5"
        CACHE PATH "Path to Qt5 on macOS" FORCE)
  else()
    set(CMAKE_INSTALL_PREFIX
        "/usr"
        CACHE PATH "The default install prefix" FORCE)
  endif()
endif()

# Set the CMAKE_INSTALL_BINDIR to /bin if not specified
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR
      "bin"
      CACHE PATH "user executables (bin)")
endif()

# Set the CMAKE_INSTALL_DATAROOTDIR to /share if not specified
if(NOT DEFINED CMAKE_INSTALL_DATAROOTDIR)
  set(CMAKE_INSTALL_DATAROOTDIR
      "share"
      CACHE PATH "read-only architecture-independent data root (share)")
endif()

# Set the CMAKE_INSTALL_LIBDIR to /lib if not specified
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR
      "/lib"
      CACHE PATH "read-only single-machine data (lib)")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Qt6 QUIET COMPONENTS Core Widgets)
if(TARGET Qt6::Core AND TARGET Qt6::Widgets)
  set(QT_VERSION_MAJOR 6)
  find_package(Qt6LinguistTools REQUIRED)
else()
  # If QT6 not found, using QT5
  find_package(Qt5 REQUIRED COMPONENTS Core Widgets LinguistTools)
  set(QT_VERSION_MAJOR 5)
endif()

find_package(LibUSB REQUIRED)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(UDEVDIR "/usr/lib/udev")
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr" OR CMAKE_INSTALL_PREFIX STREQUAL "/")
    # /usr and / install prefixes at treated by cmake GNUInstallDirs as synonym
    # for "system location". In this case we can look up the correct udevdir
    # using pkg-config. See:
    # https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html#special-cases
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PKGCONFIG_UDEV udev)
      if(PKGCONFIG_UDEV_FOUND)
        execute_process(
          COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=udevdir udev
          OUTPUT_VARIABLE PKGCONFIG_UDEVDIR
          OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(PKGCONFIG_UDEVDIR)
          file(TO_CMAKE_PATH "${PKGCONFIG_UDEVDIR}" UDEVDIR)
        endif()
      endif()
    endif()
  endif()
endif()

# Translation files
set(TS_FILES
    language/SFP-Master_hu_HU.ts language/SFP-Master_ru_RU.ts
    language/SFP-Master_it_IT.ts language/SFP-Master_pt_BR.ts
    language/SFP-Master_de_DE.ts language/SFP-Master_zh_Hans.ts)

if(QT_VERSION_MAJOR EQUAL 5)
  qt5_add_translation(QM_FILES ${TS_FILES})
else()
  qt_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(translations DEPENDS ${QM_FILES})

add_executable(
  ${PROJECT_NAME}
  main.cpp
  mainwindow.cpp
  qhexedit.cpp
  chunks.cpp
  commands.cpp
  dialogabout.cpp
  dialogpass.cpp
  ch341funcs.c
  mainwindow.h
  qhexedit.h
  chunks.h
  commands.h
  dialogpass.h
  ch341eeprom.h
  dialogabout.h
  mainwindow.ui
  resource.qrc
  ${QM_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(QT_VERSION_MAJOR EQUAL 5)
  target_link_libraries(${PROJECT_NAME} Qt5::Core)
  target_link_libraries(${PROJECT_NAME} Qt5::Widgets ${LibUSB_LIBRARIES})
else()
  target_link_libraries(${PROJECT_NAME} Qt::Core)
  target_link_libraries(${PROJECT_NAME} Qt::Widgets ${LibUSB_LIBRARIES})
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/SFP-Master.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pixmaps")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/SFP-Master.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/SFP-Master.1.gz"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/man/man1")
install(
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/other/io.github.bigbigmdm.sfp-master.metainfo.xml"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/index.html"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/screenshot.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/my_sfp_adapter_sch.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/my_sfp_adapter_3d.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/connection.jpg"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/undo64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/redo64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/read64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/write64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/password64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/save64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/open64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/exit64.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/sfp-master/html")
install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/sfp-master")
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/other/70-CH341.rules"
          DESTINATION "${UDEVDIR}/rules.d")
endif()
