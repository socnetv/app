lessThan(QT_VERSION, 6.0) {
    error("SocNetV requires at least Qt 6.0!")
}

MY_TARGET_BUILD = release
#MY_TARGET_BUILD = debug

#ALLOW_WARNINGS = warn_on
ALLOW_WARNINGS = warn_off

TEMPLATE = app
CONFIG  += qt thread $${ALLOW_WARNINGS} $${MY_TARGET_BUILD}
CONFIG  += c++17
TARGET = socnetv
VERSION=3.3.1
LANGUAGE = C++


# add Qt module support
QT += core
QT += gui
QT += xml
QT += network
QT += widgets
QT += openglwidgets
QT += printsupport
QT += charts
QT += svg
QT += core5compat

INCLUDEPATH  += ./src

FORMS += src/forms/dialogfilteredgesbyweight.ui \
    src/forms/dialogfilternodesbycentrality.ui \
    src/forms/dialogsettings.ui \
    src/forms/dialogsysteminfo.ui \
    src/forms/dialogwebcrawler.ui \
    src/forms/dialogdatasetselect.ui \
    src/forms/dialograndsmallworld.ui \
    src/forms/dialograndscalefree.ui \
    src/forms/dialogranderdosrenyi.ui \
    src/forms/dialograndregular.ui \
    src/forms/dialograndlattice.ui \
    src/forms/dialogsimilaritypearson.ui \
    src/forms/dialogsimilaritymatches.ui \
    src/forms/dialogdissimilarities.ui \
    src/forms/dialogclusteringhierarchical.ui \
    src/forms/dialognodeedit.ui \
    src/forms/dialognodefind.ui \
    src/forms/dialogedgedichotomization.ui \
    src/forms/dialogexportpdf.ui \
    src/forms/dialogexportimage.ui

HEADERS += src/mainwindow.h \
    src/texteditor.h \
    src/graph.h \
    src/graphvertex.h \
    src/matrix.h \
    src/parser.h \
    src/webcrawler.h \
    src/chart.h \
    src/graphicswidget.h \
    src/graphicsedge.h \
    src/graphicsedgeweight.h \
    src/graphicsedgelabel.h \
    src/graphicsguide.h \
    src/graphicsnode.h \
    src/graphicsnodelabel.h \
    src/graphicsnodenumber.h \
    src/forms/dialogfilteredgesbyweight.h \
    src/forms/dialogfilternodesbycentrality.h \
    src/forms/dialogedgedichotomization.h \
    src/forms/dialogwebcrawler.h \
    src/forms/dialogdatasetselect.h \
    src/forms/dialogpreviewfile.h \
    src/forms/dialognodeedit.h \
    src/forms/dialogranderdosrenyi.h \
    src/forms/dialograndsmallworld.h \
    src/forms/dialograndscalefree.h \
    src/forms/dialograndregular.h \
    src/forms/dialogsettings.h \
    src/forms/dialogsimilaritypearson.h \
    src/forms/dialogsimilaritymatches.h \
    src/forms/dialogdissimilarities.h \
    src/forms/dialogclusteringhierarchical.h \
    src/forms/dialograndlattice.h \
    src/forms/dialognodefind.h \
    src/forms/dialogexportpdf.h \
    src/forms/dialogexportimage.h \
    src/forms/dialogsysteminfo.h \
    src/global.h

SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/texteditor.cpp \
    src/engine/graph_distance_progress_sink.cpp \
    src/engine/distance_engine.cpp \
    src/graph.cpp \
    src/graph/util/graph_type_strings.cpp \
    src/graph/core/graph_structure_metrics.cpp \
    src/graph/core/graph_state_flags.cpp \
    src/graph/core/graph_metadata.cpp \
    src/graph/storage/graph_vertices.cpp \
    src/graph/storage/graph_edges.cpp \
    src/graph/io/graph_io.cpp \
    src/graph/relations/graph_relations.cpp \
    src/graph/filters/graph_edge_filters.cpp \
    src/graph/filters/graph_node_filters.cpp \
    src/graph/ui/graph_ui_facade.cpp \
    src/graph/ui/graph_ui_prominence_distribution.cpp \
    src/graph/ui/graph_canvas.cpp \
    src/graph/ui/graph_selection.cpp \
    src/graph/ui/graph_vertex_style.cpp \
    src/graph/ui/graph_edge_style.cpp \
    src/graph/distances/graph_distance_facade.cpp \
    src/graph/distances/graph_distance_cache.cpp \
    src/graph/centrality/graph_centrality.cpp \
    src/graph/centrality/graph_prestige.cpp \
    src/graph/prominence/graph_prominence_distribution.cpp \
    src/graph/matrices/graph_matrix_adjacency.cpp \
    src/graph/generators/graph_random_networks.cpp \
    src/graph/crawler/graph_crawler.cpp \
    src/graph/layouts/graph_layouts_basic.cpp \
    src/graph/layouts/graph_layouts_force.cpp \
    src/graph/reachability/graph_reachability_walks.cpp \
    src/graph/cohesion/graph_cliques.cpp \
    src/graph/clustering/graph_triad_census.cpp \
    src/graph/clustering/graph_clustering_coefficients.cpp \
    src/graph/clustering/graph_clustering_hierarchical.cpp \
    src/graph/similarity/graph_similarity_matrices.cpp \
    src/graph/reporting/graph_reports.cpp \
    src/graph/reporting/graph_reports_settings.cpp \
    src/graphvertex.cpp \
    src/matrix.cpp \
    src/parser.cpp \
    src/webcrawler.cpp \
    src/chart.cpp \
    src/graphicswidget.cpp \
    src/graphicsedge.cpp \
    src/graphicsedgeweight.cpp \
    src/graphicsedgelabel.cpp \
    src/graphicsguide.cpp \
    src/graphicsnode.cpp \
    src/graphicsnodelabel.cpp \
    src/graphicsnodenumber.cpp \
    src/forms/dialogfilteredgesbyweight.cpp \
    src/forms/dialogfilternodesbycentrality.cpp \
    src/forms/dialogedgedichotomization.cpp \
    src/forms/dialogwebcrawler.cpp \
    src/forms/dialogdatasetselect.cpp \
    src/forms/dialogpreviewfile.cpp \
    src/forms/dialognodeedit.cpp \
    src/forms/dialogranderdosrenyi.cpp \
    src/forms/dialograndsmallworld.cpp \
    src/forms/dialograndregular.cpp \
    src/forms/dialograndscalefree.cpp \
    src/forms/dialogsettings.cpp \
    src/forms/dialogsimilaritypearson.cpp \
    src/forms/dialogsimilaritymatches.cpp \
    src/forms/dialogdissimilarities.cpp \
    src/forms/dialogclusteringhierarchical.cpp \
    src/forms/dialograndlattice.cpp \
    src/forms/dialognodefind.cpp \
    src/forms/dialogexportpdf.cpp \
    src/forms/dialogexportimage.cpp \
    src/forms/dialogsysteminfo.cpp


RESOURCES = src/images.qrc \
    src/data.qrc


# This is Windows only
win32 {
  VERSION = 3.3.1.1           # major.minor.patch.build
  VERSION_PE_HEADER = 3.3     # MSVC link.exe option /VERSION:x.y expects two numeric components (major.minor)

  #RC_FILE = src/icon.rc
  RC_ICONS = src/images/socnetv.ico
  # Company name
  QMAKE_TARGET_COMPANY = "socnetv.org"

  # Product name
  QMAKE_TARGET_PRODUCT = "Social Network Visualizer"

  # Document description
  QMAKE_TARGET_DESCRIPTION = "SocNetV: Open-source social network analysis application based on Qt."

  # Copyright Information
  QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2023 Dimitris B. Kalamaras. GPLv3."

  TARGET = SocNetV
  target.path = release/
}


win32:msvc {
    # Fixes msvc compile/linking error "unresolved external symbol WinMain referenced in function ..."
    # see https://stackoverflow.com/questions/39689162/qt-project-in-visual-studio-2015-unresolved-external-symbol-wwinmain
    QMAKE_LFLAGS += /ENTRY:mainCRTStartup
}


# This is Linux/Unix only
unix:!macx{

# No matter what PREFIX the user enters when
# executing qmake to create the Makefile,
# we always set it to be /usr because
# it simplifies the .travis CI and .deb creation
# The user may still install to a different folder
# with the command:
# make INSTALL_ROOT=<folder> install;

  PREFIX = /usr
  QMAKE_STRIP = :
  target.path = $${PREFIX}/bin
  TARGET = socnetv

  pixmap.path = $${PREFIX}/share/pixmaps
  pixmap.files = src/images/socnetv.png

  desktop.path = $${PREFIX}/share/applications
  desktop.files = socnetv.desktop

  manpage.path = $${PREFIX}/share/man/man1
  manpage.files = man/socnetv.1

  appstream.path = $${PREFIX}/share/metainfo
  appstream.files = socnetv.appdata.xml

  translations.path = $${PREFIX}/share/socnetv
  translations.files = translations/socnetv_*.qm

  doc.path = $${PREFIX}/share/doc/socnetv
  doc.files = CHANGELOG.md README.md TODO COPYING AUTHORS INSTALL

  INSTALLS += pixmap desktop manpage translations appstream

}

# This is macOS only
macx {
  ICON = src/images/socnetv.icns
  TARGET = SocNetV
  QMAKE_CXXFLAGS = -Wno-unused-variable -Wdeprecated-declarations
  QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wdeprecated-declarations

  # Build app for both x86_64 and arm64, see https://doc.qt.io/qt-6/macos.html#architectures
  QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
}


INSTALLS += target


TRANSLATIONS = translations/socnetv_es.ts \
               translations/socnetv_de.ts