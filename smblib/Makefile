# smblib/Makefile

#########################################################################
# Makefile for Synchronet Message Base Library (SMBLIB)					#
# For use with Borland C++ Builder 5+ or Borland C++ 5.5 for Win32      #
# @format.tab-size 4													#
#																		#
# usage: make															#
#########################################################################

# $Id: Makefile,v 1.4 2004/09/17 04:40:55 deuce Exp $

# Macros
#DEBUG	=	1				# Comment out for release (non-debug) version

# Enable auto-dependency checking
.autodepend
.cacheautodepend	

SRC_ROOT = ..
# Cross platform/compiler definitions
!include ..\build\Common.bmake	# defines clean and output directory rules

CFLAGS = $(CFLAGS) -I$(XPDEV_SRC) -DWRAPPER_IMPORTS -DSMB_IMPORTS

# SBBS DLL Link Rule
$(SMBLIB_BUILD): $(OBJS)
    @echo Creating $< ...
	-$(QUIET)$(DELETE) $@
	&$(QUIET)tlib $@ +$**


