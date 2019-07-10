# $Id: GNUmakefile,v 1.2 2019/06/29 00:24:11 rswindell Exp $

# Macros
#DEBUG	=	1				# Comment out for release (non-debug) version

SRC_ROOT = ..
# Cross platform/compiler definitions
include $(SRC_ROOT)/build/Common.gmake	# defines clean and output directory rules

CFLAGS += -I$(ENCODE_SRC) $(XPDEV_CFLAGS)

# ENCODE Library Link Rule
$(ENCODE_BUILD): $(OBJODIR) $(OBJS)
	@echo Creating $@ ...
	$(QUIET)$(AR) rc $@ $(OBJS)
	$(QUIET)$(RANLIB) $@

