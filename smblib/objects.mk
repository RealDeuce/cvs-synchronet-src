# smblib/objects.mk

# Make 'include file' listing object files for SMBLIB

# $Id: objects.mk,v 1.2 2004/09/11 20:41:30 deuce Exp $

# OBJODIR, DIRSEP, and OFILE must be pre-defined

OBJS	=	$(OBJODIR)$(DIRSEP)smbadd$(OFILE)\
			$(OBJODIR)$(DIRSEP)smballoc$(OFILE)\
			$(OBJODIR)$(DIRSEP)smbdump$(OFILE)\
			$(OBJODIR)$(DIRSEP)smbfile$(OFILE)\
			$(OBJODIR)$(DIRSEP)smbhash$(OFILE)\
			$(OBJODIR)$(DIRSEP)smblib$(OFILE)\
			$(OBJODIR)$(DIRSEP)smbstr$(OFILE)\
			$(OBJODIR)$(DIRSEP)smbtxt$(OFILE)\
			$(OBJODIR)$(DIRSEP)crc16$(OFILE)\
			$(OBJODIR)$(DIRSEP)crc32$(OFILE)\
			$(OBJODIR)$(DIRSEP)md5$(OFILE)\
			$(OBJODIR)$(DIRSEP)lzh$(OFILE)
