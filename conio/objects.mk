# smblib/objects.mk

# Make 'include file' listing object files for SMBLIB

# $Id: objects.mk,v 1.1 2004/09/12 12:06:47 deuce Exp $

# OBJODIR, DIRSEP, and OFILE must be pre-defined

OBJS	=	$(OBJODIR)$(DIRSEP)ansi_cio$(OFILE)\
			$(OBJODIR)$(DIRSEP)ciolib$(OFILE)\
			$(OBJODIR)$(DIRSEP)cterm$(OFILE)\
			$(OBJODIR)$(DIRSEP)mouse$(OFILE)
