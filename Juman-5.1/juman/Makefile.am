bin_PROGRAMS = juman

INCLUDES = -I../lib -I@ICU_INCLUDE_DIR@

juman_SOURCES = client.c icu_conv.c juman.c server.c
juman_LDADD = $(top_builddir)/lib/libjuman.la \
	@ICU_LIB_DIR@/cygicuuc.dll \
	@ICU_LIB_DIR@/cygicuin.dll @NET_LIBS@
