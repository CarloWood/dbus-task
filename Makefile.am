AM_CPPFLAGS = -iquote $(top_builddir) -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

SUBDIRS =

noinst_LTLIBRARIES = libdbustask.la

SOURCES = \
	DBusClient.h \
	DBusClient.cxx

libdbustask_la_SOURCES = ${SOURCES}
libdbustask_la_CXXFLAGS = @LIBCWD_R_FLAGS@
libdbustask_la_LIBADD = @LIBCWD_R_LIBS@

# --------------- Maintainer's Section

if MAINTAINER_MODE
distclean-local:
	-rm -f *.s *.ii

clean-local:
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
