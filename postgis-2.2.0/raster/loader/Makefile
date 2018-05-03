#############################################################################
#
# Copyright (c) 2011 Regents of the University of California
#   <bkpark@ucdavis.edu>
# Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#############################################################################

# Set CFLAGS after PGXS, otherwise it will get overwritten with the PGXS
# version which is not what we want. 
CC=gcc
top_builddir = ../..
SHELL = /bin/sh
INSTALL = $(SHELL) ../../install-sh
LIBTOOL = $(SHELL) $(top_builddir)/libtool

# Filenames with extension as determined by the OS
RASTER2PGSQL=raster2pgsql

# PostgreSQL executable directory
PGSQL_BINDIR=/usr/local/Cellar/postgresql/9.4.5/bin

RT_CORE=../rt_core

LIBLWGEOM_LDFLAGS=../../liblwgeom/liblwgeom.la
LIBLWGEOM_CFLAGS=-I../../liblwgeom
LIBGDAL_CFLAGS=-I/usr/local/Cellar/gdal/1.11.3/include
LIBGDAL_LDFLAGS=-L/usr/local/Cellar/gdal/1.11.3/lib -lgdal
LIBGDAL_DEPLIBS_LDFLAGS=
PROJ_CFLAGS=-I/usr/local/Cellar/proj/4.9.2//include
GEOS_CFLAGS=-I/usr/local/Cellar/geos/3.4.2/include
GEOS_LDFLAGS=-L/usr/local/Cellar/geos/3.4.2/lib -lgeos_c

RTCORE_CFLAGS=-I$(RT_CORE)
RTCORE_LDFLAGS=$(RT_CORE)/librtcore.a

# GetText includes and libraries
GETTEXT_CFLAGS = 
GETTEXT_LDFLAGS =  

# iconv flags
ICONV_LDFLAGS= -liconv
ICONV_CFLAGS=

CFLAGS = \
	-g -O2  -fno-common -DPIC  -Wall -Wmissing-prototypes \
	$(RTCORE_CFLAGS) \
	$(LIBLWGEOM_CFLAGS) \
	$(PROJ_CFLAGS) \
	$(LIBGDAL_CFLAGS) \
	$(GEOS_CFLAGS) \
	$(GETTEXT_CFLAGS) \
	$(ICONV_CFLAGS)

LDFLAGS = \
	 \
	$(LIBLWGEOM_LDFLAGS) \
	$(LIBGDAL_LDFLAGS) \
	$(LIBGDAL_DEPLIBS_LDFLAGS) \
	$(GEOS_LDFLAGS) \
	$(GETTEXT_LDFLAGS) \
	$(ICONV_LDFLAGS) \
	-lm

all: $(RASTER2PGSQL)

raster2pgsql.o: raster2pgsql.c
	$(CC) $(CFLAGS) -c $<

$(RASTER2PGSQL): $(RT_CORE)/librtcore.a raster2pgsql.o
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

installdir:
	@mkdir -p $(DESTDIR)$(PGSQL_BINDIR)

install: installdir
	$(LIBTOOL) --mode=install $(INSTALL) $(RASTER2PGSQL) "$(DESTDIR)$(PGSQL_BINDIR)/$(RASTER2PGSQL)"

uninstall:
	$(LIBTOOL) --mode=uninstall $(RM) "$(DESTDIR)$(PGSQL_BINDIR)/$(RASTER2PGSQL)"

$(RT_CORE)/librtcore.a:
	$(MAKE) -C ../rt_core

clean:
	rm -rf .libs
	rm -f *.o $(RASTER2PGSQL)

distclean: clean
	rm -f Makefile
