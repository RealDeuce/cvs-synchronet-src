# targets.mak

# Make 'include file' defining targets for Synchronet project

# $Id: targets.mak,v 1.6 2001/03/09 21:56:55 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBS	=	$(LIBODIR)$(SLASH)sbbs$(LIBFILE) 
FTPSRVR	=	$(LIBODIR)$(SLASH)ftpsrvr$(LIBFILE)
MAILSRVR=	$(LIBODIR)$(SLASH)mailsrvr$(LIBFILE)
SBBSCON	=	$(EXEODIR)$(SLASH)sbbscon$(EXEFILE)
SBBSMONO=	$(EXEODIR)$(SLASH)sbbs$(EXEFILE)
NODE	=	$(EXEODIR)$(SLASH)node$(EXEFILE)
BAJA	=	$(EXEODIR)$(SLASH)baja$(EXEFILE)
FIXSMB	=	$(EXEODIR)$(SLASH)fixsmb$(EXEFILE)
CHKSMB	=	$(EXEODIR)$(SLASH)chksmb$(EXEFILE)
SMBUTIL	=	$(EXEODIR)$(SLASH)smbutil$(EXEFILE)

all:	$(LIBODIR) $(EXEODIR) \
		$(SBBSMONO) \
		$(FIXSMB) $(CHKSMB) $(SMBUTIL) $(BAJA) $(NODE)

utils:	$(LIBDIR) $(EXEODIR) \
		$(FIXSMB) $(CHKSMB) $(SMBUTIL) $(BAJA) $(NODE)

dlls:	$(LIBDIR) $(EXEODIR) \
		$(SBBS) $(FTPSRVR) $(MAILSRVR)

mono:	$(LIBDIR) $(EXEODIR) \
		$(SBBSMONO)

clean:
	$(DELETE) $(LIBODIR)
	$(DELETE) $(EXEODIR)