# targets.mak

# Make 'include file' defining targets for Synchronet project

# $Id: targets.mak,v 1.3 2000/11/04 01:58:52 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBS	=	$(LIBODIR)$(SLASH)sbbs$(LIBFILE) 
FTPSRVR	=	$(LIBODIR)$(SLASH)ftpsrvr$(LIBFILE)
MAILSRVR=	$(LIBODIR)$(SLASH)mailsrvr$(LIBFILE)
SBBSCTRL=	$(EXEODIR)$(SLASH)sbbsctrl$(EXEFILE)
BAJA	=	$(EXEODIR)$(SLASH)baja$(EXEFILE)
FIXSMB	=	$(EXEODIR)$(SLASH)fixsmb$(EXEFILE)
CHKSMB	=	$(EXEODIR)$(SLASH)chksmb$(EXEFILE)
SMBUTIL	=	$(EXEODIR)$(SLASH)smbutil$(EXEFILE)

all: $(LIBODIR) $(EXEODIR) \
	 $(SBBS) $(FTPSRVR) $(MAILSRVR) $(SBBSCTRL) \
	 $(FIXSMB) $(CHKSMB) $(SMBUTIL) $(BAJA)

clean:
	$(DELETE) $(LIBODIR)$(SLASH)*
	$(DELETE) $(EXEODIR)$(SLASH)*