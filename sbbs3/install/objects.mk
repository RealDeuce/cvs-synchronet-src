# objects.mk

# Make 'include file' listing object files for Synchronet SBBSINST

# $Id: objects.mk,v 1.4 2004/03/11 06:26:00 deuce Exp $

# LIBODIR, SBBSLIBODIR, SLASH, and OFILE must be pre-defined


OBJS	=	$(LIBODIR)$(SLASH)sbbsinst.$(OFILE)\
			$(LIBODIR)$(SLASH)sockwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)genwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)dirwrap.$(OFILE)\
			$(LIBODIR)$(SLASH)filewrap.$(OFILE)\
			$(LIBODIR)$(SLASH)httpio.$(OFILE)
