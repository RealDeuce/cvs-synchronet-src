# depends.mak

# Make 'include file' defining dependencies for Synchronet SBBS.DLL

# $Id: depends.mak,v 1.1 2002/01/25 01:09:30 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, and OFILE must be pre-defined

$(LIBODIR)$(SLASH)scfg.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgxtrn.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgmsg.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgnet.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgnode.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgsub.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgsys.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgxfr1.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgxfr2.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgchat.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgx.$(OFILE):		$(HEADERS) $(OBJS)

