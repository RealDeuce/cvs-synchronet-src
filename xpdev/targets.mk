# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.4 2004/09/12 21:41:21 deuce Exp $

# ODIR, DIRSEP, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST	= $(EXEODIR)$(DIRSEP)wraptest$(EXEFILE)

XPDEV_LIB	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev$(LIBFILE)
XPDEV-MT_LIB	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev-mt$(LIBFILE)

libs: $(OBJODIR) $(LIBODIR) $(XPDEV_LIB) $(XPDEV-MT_LIB)

all: $(OBJODIR) $(MTOBJODIR) $(EXEODIR) $(LIBODIR) $(XPDEV_LIB) $(WRAPTEST)
