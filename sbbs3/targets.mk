# targets.mk

# Make 'include file' defining targets for Synchronet project

# $Id$

# LIBODIR, EXEODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBS		= $(LIBODIR)$(SLASH)$(LIBPREFIX)sbbs$(LIBFILE) 
FTPSRVR		= $(LIBODIR)$(SLASH)$(LIBPREFIX)ftpsrvr$(LIBFILE)
WEBSRVR		= $(LIBODIR)$(SLASH)$(LIBPREFIX)websrvr$(LIBFILE)
MAILSRVR	= $(LIBODIR)$(SLASH)$(LIBPREFIX)mailsrvr$(LIBFILE)
SERVICES	= $(LIBODIR)$(SLASH)$(LIBPREFIX)services$(LIBFILE)
SBBSCON		= $(EXEODIR)$(SLASH)sbbs$(EXEFILE)
SBBSMONO	= $(EXEODIR)$(SLASH)sbbsmono$(EXEFILE)
JSEXEC		= $(EXEODIR)$(SLASH)jsexec$(EXEFILE)
NODE		= $(EXEODIR)$(SLASH)node$(EXEFILE)
BAJA		= $(EXEODIR)$(SLASH)baja$(EXEFILE)
FIXSMB		= $(EXEODIR)$(SLASH)fixsmb$(EXEFILE)
CHKSMB		= $(EXEODIR)$(SLASH)chksmb$(EXEFILE)
SMBUTIL		= $(EXEODIR)$(SLASH)smbutil$(EXEFILE)
SBBSECHO	= $(EXEODIR)$(SLASH)sbbsecho$(EXEFILE)
ECHOCFG		= $(EXEODIR)$(SLASH)echocfg$(EXEFILE)
ADDFILES	= $(EXEODIR)$(SLASH)addfiles$(EXEFILE)
FILELIST	= $(EXEODIR)$(SLASH)filelist$(EXEFILE)
MAKEUSER	= $(EXEODIR)$(SLASH)makeuser$(EXEFILE)
ANS2MSG		= $(EXEODIR)$(SLASH)ans2msg$(EXEFILE)
MSG2ANS		= $(EXEODIR)$(SLASH)msg2ans$(EXEFILE)

UTILS		= $(BUILD_DEPENDS)$(FIXSMB) $(BUILD_DEPENDS)$(CHKSMB) \
			  $(BUILD_DEPENDS)$(SMBUTIL) $(BUILD_DEPENDS)$(BAJA) $(BUILD_DEPENDS)$(NODE) \
			  $(BUILD_DEPENDS)$(SBBSECHO) $(BUILD_DEPENDS)$(ECHOCFG) $(BUILD_DEPENDS) \
			  $(BUILD_DEPENDS)$(ADDFILES) $(BUILD_DEPENDS)$(FILELIST) $(BUILD_DEPENDS)$(MAKEUSER) \
			  $(BUILD_DEPENDS)$(ANS2MSG) $(BUILD_DEPENDS)$(MSG2ANS) \
			  $(BUILD_DEPENDS)$(JSEXEC)

all:	$(LIBODIR) $(EXEODIR) $(SBBSMONO) $(UTILS) $(SBBSCON)

utils:	$(EXEODIR) $(UTILS)

dlls:	$(LIBODIR) \
		$(SBBS) $(FTPSRVR) $(MAILSRVR) $(SERVICES)

mono:	$(LIBODIR) $(EXEODIR) $(SBBSMONO)

clean:
	@$(DELETE) $(LIBODIR)$(SLASH)*
	@$(DELETE) $(EXEODIR)$(SLASH)*
