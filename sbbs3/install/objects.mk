# objects.mk

# Make 'include file' listing object files for Synchronet SBBSINST

# $Id: objects.mk,v 1.3 2003/12/08 05:39:51 deuce Exp $

# LIBODIR, SBBSLIBODIR, SLASH, and OFILE must be pre-defined


OBJS	=	$(ODIR)$(SLASH)sbbsinst.$(OFILE)\
			$(ODIR)$(SLASH)sockwrap.$(OFILE)\
			$(ODIR)$(SLASH)genwrap.$(OFILE)\
			$(ODIR)$(SLASH)dirwrap.$(OFILE)\
			$(ODIR)$(SLASH)filewrap.$(OFILE)\
			$(ODIR)$(SLASH)ciowrap.$(OFILE)\
			$(ODIR)$(SLASH)httpio.$(OFILE)\
			$(ODIR)$(SLASH)uifcx.$(OFILE)
