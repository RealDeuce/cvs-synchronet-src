# targets.mk

# Make 'include file' defining targets for Synchronet SCFG project

# $Id: targets.mk,v 1.2 2002/04/12 09:32:14 rswindell Exp $

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(ODIR)$(SLASH)scfg$(EXEFILE) 
SCFGHELP=	$(ODIR)$(SLASH)scfghelp.dat

all:	$(ODIR) \
		$(SCFG) $(SCFGHELP)

clean:
	@$(DELETE) $(ODIR)$(SLASH)*
