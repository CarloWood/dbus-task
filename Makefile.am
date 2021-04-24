AM_CPPFLAGS = -iquote $(top_builddir) -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

SUBDIRS =

noinst_LTLIBRARIES = libdbustask.la

SOURCES = \
    Connection.cxx \
    Connection.h \
    DBusConnection.cxx \
    DBusConnection.h \
    DBusMatchSignal.h \
    DBusMatchSignal.cxx \
    DBusMethodCall.cxx \
    DBusMethodCall.h \
    DBusObject.cxx \
    DBusObject.h \
    Error.cxx \
    Error.h \
    ErrorDomainManager.cxx \
    ErrorDomainManager.h \
    ErrorException.h \
    Message.cxx \
    Message.h \
\
    systemd_sd-bus.cxx \
    systemd_sd-bus.h

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
