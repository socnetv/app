# SPDX-License-Identifier: BSD-3-Clause
#
# CMakeLists.txt - Build configuration for SocNetV (Social Network Visualizer)
#
# Last Update: Apr 2026
#
# This file defines the build system for SocNetV, an open-source social network 
# analysis and visualization application built with Qt6. It is designed to support 
# cross-platform builds on Linux, macOS, and Windows. Below is an overview of its 
# key components and functionality:
#
# 1. **Project Setup**:
#    - Sets the project name, version, and description.
#    - Configures the C++17 standard as required.
#    - Includes `GNUInstallDirs` for standardized installation paths.
#
# 2. **Source and Resource Management**:
#    - Defines lists for source files, UI forms, and resources (e.g., icons).
#    - Automatically handles Qt-specific features like `AUTOMOC`, `AUTOUIC`, and `AUTORCC`.
#    - Copies the license file to the build directory during configuration.
#
# 3. **Platform-Specific Configurations**:
#    - Sets platform-specific binary names and properties:
#      - **Windows**: Configures the executable as a GUI application and embeds metadata.
#      - **macOS**: Creates a macOS app bundle with proper metadata, icons, and `PkgInfo`.
#      - **Linux**: Installs binaries, icons, desktop entry, man pages, and documentation 
#        to standard filesystem locations.
#
# 4. **Qt Integration**:
#    - Locates required Qt6 modules (e.g., Core, Widgets, OpenGLWidgets, Charts).
#    - Links the application with necessary Qt libraries.
#
# 5. **Compiler and Build Options**:
#    - Adds compiler warnings for GCC, Clang, and MSVC to ensure code quality.
#    - Configures include directories for header files.
#
# 6. **Installation and Packaging**:
#    - Defines installation rules for binaries, icons, desktop files, and documentation.
#    - Ensures compatibility with platform-specific installation standards.
#
# 7. **Debugging and Finalization**:
#    - Prints debug information during the configuration phase.
#    - Finalizes the Qt-specific setup for the executable.
#

cmake_minimum_required(VERSION 3.16) # 3.21 or 3.22 for better features and compatibility with modern tools.

option(BUILD_CLI "Build headless regression CLI" ON)

# ==============================================================================
# 1. Project Setup
# ==============================================================================
# Set our project name and version:
set(APP_NAME SocNetV)
set(APP_NAME_LOWER socnetv)

project(SocNetV 
    VERSION 3.5
    DESCRIPTION "SocNetV: Open-source social network analysis application based on Qt."
    HOMEPAGE_URL "https://socnetv.org"
    LANGUAGES CXX
)

# Set the C++ standard
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


# ==============================================================================
# 2. Source and Resource Management
# ==============================================================================
# Copy and rename our COPYING license file (Configure Time)

configure_file(${CMAKE_SOURCE_DIR}/COPYING
               ${CMAKE_BINARY_DIR}/LICENSE.txt
               COPYONLY)

set(ENGINE_SOURCES
    src/engine/graph_distance_progress_sink.cpp
    src/engine/distance_engine.cpp
)

set(GRAPH_SOURCES
    src/graph.cpp
    src/graph/util/graph_type_strings.cpp
    src/graph/core/graph_structure_metrics.cpp
    src/graph/core/graph_state_flags.cpp
    src/graph/core/graph_metadata.cpp
    src/graph/storage/graph_vertices.cpp
    src/graph/storage/graph_edges.cpp
    src/graph/io/graph_io.cpp
    src/graph/io/graph_parse_sink_graph.cpp
    src/graph/relations/graph_relations.cpp
    src/graph/filters/graph_edge_filters.cpp
    src/graph/filters/graph_node_filters.cpp
    src/graph/ui/graph_ui_facade.cpp
    src/graph/ui/graph_ui_prominence_distribution.cpp
    src/graph/ui/graph_canvas.cpp
    src/graph/ui/graph_selection.cpp
    src/graph/ui/graph_vertex_style.cpp
    src/graph/ui/graph_edge_style.cpp
    src/graph/distances/graph_distance_facade.cpp
    src/graph/distances/graph_distance_cache.cpp
    src/graph/centrality/graph_centrality.cpp
    src/graph/centrality/graph_prestige.cpp
    src/graph/prominence/graph_prominence_distribution.cpp
    src/graph/matrices/graph_matrix_adjacency.cpp
    src/graph/generators/graph_random_networks.cpp
    src/graph/crawler/graph_crawler.cpp
    src/graph/layouts/graph_layouts_basic.cpp
    src/graph/layouts/graph_layouts_force.cpp
    src/graph/reachability/graph_reachability_walks.cpp
    src/graph/cohesion/graph_cliques.cpp
    src/graph/clustering/graph_triad_census.cpp
    src/graph/clustering/graph_clustering_coefficients.cpp
    src/graph/clustering/graph_clustering_hierarchical.cpp
    src/graph/similarity/graph_similarity_matrices.cpp
    src/graph/reporting/graph_reports.cpp
    src/graph/reporting/graph_reports_settings.cpp
)


