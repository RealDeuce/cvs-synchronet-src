# Makefile

#########################################################################
# Makefile for Synchronet BBS Menu Editor (menuedit)
# For use with gcc and gmake for *nix      				#
# @format.tab-size 4													#
#########################################################################

# $Id: GNUmakefile,v 1.4 2005/02/20 20:51:40 deuce Exp $

# Macros
#DEBUG	=	1				# Comment out for release (non-debug) version

SRC_ROOT        =       ../..
include $(SRC_ROOT)/build/Common.gmake
CFLAGS	+=	$(UIFC_CFLAGS) $(XPDEV_CFLAGS)
LDFLAGS	+=	$(UIFC_LDFLAGS) $(XPDEV_LDFLAGS)

# MenuEdit Build Rule
$(MENUEDIT): $(OBJS)
	@echo Linking $@
	${QUIET}$(CC) $(LDFLAGS) $(MT_LDFLAGS) $(OBJS) -o $@
