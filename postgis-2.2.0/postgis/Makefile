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

POSTGIS_PGSQL_VERSION=94
MODULE_big=postgis-2.2
MODULEDIR=contrib/$(MODULE_big)

# Files to be copied to the contrib/ directory
SQL_built=postgis.sql uninstall_postgis.sql postgis_upgrade.sql  legacy.sql uninstall_legacy.sql legacy_minimal.sql legacy_gist.sql 
DATA=../spatial_ref_sys.sql

# SQL objects (files requiring pre-processing)
SQL_objs=postgis.sql legacy.sql legacy_minimal.sql

GEOM_BACKEND_OBJ = lwgeom_geos.o
SFCGAL_BACKEND_OBJ = lwgeom_sfcgal.o

ifeq (sfcgal,sfcgal)
DATA_built=$(SQL_built) sfcgal.sql sfcgal_upgrade.sql uninstall_sfcgal.sql
SQL_OBJS=$(SQL_objs) sfcgal.sql
BACKEND_OBJ=$(GEOM_BACKEND_OBJ) $(SFCGAL_BACKEND_OBJ)
else
BACKEND_OBJ=$(GEOM_BACKEND_OBJ)
DATA_built=$(SQL_built)
SQL_OBJS=$(SQL_objs)
endif



# SQL preprocessor
SQLPP = /usr/bin/cpp -traditional-cpp -w -P

# PostgreSQL objects
PG_OBJS= \
	postgis_module.o \
	lwgeom_accum.o \
	lwgeom_spheroid.o \
	lwgeom_ogc.o \
	lwgeom_functions_analytic.o \
	lwgeom_inout.o \
	lwgeom_functions_basic.o \
	lwgeom_btree.o \
	lwgeom_box.o \
	lwgeom_box3d.o \
	$(BACKEND_OBJ) \
	lwgeom_backend_api.o \
	lwgeom_geos_prepared.o \
	lwgeom_geos_clean.o \
	lwgeom_geos_relatematch.o \
	lwgeom_export.o \
	lwgeom_in_gml.o \
	lwgeom_in_kml.o \
	lwgeom_in_geohash.o \
	lwgeom_in_geojson.o \
	lwgeom_in_encoded_polyline.o \
	lwgeom_triggers.o \
	lwgeom_dump.o \
	lwgeom_dumppoints.o \
	lwgeom_functions_lrs.o \
	lwgeom_functions_temporal.o \
	long_xact.o \
	lwgeom_sqlmm.o \
	lwgeom_rtree.o \
	lwgeom_transform.o \
	gserialized_typmod.o \
	gserialized_gist_2d.o \
	gserialized_gist_nd.o \
	gserialized_estimate.o \
	geography_inout.o \
	geography_btree.o \
	geography_measurement.o \
	geography_measurement_trees.o \
	geometry_inout.o

# Objects to build using PGXS
OBJS=$(PG_OBJS)

# Libraries to link into the module (proj, geos)
#
# Note: we specify liblwgeom.a directly in SHLIB_LINK rather than using
# -L... -l options to prevent issues with some platforms trying to link
# to an existing liblwgeom.so in the PostgreSQL $libdir supplied by an
# older version of PostGIS, rather than with the static liblwgeom.a
# supplied with newer versions of PostGIS
PG_CPPFLAGS += -I../liblwgeom -I../libpgcommon  -I/usr/local/Cellar/geos/3.4.2/include -I/usr/local/Cellar/proj/4.9.2//include -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/libxml2 -I/usr/local/include -DHAVE_SFCGAL    -fPIC
SHLIB_LINK_F = ../libpgcommon/libpgcommon.a ../liblwgeom/.libs/liblwgeom.a  -L/usr/local/Cellar/geos/3.4.2/lib -L/usr/local/Cellar/proj/4.9.2//lib -lgeos_c -lproj  -L/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/lib -lxml2 -lz -lpthread -licucore -lm -L/usr/local/lib -lSFCGAL  

# Add SFCGAL Flags if defined
ifeq (sfcgal,sfcgal)
PG_CPPFLAGS += -I/usr/local/include -DHAVE_SFCGAL
SHLIB_LINK_F += -L/usr/local/lib -lSFCGAL
endif

