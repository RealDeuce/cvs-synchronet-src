# objects.mk

# Make 'include file' listing object files for xpdev "wrappers"

# $Id: objects.mk,v 1.3 2004/03/11 06:26:00 deuce Exp $

# LIBODIR, SLASH, and OFILE must be pre-defined

OBJS = \
	$(LIBODIR)$(SLASH)genwrap.$(OFILE) \
	$(LIBODIR)$(SLASH)conwrap.$(OFILE) \
	$(LIBODIR)$(SLASH)dirwrap.$(OFILE) \
	$(LIBODIR)$(SLASH)filewrap.$(OFILE) \
	$(LIBODIR)$(SLASH)threadwrap.$(OFILE) \
	$(LIBODIR)$(SLASH)semwrap.$(OFILE)
