# objects.mk

# Make 'include file' listing object files for Synchronet SBBSINST

# $Id: objects.mk,v 1.2 2003/01/25 01:42:28 deuce Exp $

# LIBODIR, SBBSLIBODIR, SLASH, and OFILE must be pre-defined


OBJS	=	$(ODIR)$(SLASH)sbbsinst.$(OFILE)\
			$(ODIR)$(SLASH)conwrap.$(OFILE)\
			$(ODIR)$(SLASH)genwrap.$(OFILE)\
			$(ODIR)$(SLASH)dirwrap.$(OFILE)\
			$(ODIR)$(SLASH)ftpio.$(OFILE)\
			$(ODIR)$(SLASH)uifcx.$(OFILE)

