# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.2 2004/03/11 06:26:00 deuce Exp $

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST	= $(EXEODIR)$(SLASH)wraptest$(EXEFILE) 

all: $(EXEODIR) $(LIBODIR) $(WRAPTEST)
