# objects.mk

# Make 'include file' listing object files for xpdev "wrappers"

# $Id: objects.mk,v 1.1 2002/04/06 01:35:28 rswindell Exp $

# ODIR, SLASH, and OFILE must be pre-defined

OBJS = \
	$(ODIR)$(SLASH)genwrap.$(OFILE) \
	$(ODIR)$(SLASH)conwrap.$(OFILE) \
	$(ODIR)$(SLASH)dirwrap.$(OFILE) \
	$(ODIR)$(SLASH)filewrap.$(OFILE) \
	$(ODIR)$(SLASH)threadwrap.$(OFILE)
