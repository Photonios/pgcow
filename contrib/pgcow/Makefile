# contrib/pgcow/Makefile

EXTENSION = pgcow
PROGRAM = pgcow-initdb
PGFILEDESC = "pgcow - bringing copy-on-write technology to postgresql"

MODULE_big = pgcow
OBJS = \
	pgcow-initdb.o \
	src/fs.o \
	src/zfs/dataset.o \
	src/zfs/error.o \
	src/postgres/data_directory.o \
	$(WIN32RES)

PG_CPPFLAGS = \
	-fPIC \
	-static-libgcc \
	-static-libstdc++ \
	-I./inc \
	-I./third-party/spdlog/include \
	-I./third-party/cxxopts/include \
	-std=c++17 \
	$(shell pkg-config libzfs --cflags | sed 's/-I/-isystem/g')

# we set CC to g++, so CFLAGS == CPPFLAGS
PG_CFLAGS = $(PG_CPPFLAGS)

PG_LIBS = $(shell pkg-config libzfs --libs)

src/%.o:
	$(CXX) $(CPPFLAGS) -c $(@:.o=.cpp) -o $@

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pgcow
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# pgxs doesn't know how to compile c++, let's trick
# it by telling it g++ is a C compiler
CC = g++-9
CXX = g++-9

maintainer-clean:
	find . -type f -name "*.o" -exec rm -f {} \;
	find . -type f -name "*.so" -exec rm -f {} \;
