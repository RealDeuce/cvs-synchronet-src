# headers.mk

# Make 'include file' for building Synchronet DLLs 
# Used with GNU and Borland compilers

# $Id: headers.mk,v 1.3 2002/07/15 20:42:54 rswindell Exp $

HEADERS = \
	ars_defs.h \
	client.h \
	cmdshell.h \
	nodedefs.h \
	ringbuf.h \
	riodefs.h \
	sbbs.h \
	sbbsdefs.h \
	scfgdefs.h \
	scfglib.h \
	smbdefs.h \
	smblib.h \
	text.h \
	userdat.h \
	$(XPDEV)gen_defs.h \
	$(XPDEV)genwrap.h \
	$(XPDEV)wrapdll.h \
	$(XPDEV)dirwrap.h \
	$(XPDEV)filewrap.h \
	$(XPDEV)sockwrap.h \
	$(XPDEV)threadwrap.h
