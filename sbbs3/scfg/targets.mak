# targets.mak

# Make 'include file' defining targets for Synchronet SCFG project

# $Id: targets.mak,v 1.3 2002/03/05 16:58:57 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(EXEODIR)$(SLASH)scfg$(EXEFILE) 
SCFGHELP=	$(EXEODIR)$(SLASH)scfghelp.dat

all:	$(LIBODIR) $(SBBSLIBODIR) $(UIFCLIBODIR) $(EXEODIR) \
		$(SCFG) $(SCFGHELP)

clean:
	$(DELETE) $(LIBODIR)$(SLASH)*
	$(DELETE) $(EXEODIR)$(SLASH)*
	$(DELETE) $(SBBSLIBODIR)$(SLASH)*
	$(DELETE) $(UIFCLIBODIR)$(SLASH)*
