TEMPLATE = app
CONFIG  += qt thread warn_on release
LANGUAGE = C++

# support
QT += webkit
QT += xml

INCPATH += ./src
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
  	   src/parser.h

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
           src/parser.cpp






# Extra optimization flags
QMAKE_CXXFLAGS += -msse -mfpmath=sse -ffast-math 

#LIBS    += @ac_libs@
INCPATH += /usr/lib/qt4/include /usr/local/include /usr/include 


RESOURCES = src/src.qrc
win32 {
     RC_FILE = src/icon.rc
}

TRANSLATIONS    = translations/socnetv_es.ts 




