# objects.mk

# Make 'include file' listing object files for Synchronet SBBSINST

# $Id: objects.mk,v 1.1 2003/01/21 10:10:18 rswindell Exp $

# LIBODIR, SBBSLIBODIR, SLASH, and OFILE must be pre-defined


OBJS	=	$(ODIR)$(SLASH)sbbsinst.$(OFILE)\
			$(ODIR)$(SLASH)conwrap.$(OFILE)\
			$(ODIR)$(SLASH)genwrap.$(OFILE)\
			$(ODIR)$(SLASH)dirwrap.$(OFILE)\
			$(ODIR)$(SLASH)uifcx.$(OFILE)\

