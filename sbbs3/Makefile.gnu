# Makefile.gnu

#########################################################################
# Makefile for Synchronet BBS 											#
# For use with GNU make and GNU C Compiler								#
# @format.tab-size 4, @format.use-tabs true								#
#																		#
# Linux: make -f Makefile.gnu											#
# Win32: make -f Makefile.gnu os=win32									#
#########################################################################

# $Id: Makefile.gnu,v 1.9 2000/10/30 08:51:44 rswindell Exp $

# Macros
DEBUG	=	1		# Comment out for release (non-debug) version
CC		=	gcc
SLASH	=	/
OFILE	=	o

ifeq ($(os),win32)	# Windows

LD		=	dllwrap
LIBFILE	=	.dll
EXEFILE	=	.exe
LIBODIR	:=	gcc.win32.dll
EXEODIR	:=	gcc.win32.exe
LIBDIR	:=	/gcc/i386-mingw32/lib
CFLAGS	:=	-mno-cygwin
LFLAGS  :=	--target=i386-mingw32 -mno-cygwin
DELETE	=	echo y | del 

else	# Linux

LD		=	ld
LIBFILE	=	.a
EXEFILE	=	
LIBODIR	:=	gcc.linux.lib
EXEODIR	:=	gcc.linux.exe
LIBDIR	:=	/usr/lib
CFLAGS	:=	
LFLAGS  :=	
DELETE	=	rm -f -v

endif

ifdef DEBUG
CFLAGS	:=	$(CFLAGS) -g -O0 -D_DEBUG 
LIBODIR	:=	$(LIBODIR).debug
EXEODIR	:=	$(EXEODIR).debug
else
LFLAGS	:=	$(LFLAGS) -S
LIBODIR	:=	$(LIBODIR).release
EXEODIR	:=	$(EXEODIR).release
endif

include targets.mak		# defines all targets
include objects.mak		# defines $(OBJS)
include headers.mak		# defines $(HEADERS)
include sbbsdefs.mak	# defines $(SBBSDEFS)

SBBSLIB	=	$(LIBODIR)/sbbs.a
	
LIBS	=	$(LIBDIR)/libwsock32.a $(LIBDIR)/libwinmm.a

# Implicit C Compile Rule for SBBS
$(LIBODIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $(SBBSDEFS) $< -o $@

# Implicit C++ Compile Rule for SBBS
$(LIBODIR)/%.o : %.cpp
	$(CC) $(CFLAGS) -c $(SBBSDEFS) $< -o $@

# Create output directories
$(LIBODIR):
	mkdir $(LIBODIR)

$(EXEODIR):
	mkdir $(EXEODIR)


# SBBS Link Rule
$(SBBS): $(OBJS) $(LIBODIR)/ver.o
	$(LD) $(LFLAGS) -o $(SBBS) $^ $(LIBS) --output-lib $(SBBSLIB)

# FTP Server Link Rule
$(FTPSRVR): $(LIBODIR)/ftpsrvr.o $(SBBSLIB)
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) --output-lib $(LIBODIR)/ftpsrvr.a

# Mail Server Link Rule
$(MAILSRVR): $(LIBODIR)/mailsrvr.o $(LIBODIR)/mxlookup.o $(SBBSLIB)
	$(LD) $(LFLAGS) -o $@ $^ $(LIBS) --output-lib $(LIBODIR)/mailsrvr.a

# Specifc Compile Rules
$(LIBODIR)/ftpsrvr.o: ftpsrvr.c ftpsrvr.h
	$(CC) $(CFLAGS) -c -DFTPSRVR_EXPORTS $< -o $@

$(LIBODIR)/mailsrvr.o: mailsrvr.c mailsrvr.h
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@

$(LIBODIR)/mxlookup.o: mxlookup.c
	$(CC) $(CFLAGS) -c -DMAILSRVR_EXPORTS $< -o $@		

# Baja Utility
$(BAJA): baja.c ars.c
	$(CC) $(CFLAGS) -o $@ $^

# FIXSMB Utility
$(FIXSMB): fixsmb.c smblib.c smbwrap.c
	$(CC) $(CFLAGS) -o $@ $^

# CHKSMB Utility
$(CHKSMB): chksmb.c smblib.c smbwrap.c
	$(CC) $(CFLAGS) -o $@ $^

# SMB Utility
$(SMBUTIL): smbutil.c smblib.c smbwrap.c smbtxt.c lzh.c
	$(CC) $(CFLAGS) -o $@ $^


include depends.mak