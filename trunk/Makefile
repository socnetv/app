prefix  = /usr/local
qmake   = /usr/bin/qmake

name    = socnetv

headers =  src/backgrcircle.h \
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


sources = src/backgrcircle.cpp \
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


forms = 


all:	$(name)

$(name).mak:	$(name).pro
	@$(qmake) -o $(name).mak $(name).pro

$(name):	$(name).mak ${forms} $(sources) $(headers)
	@make -f $(name).mak

install:	$(name) src/images/$(name).png
	@install -d -v -m 0755 $(DESTDIR)$(prefix)/bin
	@install -d -v -m 0755 $(DESTDIR)$(prefix)/share/pixmaps
	@install -d -v -m 0755 $(DESTDIR)$(prefix)/share/applications
	@install -v -m 0755 $(name) $(DESTDIR)$(prefix)/bin/$(name)
	@install -v -m 0644 src/images/$(name).png $(DESTDIR)$(prefix)/share/pixmaps/$(name).png
	@install -v -m 0644 $(name).desktop $(DESTDIR)$(prefix)/share/applications/$(name).desktop
	mkdir -p $(DESTDIR)$(prefix)/share/doc/$(name)/manual
	@install -v -m 0644 doc/* $(DESTDIR)$(prefix)/share/doc/$(name)/manual/
	@install -v -m 0644 README NEWS COPYING AUTHORS TODO ChangeLog $(DESTDIR)$(prefix)/share/doc/$(name)/ 

uninstall:	$(DESTDIR)$(prefix)/bin/$(name)
	@rm -vf $(DESTDIR)$(prefix)/bin/$(name)
	@rm -vf $(DESTDIR)$(prefix)/share/pixmaps/$(name).png
	@rm -vf $(DESTDIR)$(prefix)/share/applications/$(name).desktop
	@rm -rf $(DESTDIR)$(prefix)/share/doc/$(name)
clean:	$(name).mak
	@make -f $(name).mak clean
	@rm -f $(name) $(name).mak
	@rm -rf *.cache *.log *.status
