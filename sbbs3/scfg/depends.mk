# depends.mk

# Make 'include file' defining dependencies for Synchronet SCFG

# $Id: depends.mk,v 1.2 2002/04/12 09:32:14 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, and OFILE must be pre-defined

$(LIBODIR)$(SLASH)scfgxtrn.$(OFILE):	$(HEADERS)
$(LIBODIR)$(SLASH)scfgmsg.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgnet.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgnode.$(OFILE):	$(HEADERS)
$(LIBODIR)$(SLASH)scfgsub.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgsys.$(OFILE):		$(HEADERS)
$(LIBODIR)$(SLASH)scfgxfr1.$(OFILE):	$(HEADERS)
$(LIBODIR)$(SLASH)scfgxfr2.$(OFILE):	$(HEADERS)
$(LIBODIR)$(SLASH)scfgchat.$(OFILE):	$(HEADERS)
$(LIBODIR)$(SLASH)scfg.$(OFILE):		$(HEADERS)

