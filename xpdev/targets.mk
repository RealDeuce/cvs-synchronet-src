# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.1 2002/04/06 10:49:25 rswindell Exp $

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST	= $(ODIR)$(SLASH)wraptest$(EXEFILE) 

all: $(ODIR) $(WRAPTEST)

clean:
	@$(DELETE) $(ODIR)$(SLASH)*