# Extra files to remove during 'make clean'
EXTRA_CLEAN=$(SQL_OBJS) \
	uninstall_postgis.sql \
	uninstall_legacy.sql \
	uninstall_sfcgal.sql \
	postgis_upgrade.sql.in \
	postgis_upgrade.sql \
	sfcgal_upgrade.sql.in \
	sfcgal_upgrade.sql 

# PGXS information
PG_CONFIG = /usr/local/bin/pg_config
PGXS := /usr/local/Cellar/postgresql/9.4.5/lib/pgxs/src/makefiles/pgxs.mk
# NO_TEMP_INSTALL is a workaround for a 9.5dev bug. See:
# http://www.postgresql.org/message-id/CAB7nPqTsR5o3g-fBi6jbsVdhfPiLFWQ_0cGU5=94Rv_8W3qvFA@mail.gmail.com
NO_TEMP_INSTALL=yes
include $(PGXS)

# Set PERL _after_ the include of PGXS
PERL=/usr/bin/perl

# This is to workaround a bug in PGXS 8.4 win32 link line,
# see http://trac.osgeo.org/postgis/ticket/1158#comment:57
SHLIB_LINK := $(SHLIB_LINK_F) $(SHLIB_LINK)

# If REGRESS=1 passed as a parameter, change the default install paths
# so that no prefix is included. This allows us to relocate to a temporary
# directory for regression testing.
ifeq ($(REGRESS),1)
        bindir=/bin
        pkglibdir=/lib
        datadir=/share
        datamoduledir=contrib/postgis
endif

# Make all PostGIS objects depend upon liblwgeom, so that if an underlying
# change is made, a PostGIS rebuild is triggered.
#
# Also they are all dependent on postgis_config.h
# and thus postgis_svn_revision.h
#
$(PG_OBJS): ../liblwgeom/.libs/liblwgeom.a ../libpgcommon/libpgcommon.a ../postgis_config.h ../postgis_svn_revision.h

../postgis_svn_revision.h:
	$(MAKE) -C .. postgis_svn_revision.h

../liblwgeom/.libs/liblwgeom.a:
	$(MAKE) -C ../liblwgeom liblwgeom.la

../libpgcommon/libpgcommon.a:
	$(MAKE) -C ../libpgcommon libpgcommon.a

# Borrow the $libdir substitution from PGXS but customise by running the preprocessor
# and adding the version number
%.sql: %.sql.in
	$(SQLPP) -I../libpgcommon $< | grep -v '^#' | \
	$(PERL) -lpe "s'MODULE_PATHNAME'\$$libdir/postgis-2.2'g" > $@

postgis_upgrade.sql.in: postgis.sql ../utils/postgis_proc_upgrade.pl
	$(PERL) ../utils/postgis_proc_upgrade.pl $< UNUSED > $@

postgis_upgrade.sql: postgis_drop_before.sql postgis_upgrade.sql.in postgis_drop_after.sql
	echo "BEGIN;" > $@
	cat $^ >> $@
	echo "COMMIT;" >> $@

sfcgal_upgrade.sql.in: sfcgal.sql ../utils/postgis_proc_upgrade.pl 
	$(PERL) ../utils/postgis_proc_upgrade.pl $< UNUSED > $@

sfcgal_upgrade.sql: sfcgal_upgrade.sql.in postgis_drop_after.sql
	cat $^ > $@

# SQL objects are also dependent on postgis_config.h for PostgreSQL version
$(SQL_OBJS): ../postgis_config.h ../postgis_svn_revision.h

#postgis.sql.in: sqldefines.h long_xact.sql.in.c geography.sql.in.c

uninstall_postgis.sql: postgis.sql ../utils/create_undef.pl
	$(PERL) ../utils/create_undef.pl $< $(POSTGIS_PGSQL_VERSION) > $@

uninstall_legacy.sql: legacy.sql ../utils/create_undef.pl
	$(PERL) ../utils/create_undef.pl $< $(POSTGIS_PGSQL_VERSION) > $@

uninstall_sfcgal.sql: sfcgal.sql ../utils/create_undef.pl
	$(PERL) ../utils/create_undef.pl $< $(POSTGIS_PGSQL_VERSION) > $@

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
