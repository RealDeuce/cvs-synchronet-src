# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.3 2004/09/12 21:27:23 deuce Exp $

# ODIR, DIRSEP, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST	= $(EXEODIR)$(DIRSEP)wraptest$(EXEFILE)

XPDEV_LIB	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev$(LIBFILE)
XPDEV-MT_LIB	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev-mt$(LIBFILE)

libs: $(OBJODIR) $(LIBODIR) $(XPDEV_LIB) $(XPDEV-MT_LIB)

all: $(OBJODIR) $(OBJODIR)-mt $(EXEODIR) $(LIBODIR) $(XPDEV_LIB) $(WRAPTEST)
