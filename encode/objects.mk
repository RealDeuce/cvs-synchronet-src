# encode/objects.mk

# Make 'include file' listing object files for ENCODE LIB

# $Id: objects.mk,v 1.3 2019/06/29 00:24:11 rswindell Exp $

# OBJODIR, DIRSEP, and OFILE must be pre-defined

OBJS	=	$(OBJODIR)$(DIRSEP)cp437_utf8_tbl$(OFILE) \
		$(OBJODIR)$(DIRSEP)uucode$(OFILE) \
		$(OBJODIR)$(DIRSEP)yenc$(OFILE) \
		$(OBJODIR)$(DIRSEP)lzh$(OFILE) \
		$(OBJODIR)$(DIRSEP)base64$(OFILE)


