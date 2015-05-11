project:=edi
prefix ?= $(HOME)
doc:=$(project).pdf
system:=$(shell uname -s)
arch:=$(shell if ! uname -i 2>/dev/null; then uname -p; fi)
version:=$(shell echo `grep Version $(project).spec | cut -d ':' -f 2`)
release:=$(shell echo `grep Release $(project).spec | cut -d ':' -f 2`)
wd=$(shell pwd)
epicsflags=$(shell epics-ca-config --cflags)
epicslibs=$(shell epics-ca-config --libs)

cc:=g++ 
cc+= -pipe -g
dcc:=$(cc) -D_LARGEFILE_IGNORE $(epicsflags)
link:=$(cc) 
cc+=-D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 $(epicsflags)

QTDIR:=/usr/lib/qt4
PATH:=$(QTDIR)/bin:$(PATH)

.PHONY: all clean dist rpm

all: $(project)d

$(shell perl findqt.pl)
-include makefile.qt

svrmobjs:=server.hh
moc:=moc
$(svrmobjs:.hh=-moc.cc): %-moc.cc: %.hh
	$(moc) -o $@ $<

svrobjs=$(project)d.o pv.o \
	$(svrmobjs:.hh=.o) $(svrmobjs:.hh=-moc.o)
$(svrobjs:.o=.d): %.d: %.cc
	$(dcc) -o $@ $(QT_CXXFLAGS) $(QT_INCDEP) -MM -MT '$@ $(@:.d=.o)' $<
$(svrobjs):
	$(cc) -o $@ -c $< $(QT_CXXFLAGS) -Wno-sign-compare $(QT_INCPATH)
-include $(svrobjs:.o=.d)

$(project)d: $(svrobjs) 
	$(link) $(QT_LFLAGS) -o $@ $^ $(QT_LIBS) $(epicslibs)

clean:
	rm -f *.o $(project)d *~ *.d *-moc.cc $(project)*.tar.gz *.rpm

install: $(project)d 
	[ -d $(prefix)/sbin ] || mkdir -p $(prefix)/sbin
	/bin/cp $(project)d $(prefix)/sbin/

dist: $(project)-$(version).tar.gz

$(project)-$(version).tar.gz:
	make clean
	[ ! -d /tmp/$(project)-$(version) ] || rm -rf /tmp/$(project)-$(version)
	mkdir /tmp/$(project)-$(version)
	cp -r * /tmp/$(project)-$(version)/
	tar -C /tmp -czf $(project)-$(version).tar.gz $(project)-$(version)
	rm -rf /tmp/$(project)-$(version)

rpm: clean $(project)-$(version).tar.gz
	mkdir -p rpmbuild/{SOURCES,BUILD,SRPMS,RPMS}
	mv $(project)-$(version).tar.gz rpmbuild/SOURCES/
	rpmbuild -ba --define "_topdir $(wd)/rpmbuild" \
	--define "_tmppath $(wd)/tmp" $(project).spec
	mv rpmbuild/SOURCES/$(project)-$(version).tar.gz .
	mv rpmbuild/SRPMS/$(project)-$(version)-$(release).src.rpm .
	mv rpmbuild/RPMS/$(arch)/$(project)-$(version)-$(release).$(arch).rpm .
	rm -rf rpmbuild tmp
