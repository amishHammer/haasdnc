AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = dncd
dncd_SOURCES = main.c daemon.c log.c dnctty.c haastty.c moritty.c dnc_thread.c thread.c list.c lock.c
dncd_CXXFLAGS = @libconfuse_CFLAGS@
dncd_LDFLAGS = @libconfuse_LIBS@ -lpthread
