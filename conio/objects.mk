# smblib/objects.mk

# Make 'include file' listing object files for SMBLIB

# $Id: objects.mk,v 1.2 2004/09/12 23:54:00 deuce Exp $

# OBJODIR, DIRSEP, and OFILE must be pre-defined

OBJS	=	$(MTOBJODIR)$(DIRSEP)ansi_cio$(OFILE)\
			$(MTOBJODIR)$(DIRSEP)ciolib$(OFILE)\
			$(MTOBJODIR)$(DIRSEP)cterm$(OFILE)\
			$(MTOBJODIR)$(DIRSEP)mouse$(OFILE)
