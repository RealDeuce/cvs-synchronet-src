# build/rules.mk
#
# Global build targets for all make systems
#
# $Id: rules.mk,v 1.4 2004/09/12 21:39:08 deuce Exp $

$(OBJODIR):
	$(QUIET)$(IFNOTEXIST) mkdir $(OBJODIR)

$(MTOBJODIR):
	$(QUIET)$(IFNOTEXIST) mkdir $(MTOBJODIR)

$(LIBODIR):
	$(QUIET)$(IFNOTEXIST) mkdir $(LIBODIR)

$(EXEODIR):
	$(QUIET)$(IFNOTEXIST) mkdir $(EXEODIR)

clean:
	@echo Deleting $(OBJODIR)$(DIRSEP)
	$(QUIET)$(DELETE) $(OBJODIR)$(DIRSEP)*
	@echo Deleting $(MTOBJODIR)$(DIRSEP)
	$(QUIET)$(DELETE) $(MTOBJODIR)$(DIRSEP)*
	@echo Deleting $(LIBODIR)$(DIRSEP)
	$(QUIET)$(DELETE) $(LIBODIR)$(DIRSEP)*
	@echo Deleting $(EXEODIR)$(DIRSEP)
	$(QUIET)$(DELETE) $(EXEODIR)$(DIRSEP)*
