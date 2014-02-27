lessThan(QT_VERSION, 4.8) {
    error("QupZilla requires at least Qt 4.8!")
}

TEMPLATE = app
CONFIG  += qt thread warn_on release
LANGUAGE = C++

# support
QT += webkitwidgets
QT += webkit
QT += xml 
QT += network
QT += widgets
QT += printsupport 

INCLUDEPATH  += ./src
FORMS += src/forms/filteredgesbyweightdialog.ui \
	src/forms/webcrawlerdialog.ui \
	src/forms/datasetselectdialog.ui 

HEADERS += src/backgrcircle.h \
           src/graphicswidget.h \
           src/edge.h \
           src/edgeweight.h \
           src/graph.h \
           src/htmlviewer.h \
           src/mainwindow.h \
           src/matrix.h \
           src/node.h \
           src/nodelabel.h \
           src/nodenumber.h \
           src/texteditor.h \
           src/vertex.h \ 
  	   src/parser.h \
	   src/filteredgesbyweightdialog.h \
	   src/webcrawlerdialog.h \
	   src/webcrawler.h \
	   src/datasetselectdialog.h

SOURCES += src/backgrcircle.cpp \
           src/graphicswidget.cpp \
           src/edge.cpp \
           src/edgeweight.cpp \
           src/graph.cpp \
           src/htmlviewer.cpp \
           src/main.cpp \
           src/mainwindow.cpp \
           src/matrix.cpp \
           src/node.cpp \
           src/nodelabel.cpp \
           src/nodenumber.cpp \
           src/texteditor.cpp \
           src/vertex.cpp \
           src/parser.cpp \
	   src/filteredgesbyweightdialog.cpp \
	   src/webcrawlerdialog.cpp \
	   src/webcrawler.cpp \
	   src/datasetselectdialog.cpp



# Extra optimization flags
QMAKE_CXXFLAGS += -msse -mfpmath=sse -ffast-math 

#LIBS    += @ac_libs@
INCLUDEPATH += /usr/share/qt4/include /usr/local/include /usr/include /usr/include/qt4 /usr/include/qt5 /usr/share/qt5/include 


RESOURCES = src/src.qrc
win32 {
     RC_FILE = src/icon.rc
}

macx:ICON = src/images/socnetv.icns


TRANSLATIONS    = translations/socnetv_es.ts 




