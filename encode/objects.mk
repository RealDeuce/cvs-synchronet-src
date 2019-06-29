# encode/objects.mk

# Make 'include file' listing object files for ENCODE LIB

# $Id: objects.mk,v 1.2 2019/06/29 00:05:46 rswindell Exp $

# OBJODIR, DIRSEP, and OFILE must be pre-defined

OBJS	=	$(OBJODIR)$(DIRSEP)cp437_utf8_tbl$(OFILE) \
		$(OBJODIR)$(DIRSEP)uucode$(OFILE) \
		$(OBJODIR)$(DIRSEP)yenc$(OFILE)


