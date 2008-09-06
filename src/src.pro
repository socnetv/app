SOURCES += backgrcircle.cpp \
           graphicswidget.cpp \
           edge.cpp \
           edgeweight.cpp \
           graph.cpp \
           htmlviewer.cpp \
           main.cpp \
           mainwindow.cpp \
           matrix.cpp \
           node.cpp \
           nodelabel.cpp \
           nodenumber.cpp \
           texteditor.cpp \
           vertex.cpp \
 parser.cpp
HEADERS += backgrcircle.h \
           graphicswidget.h \
           edge.h \
           edgeweight.h \
           graph.h \
           htmlviewer.h \
           mainwindow.h \
           matrix.h \
           node.h \
           nodelabel.h \
           nodenumber.h \
           texteditor.h \
           vertex.h \
 parser.h
TEMPLATE = app
CONFIG += warn_on \
	  thread \
          qt
QT += webkit
TARGET = ../bin/socnetv
RESOURCES = src.qrc
