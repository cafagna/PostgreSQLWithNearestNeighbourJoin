# **********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# * Copyright 2008 Mark Cave-Ayland
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

CC = gcc
CPPFLAGS =  -I/usr/local/Cellar/geos/3.4.2/include -I/usr/local/Cellar/proj/4.9.2//include -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/libxml2 -I/usr/local/include -DHAVE_SFCGAL   
CFLAGS = -g -O2  -fno-common -DPIC  -Wall -Wmissing-prototypes -I/usr/local/Cellar/geos/3.4.2/include -I/usr/local/Cellar/proj/4.9.2//include 
LDFLAGS =  -L/usr/local/Cellar/geos/3.4.2/lib -lgeos_c -L/usr/local/Cellar/proj/4.9.2//lib -lproj 
NUMERICFLAGS = 
top_builddir = ..
prefix = /Users/francescocafagna/Desktop/postgresFEEDBASE/contrib
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib
includedir = ${prefix}/include
SHELL = /bin/sh
INSTALL = $(SHELL) ../install-sh
LIBTOOL = $(SHELL) $(top_builddir)/libtool

SOVER = 2.2

IFACE_CURRENT = 4
IFACE_AGE = 2
IFACE_REV = 0

VERSION_INFO = $(IFACE_CURRENT):$(IFACE_REV):$(IFACE_AGE)


YACC=bison -y
LEX=flex

# Standalone LWGEOM objects
SA_OBJS = \
	stringbuffer.o \
	bytebuffer.o \
	measures.o \
	measures3d.o \
	box2d.o \
	ptarray.o \
	lwgeom_api.o \
	lwgeom.o \
	lwpoint.o \
	lwline.o \
	lwpoly.o \
	lwtriangle.o \
	lwmpoint.o \
	lwmline.o \
	lwmpoly.o \
	lwcollection.o \
	lwcircstring.o \
	lwcompound.o \
	lwcurvepoly.o \
	lwmcurve.o \
	lwmsurface.o \
	lwpsurface.o \
	lwtin.o \
	lwout_wkb.o \
	lwin_geojson.o \
	lwin_wkb.o \
	lwin_twkb.o \
	lwout_wkt.o \
	lwout_twkb.o \
	lwin_wkt_parse.o \
	lwin_wkt_lex.o \
	lwin_wkt.o \
	lwin_encoded_polyline.o \
	lwutil.o \
	lwhomogenize.o \
	lwalgorithm.o \
	lwstroke.o \
	lwlinearreferencing.o \
	lwprint.o \
	g_box.o \
	g_serialized.o \
	g_util.o \
	lwgeodetic.o \
	lwgeodetic_tree.o \
	lwtree.o \
	lwout_gml.o \
	lwout_kml.o \
	lwout_geojson.o \
	lwout_svg.o \
	lwout_x3d.o \
	lwout_encoded_polyline.o \
	lwgeom_debug.o \
	lwgeom_geos.o \
	lwgeom_geos_clean.o \
	lwgeom_geos_cluster.o \
	lwgeom_geos_node.o \
	lwgeom_geos_split.o \
	lwgeom_topo.o \
	lwgeom_transform.o \
	lwunionfind.o \
	effectivearea.o \
	varint.o

NM_OBJS = \
	lwspheroid.o

ifeq (sfcgal,sfcgal)
CFLAGS += -I/usr/local/include -DHAVE_SFCGAL
LDFLAGS += -L/usr/local/lib -lSFCGAL
SA_OBJS += lwgeom_sfcgal.o
endif

LDFLAGS += -no-undefined

LT_SA_OBJS = $(SA_OBJS:.o=.lo)
LT_NM_OBJS = $(NM_OBJS:.o=.lo)
LT_OBJS = $(LT_SA_OBJS) $(LT_NM_OBJS)

SA_HEADERS = \
	bytebuffer.h \
	effectivearea.h \
	liblwgeom.h \
	liblwgeom_internal.h \
	lwgeodetic.h \
	lwgeodetic_tree.h \
	liblwgeom_topo.h \
	liblwgeom_topo_internal.h \
	lwgeom_log.h \
	lwgeom_geos.h \
	lwgeom_log.h \
	lwgeom_sfcgal.h \
	lwin_wkt.h \
	lwin_wkt_parse.h \
	lwout_twkb.h \
	lwtree.h \
	measures3d.h \
	measures.h \
	stringbuffer.h \
	varint.h

all: liblwgeom.la

install: install-liblwgeom

uninstall: uninstall-liblwgeom

install-liblwgeom: liblwgeom.la
	$(LIBTOOL) --mode=install $(INSTALL) liblwgeom.la "$(DESTDIR)$(libdir)/liblwgeom.la"
	$(INSTALL) -m 0644 liblwgeom.h "$(DESTDIR)$(includedir)/liblwgeom.h"
	$(INSTALL) -m 0644 liblwgeom_topo.h "$(DESTDIR)$(includedir)/liblwgeom_topo.h"

uninstall-liblwgeom:
	$(LIBTOOL) --mode=uninstall rm -f "$(DESTDIR)$(libdir)/liblwgeom.la"
	$(LIBTOOL) --mode=uninstall rm -f "$(DESTDIR)$(includedir)/liblwgeom.h"
	$(LIBTOOL) --mode=uninstall rm -f "$(DESTDIR)$(includedir)/liblwgeom_topo.h"

# Make all objects depend upon postgis_config.h and postgis_svn_revision.h
$(LT_OBJS): ../postgis_config.h ../postgis_svn_revision.h $(SA_HEADERS)

../postgis_svn_revision.h:
	$(MAKE) -C .. postgis_svn_revision.h

#liblwgeom.a: $(SA_OBJS) $(NM_OBJS) $(SA_HEADERS)
#ar rs liblwgeom.a $(SA_OBJS) $(NM_OBJS)

liblwgeom.la: $(LT_OBJS)
	$(LIBTOOL) --tag=CC --mode=link $(CC) -rpath $(libdir) $(LT_OBJS) \
             -release $(SOVER) -version-info $(VERSION_INFO) $(LDFLAGS) -o $@

maintainer-clean: clean
	rm -f lwin_wkt_lex.c
	rm -f lwin_wkt_parse.h
	rm -f lwin_wkt_parse.c

clean:
	$(MAKE) -C cunit clean
	rm -f $(LT_OBJS) $(SA_OBJS) $(NM_OBJS)
	rm -f liblwgeom.la
	rm -rf .libs

distclean: clean
	$(MAKE) -C cunit distclean
	rm -f liblwgeom.h Makefile
	rm -f Makefile

check: liblwgeom.la
	$(MAKE) -C cunit check

# Command to build each of the .lo files
$(LT_SA_OBJS): %.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(LT_NM_OBJS): %.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) $(CFLAGS) $(NUMERICFLAGS) -c -o $@ $<

lwin_wkt_parse.c lwin_wkt_parse.h: lwin_wkt_parse.y
	$(YACC) -o'lwin_wkt_parse.c' -d lwin_wkt_parse.y

lwin_wkt_lex.c: lwin_wkt_lex.l
	$(LEX) -i lwin_wkt_lex.l

liblwgeom.h: liblwgeom.h.in
	cd .. && ./config.status
