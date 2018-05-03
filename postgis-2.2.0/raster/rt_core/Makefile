#############################################################################
#
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

AR = ar rs

CC = gcc
LIBLWGEOM_LDFLAGS = ../../liblwgeom/.libs/liblwgeom.a
LIBLWGEOM_CFLAGS = -I../../liblwgeom
LIBGDAL_CFLAGS = -I/usr/local/Cellar/gdal/1.11.3/include
LIBGDAL_LDFLAGS = -L/usr/local/Cellar/gdal/1.11.3/lib -lgdal
GEOS_LDFLAGS = -L/usr/local/Cellar/geos/3.4.2/lib -lgeos_c
PROJ_LDFLAGS = -L/usr/local/Cellar/proj/4.9.2//lib -lproj
LDFLAGS = $(LIBLWGEOM_LDFLAGS) $(LIBGDAL_LDFLAGS) $(PROJ_LDFLAGS) $(GEOS_LDFLAGS)
CFLAGS = -g -O2  -fno-common -DPIC  -Wall -Wmissing-prototypes $(LIBLWGEOM_CFLAGS) $(LIBGDAL_CFLAGS) -I/usr/local/Cellar/proj/4.9.2//include -I/usr/local/Cellar/geos/3.4.2/include

# Standalone RTCORE objects
RT_OBJS = \
	rt_util.o \
	rt_spatial_relationship.o \
	rt_mapalgebra.o \
	rt_geometry.o \
	rt_statistics.o \
	rt_pixel.o \
	rt_warp.o \
	rt_band.o \
	rt_raster.o \
	rt_serialize.o \
	rt_wkb.o \
	rt_context.o

RT_LIB = librtcore.a
RT_HEADERS = \
	rt_serialize.h \
	librtcore.h \
	librtcore_internal.h \
	../raster_config.h

all: $(RT_LIB)

$(RT_OBJS): $(RT_HEADERS)

clean:
	rm -f $(RT_OBJS)
	rm -f $(RT_LIB)

distclean: clean
	rm -f Makefile

librtcore.a: $(RT_OBJS)
	$(AR) $(RT_LIB) $(RT_OBJS)
