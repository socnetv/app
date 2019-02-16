lessThan(QT_VERSION, 5.0) {
    error("SocNetV requires at least Qt 5.0!")
}

TARGET = socnetv

TEMPLATE = app
#CONFIG  += qt thread warn_on release
CONFIG  += qt thread warn_on debug

LANGUAGE = C++

# support
QT += xml 
QT += network
QT += widgets
QT += printsupport 
QT += charts

INCLUDEPATH  += ./src
FORMS += src/forms/dialogfilteredgesbyweight.ui \
    src/forms/dialogsettings.ui \
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
    src/global.h

SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/texteditor.cpp \
    src/graph.cpp \
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
    src/forms/dialogexportimage.cpp


RESOURCES = src/src.qrc
    


win32 {
  RC_FILE = src/icon.rc
  TARGET = SocNetV
}

unix:!macx{

  isEmpty(PREFIX) {
        PREFIX = /usr/local
  }

  target.path = $$PREFIX/bin
  TARGET = socnetv

  pixmap.path = $$PREFIX/share/pixmaps
  pixmap.files = src/images/socnetv.png

  documentation.path = $$PREFIX/share/doc/socnetv
  documentation.files = manual

  desktop.path = $$PREFIX/share/applications
  desktop.files = socnetv.desktop
  
  manpage.path = $$PREFIX/share/man/man1
  manpage.files = man/socnetv.1.gz

  translations.path = $$PREFIX/share/socnetv
  translations.files = translations

  doc.path = $$PREFIX/share/doc/socnetv
  doc.files = license changelog.gz NEWS README.md TODO COPYING AUTHORS INSTALL

  INSTALLS += pixmap documentation manpage translations doc desktop

}

macx {
  ICON = src/images/socnetv.icns
  TARGET = SocNetV
}


INSTALLS += target


TRANSLATIONS    = translations/socnetv_es.ts \ 
		  translations/socnetv_el.ts 




