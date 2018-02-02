# targets.mk

# Make 'include file' defining targets for Synchronet SCFG project

# $Id: targets.mk,v 1.10 2017/12/13 21:25:22 rswindell Exp $

# LIBODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(EXEODIR)$(DIRSEP)scfg$(EXEFILE) 

all:		xpdev-mt \
		uifc-mt \
		ciolib-mt \
		smblib \
		$(EXEODIR) \
		$(MTOBJODIR) \
		$(SCFG)

ifdef SBBSEXEC
.PHONY: install
install: all
	install $(EXEODIR)/* $(SBBSEXEC)

.PHONY: symlinks
symlinks: all
	ln -sfr $(EXEODIR)/* $(SBBSEXEC)
endif

$(SCFG):	$(XPDEV-MT_LIB) $(UIFCLIB-MT) $(CIOLIB-MT)
