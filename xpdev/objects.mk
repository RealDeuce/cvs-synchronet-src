# objects.mk

# Make 'include file' listing object files for xpdev "wrappers"

# $Id: objects.mk,v 1.2 2003/05/02 03:49:00 deuce Exp $

# ODIR, SLASH, and OFILE must be pre-defined

OBJS = \
	$(ODIR)$(SLASH)genwrap.$(OFILE) \
	$(ODIR)$(SLASH)conwrap.$(OFILE) \
	$(ODIR)$(SLASH)dirwrap.$(OFILE) \
	$(ODIR)$(SLASH)filewrap.$(OFILE) \
	$(ODIR)$(SLASH)threadwrap.$(OFILE) \
	$(ODIR)$(SLASH)semwrap.$(OFILE)
