UIFCLIB  =       $(UIFC_SRC)$(DIRSEP)$(LIBODIR)$(DIRSEP)$(LIBPREFIX)uifc$(LIBFILE)
UIFCLIB-MT  =       $(UIFC_SRC)$(DIRSEP)$(LIBODIR)$(DIRSEP)$(LIBPREFIX)uifc_mt$(LIBFILE)

UIFC_CFLAGS   =       -I$(UIFC_SRC)
UIFC_LDFLAGS  =       -L$(UIFC_SRC)$(DIRSEP)$(LIBODIR)
UIFC-MT_CFLAGS   =       -I$(UIFC_SRC)
UIFC-MT_LDFLAGS  =       -L$(UIFC_SRC)$(DIRSEP)$(LIBODIR)
UIFC_LIBS	=	$(UL_PRE)uifc$(UL_SUF)
UIFC-MT_LIBS	=	$(UL_PRE)uifc_mt$(UL_SUF)
