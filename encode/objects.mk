# encode/objects.mk

# Make 'include file' listing object files for ENCODE LIB

# $Id: objects.mk,v 1.4 2019/07/06 07:39:40 rswindell Exp $

# OBJODIR, DIRSEP, and OFILE must be pre-defined

OBJS	=	$(OBJODIR)$(DIRSEP)cp437_unicode_tbl$(OFILE) \
        $(OBJODIR)$(DIRSEP)utf8$(OFILE) \
		$(OBJODIR)$(DIRSEP)uucode$(OFILE) \
		$(OBJODIR)$(DIRSEP)yenc$(OFILE) \
		$(OBJODIR)$(DIRSEP)lzh$(OFILE) \
		$(OBJODIR)$(DIRSEP)base64$(OFILE)


