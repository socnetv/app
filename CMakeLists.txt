# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16) # 3.21 or 3.22 for better features and compatibility with modern tools.

# Set our project name and version:
set(APP_NAME SocNetV)
set(APP_NAME_LOWER socnetv)

project(SocNetV 
    VERSION 3.2
    DESCRIPTION "SocNetV: Open-source social network analysis application based on Qt."
    HOMEPAGE_URL "https://socnetv.org"
    LANGUAGES CXX
)
# Set the CMAKE_CXX_STANDARD 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Some options here (UNUSED for now...):
option(DEV_MODE "Build in developer mode" OFF)


# Include GNUInstallDirs for standardized install locations
# Defines a set of variables for installation directories:
# CMAKE_INSTALL_PREFIX	Root directory for installation	/usr/local or C:/Program Files/<project>
# CMAKE_INSTALL_BINDIR	Directory for executable binaries	bin
# CMAKE_INSTALL_LIBDIR	Directory for libraries	lib or lib64
# CMAKE_INSTALL_INCLUDEDIR	Directory for header files	include
# CMAKE_INSTALL_DATAROOTDIR	Root directory for read-only data	share
# CMAKE_INSTALL_SYSCONFDIR	Directory for configuration files	etc
include(GNUInstallDirs)

# Define platform-specific binary names
if(WIN32)
    set(TARGET_NAME SocNetV)
elseif(APPLE)
    set(TARGET_NAME SocNetV)
else()
    set(TARGET_NAME socnetv)
endif()

# Override with -DCMAKE_INSTALL_PREFIX=/custom/path.
set(INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME})

# Debug information
message(STATUS "Building ${PROJECT_NAME} (${TARGET_NAME}) ${PROJECT_VERSION}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: "  ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_LIST_DIR: "  ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: "  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "CMAKE_MODULE_PATH: "  ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_INSTALL_DATADIR: "  ${CMAKE_INSTALL_DATADIR})
message(STATUS "INSTALL_DIR: "  ${INSTALL_DIR})


# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core OpenGLWidgets Gui Core5Compat Widgets PrintSupport Network Charts Svg Xml)
# find_package_handle_standard_args(Qt6 DEFAULT_MSG Qt6_FOUND)

# qt_standard_project_setup()

# Copy and rename our COPYING license file (Configure Time)
configure_file(${CMAKE_SOURCE_DIR}/COPYING
               ${CMAKE_BINARY_DIR}/LICENSE.txt
               COPYONLY)

# Define sources and headers
set (FORMS 
    src/forms/dialogfilteredgesbyweight.ui 
    src/forms/dialogsettings.ui 
    src/forms/dialogsysteminfo.ui 
    src/forms/dialogwebcrawler.ui 
    src/forms/dialogdatasetselect.ui 
    src/forms/dialograndsmallworld.ui 
    src/forms/dialograndscalefree.ui 
    src/forms/dialogranderdosrenyi.ui 
    src/forms/dialograndregular.ui 
    src/forms/dialograndlattice.ui 
    src/forms/dialogsimilaritypearson.ui 
    src/forms/dialogsimilaritymatches.ui 
    src/forms/dialogdissimilarities.ui 
    src/forms/dialogclusteringhierarchical.ui   
    src/forms/dialognodeedit.ui 
    src/forms/dialognodefind.ui 
    src/forms/dialogedgedichotomization.ui 
    src/forms/dialogexportpdf.ui 
    src/forms/dialogexportimage.ui
    # Other UI files
)

set(SOURCES
    src/main.cpp 
    src/mainwindow.cpp 
    src/texteditor.cpp 
    src/graph.cpp 
    src/graphvertex.cpp 
    src/matrix.cpp 
    src/parser.cpp 
    src/webcrawler.cpp 
    src/chart.cpp 
    src/graphicswidget.cpp 
    src/graphicsedge.cpp 
    src/graphicsedgeweight.cpp 
    src/graphicsedgelabel.cpp 
    src/graphicsguide.cpp 
    src/graphicsnode.cpp 
    src/graphicsnodelabel.cpp 
    src/graphicsnodenumber.cpp 
    src/forms/dialogfilteredgesbyweight.cpp 
    src/forms/dialogedgedichotomization.cpp 
    src/forms/dialogwebcrawler.cpp 
    src/forms/dialogdatasetselect.cpp 
    src/forms/dialogpreviewfile.cpp 
    src/forms/dialognodeedit.cpp 
    src/forms/dialogranderdosrenyi.cpp 
    src/forms/dialograndsmallworld.cpp 
    src/forms/dialograndregular.cpp 
    src/forms/dialograndscalefree.cpp 
    src/forms/dialogsettings.cpp 
    src/forms/dialogsimilaritypearson.cpp 
    src/forms/dialogsimilaritymatches.cpp  
    src/forms/dialogdissimilarities.cpp 
    src/forms/dialogclusteringhierarchical.cpp  
    src/forms/dialograndlattice.cpp 
    src/forms/dialognodefind.cpp 
    src/forms/dialogexportpdf.cpp 
    src/forms/dialogexportimage.cpp 
    src/forms/dialogsysteminfo.cpp
    # Add remaining source files here
)


set(RESOURCES
    src/src.qrc
)


# List source files
message(STATUS "Source files to be compiled: ${SOURCES}")



# Set icons.
set(ICON_FILE src/images/${APP_NAME_LOWER}.png)
if (APPLE)
    set(ICON_FILE src/images/${APP_NAME_LOWER}.icns)
endif()

if (WIN32)
    set(ICON_FILE src/images/${APP_NAME_LOWER}.ico)
endif()


# The MANUAL_FINALIZATION option allows for additional 
# configuration before finalization
qt_add_executable(${TARGET_NAME} 
    MANUAL_FINALIZATION  
    ${SOURCES}
    ${FORMS}
    ${RESOURCES}
    ${ICON_FILE}
)

# Add include directories for headers (equivalent to INCLUDEPATH)
target_include_directories(${TARGET_NAME}  PUBLIC
                           "${CMAKE_SOURCE_DIR}/src"
                           )


# Set DEFAULT Target Properties
set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)


