# targets.mak

# Make 'include file' defining targets for Synchronet project

# $Id: targets.mak,v 1.1 2002/01/25 01:09:44 rswindell Exp $

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(EXEODIR)$(SLASH)scfgx$(EXEFILE) 
SCFGOBJ	=	$(OBJODIR)$(SLASH)scfgx$(OBJFILE) 

all:	$(LIBODIR) $(EXEODIR) \
		$(OBJS)\
		$(SCFG)

clean:
	$(DELETE) $(LIBODIR)/*
	$(DELETE) $(EXEODIR)/*
