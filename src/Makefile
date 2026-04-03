# MSYNC35 - VMware Mouse Sync Driver for Windows NT 3.51
# Master makefile - builds driver, installer, and uninstaller
# Usage: nmake
#
# Requires NTDDK environment variable to be set

CC   = cl
LINK = link

# ---------------------------------------------------------------
# Validate required environment variables
# ---------------------------------------------------------------
!IFNDEF NTDDK
!ERROR NTDDK environment variable not set. Example: set NTDDK=C:\NTDDK
!ENDIF

!IFNDEF MSDEVDIR
!ERROR MSDEVDIR environment variable not set. Example: set MSDEVDIR=C:\MSDEV
!ENDIF

# ---------------------------------------------------------------
# Driver flags (kernel mode)
# ---------------------------------------------------------------
DRIVER_CFLAGS = -nologo -c -G3 -Gz -W3 -Zp8 -D_X86_=1 -Di386=1 -O2

DRIVER_INCLUDES = -I. \
                  -I$(NTDDK)\inc \
                  -I$(NTDDK)\src\input\inc

DRIVER_LFLAGS = /NODEFAULTLIB \
                /ALIGN:32 \
                /SUBSYSTEM:NATIVE \
                /BASE:0x10000 \
                /ENTRY:DriverEntry@8 \
                /DRIVER \
                /MACHINE:IX86 \
                /INCREMENTAL:NO \
                /NOLOGO

DRIVER_LIBS = $(NTDDK)\lib\i386\free\ntoskrnl.lib \
              $(NTDDK)\lib\i386\free\hal.lib

# ---------------------------------------------------------------
# User mode flags (installer and uninstaller)
# ---------------------------------------------------------------
USER_CFLAGS = -c -G3 -nologo -D_X86_ -O2

USER_INCLUDES = -I$(MSDEVDIR)\INCLUDE

USER_LFLAGS = /NOLOGO \
              /SUBSYSTEM:CONSOLE \
              /MACHINE:IX86 \
              /INCREMENTAL:NO

USER_LIBS = $(MSDEVDIR)\LIB\kernel32.lib \
            $(MSDEVDIR)\LIB\advapi32.lib

# ---------------------------------------------------------------
# Targets
# ---------------------------------------------------------------
all: MSYNC35.sys setup.exe uninst.exe

# Driver
MSYNC35.obj: MSYNC35.c
	$(CC) $(DRIVER_CFLAGS) $(DRIVER_INCLUDES) MSYNC35.c

MSYNC35.sys: MSYNC35.obj
	$(LINK) $(DRIVER_LFLAGS) /OUT:MSYNC35.sys MSYNC35.obj $(DRIVER_LIBS)

# Installer
setup.obj: setup.c
	$(CC) $(USER_CFLAGS) $(USER_INCLUDES) setup.c

setup.exe: setup.obj
	$(LINK) $(USER_LFLAGS) /OUT:setup.exe setup.obj $(USER_LIBS)

# Uninstaller
uninst.obj: uninst.c
	$(CC) $(USER_CFLAGS) $(USER_INCLUDES) uninst.c

uninst.exe: uninst.obj
	$(LINK) $(USER_LFLAGS) /OUT:uninst.exe uninst.obj $(USER_LIBS)

# ---------------------------------------------------------------
# Clean
# ---------------------------------------------------------------
clean:
	-del *.obj
	-del *.sys
	-del *.exe