set(PARSER_SOURCES
    src/parser.cpp
    src/parser/parser_common.cpp 
    src/parser/parser_graphml.cpp 
    src/parser/parser_edgelist.cpp 
    src/parser/parser_adjacency.cpp 
    src/parser/parser_dl.cpp 
    src/parser/parser_dot.cpp 
    src/parser/parser_gml.cpp
    src/parser/parser_pajek.cpp
)

set(CLI_SOURCES
    src/tools/socnetv_cli.cpp
    src/tools/cli/cli_common.cpp
    src/tools/headless_graph_loader.cpp
    src/tools/cli/kernels/kernel_distance_v1.cpp
    src/tools/cli/kernels/kernel_reachability_v2.cpp
    src/tools/cli/kernels/kernel_walks_v3.cpp
    src/tools/cli/kernels/kernel_prominence_v4.cpp
    src/tools/cli/kernels/kernel_io_roundtrip_v5.cpp
    src/tools/cli/kernels/kernel_clustering_v6.cpp
)


# Define sources and headers
set (FORMS 
    src/forms/dialogfilteredgesbyweight.ui 
    src/forms/dialogfilternodesbycentrality.ui
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
    src/forms/dialogedgeedit.ui
    src/forms/dialognodefind.ui
    src/forms/dialogfilterbyattribute.ui
    src/forms/dialogedgedichotomization.ui
    src/forms/dialogexportpdf.ui 
    src/forms/dialogexportimage.ui
    # Other UI files
)

set(SOURCES
    src/main.cpp 
    src/mainwindow.cpp 
    src/texteditor.cpp
    ${ENGINE_SOURCES}
    ${GRAPH_SOURCES}
    ${PARSER_SOURCES}
    src/graphvertex.cpp
    src/matrix.cpp 
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
    src/forms/dialogfilternodesbycentrality.cpp
    src/forms/dialogfilteredgesbyweight.cpp
    src/forms/dialogfilterbyattribute.cpp
    src/forms/dialogedgedichotomization.cpp 
    src/forms/dialogwebcrawler.cpp 
    src/forms/dialogdatasetselect.cpp 
    src/forms/dialogpreviewfile.cpp 
    src/forms/dialognodeedit.cpp
    src/forms/dialogedgeedit.cpp
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
    src/images.qrc
    src/data.qrc
)


# List source files
message(STATUS "Source files to be compiled: ${SOURCES}")

# ==============================================================================
# 3. Platform-Specific Configurations
# ==============================================================================
# Set icons
set(ICON_FILE src/images/${APP_NAME_LOWER}.png)

if (APPLE)
    set(ICON_FILE src/images/${APP_NAME_LOWER}.icns)
endif()

if (WIN32)
    set(ICON_FILE src/images/${APP_NAME_LOWER}.ico)
endif()


# ==============================================================================
# 4. Qt Integration
# ==============================================================================
# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core OpenGLWidgets Gui Core5Compat Widgets PrintSupport Network Charts Svg Xml LinguistTools)
# find_package_handle_standard_args(Qt6 DEFAULT_MSG Qt6_FOUND)

