GTKMONITOR	=	$(EXEODIR)$(DIRSEP)gtkmonitor$(EXEFILE)

all: xpdev-mt smblib $(MTOBJODIR) $(EXEODIR) $(GTKMONITOR)

$(GTKMONITOR):	$(XPDEV-MT_LIB) $(SMBLIB)
