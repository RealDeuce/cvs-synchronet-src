# targets.mk

# Make 'include file' defining targets for Synchronet SBBSINST project

# $Id: targets.mk,v 1.1 2003/01/21 10:10:18 rswindell Exp $

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBSINST	=	$(ODIR)$(SLASH)sbbsinst$(EXEFILE) 

all:	$(ODIR) \
		$(SBBSINST)

clean:
	@$(DELETE) $(ODIR)$(SLASH)*