# Add compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TARGET_NAME} PRIVATE /W4)
endif()


# Link libraries
target_link_libraries(${TARGET_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets  
    Qt6::Core5Compat 
    Qt6::PrintSupport 
    Qt6::Network 
    Qt6::Charts 
    Qt6::Svg 
    Qt6::Xml
)


# Add platform-specific properties
if(WIN32)

    # Add Windows-specific metadata and icon
    set_target_properties(${TARGET_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION}
    )
    
    # Embed version and metadata into the binary using a resource file
    target_sources(${TARGET_NAME} PRIVATE src/icon.rc)

    # Optionally copy the target to a specific directory after build
    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION release/  # Install to release/ folder
    )

elseif(APPLE AND NOT IOS)
    set_source_files_properties("${CMAKE_SOURCE_DIR}/src/images/socnetv.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/images/socnetv.icns")

    set_target_properties(${TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${TARGET_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.socnetv.app"
        MACOSX_BUNDLE_COPYRIGHT "© 2024 Dimitris Kalamaras. Licensed under GNU GPL v3."
        MACOSX_BUNDLE_ICON_FILE "socnetv.icns"
    )

    # Installation
    install(TARGETS ${TARGET_NAME}
        BUNDLE DESTINATION "${INSTALL_DIR}"
    )


elseif(LINUX)

    # Define the installation prefix (equivalent to qmake's PREFIX)
    # By default, this is /usr/local, but we set it to /usr for compatibility
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix")

    # Set the installation paths and files
    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"  # Binaries go to /usr/bin
#     LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
#     ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )

    # Install the pixmap (icon file)
    install(FILES src/images/socnetv.png
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
    )

    # Install the desktop entry file
    install(FILES socnetv.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )

    # Install the man page
    install(FILES man/socnetv.1
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/man/man1"
    )

    # Install the AppStream metadata file
    install(FILES socnetv.appdata.xml
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
    )

    # Install translations
    # install(FILES translations/socnetv_*.qm
    #     DESTINATION "${CMAKE_INSTALL_DATADIR}/socnetv"
    # )

    # Install documentation
    install(FILES changelog.gz NEWS README.md TODO COPYING AUTHORS INSTALL
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/socnetv"
    )

endif()



qt_finalize_executable(${TARGET_NAME})




