# smblib/Makefile

#########################################################################
# Makefile for Synchronet Message Base Library (SMBLIB)					#
# For use with Borland C++ Builder 5+ or Borland C++ 5.5 for Win32      #
# @format.tab-size 4													#
#																		#
# usage: make															#
#########################################################################

# $Id: Makefile,v 1.1 2004/09/12 12:06:47 deuce Exp $

# Macros
#DEBUG	=	1				# Comment out for release (non-debug) version

# Enable auto-dependency checking
.autodepend
.cacheautodepend	

SRC_ROOT = ..
# Cross platform/compiler definitions
!include ..\build\Common.bmake	# defines clean and output directory rules

CFLAGS = $(CFLAGS) -I$(XPDEV_SRC) -I$(CIOLIB_SRC)

# SBBS DLL Link Rule
$(CIOLIB): $(OBJS)
    @echo Creating $< ...
	-$(QUIET)$(DELETE) $@
	$(QUIET)for %f in ($(OBJS)) do $(QUIET)tlib $@ +%f