# Check if Qt6 was found
if (NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please install Qt6 and set the CMAKE_PREFIX_PATH.")
endif()


set(SOCNETV_TS
    ${CMAKE_SOURCE_DIR}/translations/socnetv_de.ts
    ${CMAKE_SOURCE_DIR}/translations/socnetv_es.ts
)


# The MANUAL_FINALIZATION option allows for additional 
# configuration before finalization
qt_add_executable(${TARGET_NAME} 
    MANUAL_FINALIZATION  
    ${SOURCES}
    ${FORMS}
    ${RESOURCES}
    ${ICON_FILE}
)

qt_add_lrelease(${TARGET_NAME}
    TS_FILES ${SOCNETV_TS}
    QM_FILES_OUTPUT_VARIABLE SOCNETV_QM
)

# These are generated during the build
set_source_files_properties(${SOCNETV_QM} PROPERTIES GENERATED TRUE)

# Add include directories for headers (equivalent to INCLUDEPATH)
target_include_directories(${TARGET_NAME}  PUBLIC
                           "${CMAKE_SOURCE_DIR}/src"
                           )


# ==============================================================================
# 5. Compiler and Build Options
# ==============================================================================
# Set default target properties
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




# ==============================================================================
# 6. Installation and Packaging
# ==============================================================================

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

    install(FILES ${SOCNETV_QM}
        DESTINATION release/
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

    # Ensure PkgInfo is explicitly created in the app bundle
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND echo -n "APPL????" > "${CMAKE_BINARY_DIR}/${TARGET_NAME}.app/Contents/PkgInfo"
        COMMENT "🔍 Ensuring PkgInfo exists in the macOS app bundle"
    )

    # Install the main app bundle
    install(TARGETS ${TARGET_NAME}
        BUNDLE DESTINATION "${CMAKE_INSTALL_PREFIX}/Applications"
    )

    foreach(qm ${SOCNETV_QM})
        set_source_files_properties(${qm} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/translations"
        )
    endforeach()
    target_sources(${TARGET_NAME} PRIVATE ${SOCNETV_QM})

elseif(UNIX AND NOT APPLE)

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
    install(FILES ${SOCNETV_QM}
        DESTINATION "${CMAKE_INSTALL_DATADIR}/socnetv"
    )

    # Install documentation
    install(FILES README.md TODO AUTHORS
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/socnetv"
    )

    # Install license
    install(FILES COPYING
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/socnetv"
    )

    # Install upstream changelog with Debian-friendly name
    install(FILES CHANGELOG.md
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/socnetv"
        RENAME changelog
    )

endif()


# ==============================================================================
# 7. Debugging and Finalization
# ==============================================================================
# Print debug information

message(STATUS "Building ${PROJECT_NAME} (${TARGET_NAME}) ${PROJECT_VERSION}")
# NOTE: To develop/build in Visual Studio Code, do these steps:  
# 1. Install the extensions: Qt Extension Pack and CMake Tools 
# 2. Open Command Palette and select "Open User Settings". 
# 3. Search for "cmake.configureSettings", click on "Edit in settings.json" and in the file add a line with your 'CMAKE_PREFIX_PATH'. 
# i.e. "CMAKE_PREFIX_PATH": "/home/<user>/Qt/6.8.3/gcc_64"
message(STATUS "CMAKE_PREFIX_PATH: "  ${CMAKE_PREFIX_PATH})
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: "  ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_LIST_DIR: "  ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: "  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "CMAKE_MODULE_PATH: "  ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_INSTALL_DATADIR: "  ${CMAKE_INSTALL_DATADIR})
# The root directory for the installation. 
# Defaults to /usr/local on Unix systems and C:/Program Files/<Project> on Windows. 
# You can override this with -DCMAKE_INSTALL_PREFIX=<path>.
message(STATUS "CMAKE_INSTALL_PREFIX: "  ${CMAKE_INSTALL_PREFIX})



if(BUILD_CLI)
    set(CLI_TARGET socnetv-cli)

    qt_add_executable(${CLI_TARGET}
        MANUAL_FINALIZATION
        ${CLI_SOURCES}
        ${ENGINE_SOURCES}
        ${GRAPH_SOURCES}
        ${PARSER_SOURCES}
        src/graphvertex.cpp
        src/matrix.cpp
        src/webcrawler.cpp
    )

    target_include_directories(${CLI_TARGET} PUBLIC
        "${CMAKE_SOURCE_DIR}/src"
    )

    target_link_libraries(${CLI_TARGET} PRIVATE
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Network
        Qt6::Xml
        Qt6::Widgets
        Qt6::Gui
        Qt6::Charts
    )

    set_target_properties(${CLI_TARGET} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
    )

    qt_finalize_executable(${CLI_TARGET})
endif()


# Finalize the executable
qt_finalize_executable(${TARGET_NAME})
