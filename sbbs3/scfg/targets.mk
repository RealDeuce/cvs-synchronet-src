# targets.mk

# Make 'include file' defining targets for Synchronet SCFG project

# $Id: targets.mk,v 1.3 2003/01/17 22:31:22 rswindell Exp $

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(ODIR)$(SLASH)scfg$(EXEFILE) 
MAKEHELP=	$(ODIR)$(SLASH)makehelp$(EXEFILE) 
SCFGHELP=	$(ODIR)$(SLASH)scfghelp.dat

all:	$(ODIR) \
		$(SCFG) $(SCFGHELP)

clean:
	@$(DELETE) $(ODIR)$(SLASH)*
