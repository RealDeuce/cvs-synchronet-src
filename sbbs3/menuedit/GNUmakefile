# Makefile

#########################################################################
# Makefile for Synchronet BBS Menu Editor (menuedit)
# For use with gcc and gmake for *nix      				#
# @format.tab-size 4													#
#########################################################################

# $Id: GNUmakefile,v 1.2 2004/07/23 06:10:55 deuce Exp $

# Macros
#DEBUG	=	1				# Comment out for release (non-debug) version
#USE_FLTK=	1
USE_UIFC32= 1
NEED_UIFC	:=	1
XPDEV	?=	../../xpdev/
UIFC_SRC	?=	../../uifc/

include ${XPDEV}Common.gmake
include ${UIFC_SRC}Common.gmake

# MenuEdit Build Rule
$(MENUEDIT): $(OBJS)
	@echo Linking $@
	${QUIET}$(CC) $(LDFLAGS) $(OBJS) -o $@

clean:
	@echo Deleting $(EXEODIR)
	$(QUIET)$(DELETE) $(EXEODIR)\*
