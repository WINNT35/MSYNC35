MSYNC35 - VMware Mouse Sync Driver for Windows NT 3.51
======================================================
Version 1.1
Author: WINNT35
Contact: WINNT35@outlook.com
Release Date: 2026-04-03

DESCRIPTION
-----------
MSYNC35 enables seamless mouse integration between a VMware host
and a Windows NT 3.51 guest. The mouse cursor moves freely in and
out of the VM window without requiring Ctrl+Alt to release capture.

DOWNLOAD
--------
Precompiled binaries are available on the GitHub Releases page:
https://github.com/WINNT35/MSYNC35/releases

REQUIREMENTS
------------
- Windows NT 3.51 (tested on Service Pack 5)
- VMware (tested on Workstation 10, 15.5, and 16.1 Pro)

INSTALLATION
------------
1. Loading the files onto an ISO or IMG file is the best way
to transfer them to the guest virtual machine
2. Run setup.exe
3. When prompted, confirm or enter the drivers folder path
4. Reboot when installation is complete

FILES
-----
MSYNC35.sys  - Kernel filter driver
setup.exe    - Installer
UNINST.exe   - Uninstaller
readme.txt   - This file

REMOVAL
-------
Run UNINST.exe and reboot when prompted.

CHANGES
-------
v1.1 (2026-04-03)
  - Improved mouse input handling under non-standard host
    configurations, reducing cursor jitter and teleporting on
    affected systems
  - Improved driver unload sequence with proper device detachment
    and cleanup
  - Installer now detects existing installations and prompts
    before replacing
  - Installer now writes version, author, and support information
    to the registry
  - Uninstaller now correctly removes driver file
  - Minor internal code safety improvements

v1.0 (2026-04-01)
  - Initial release

KNOWN COMPATIBILITY NOTES
-------------------------
Tested on Windows Vista, 7, and 10 hosts with VMware Workstation
10 and 15.5. Nested VMware configurations (VMware running inside
VMware) are unsupported and may be unstable.

HOW IT WORKS
------------
MSYNC35 installs as a kernel filter driver between the PS/2 port
driver (i8042prt.sys) and the mouse class driver (mouclass.sys).
It communicates with VMware through the backdoor I/O port to
receive absolute mouse coordinates, replacing PS/2 relative
movement with precise absolute positioning.

REFERENCES
----------
vmwmouse - VMware mouse driver for Windows 3.x by NattyNarwhal
https://github.com/NattyNarwhal/vmwmouse

open-vm-tools - VMware open source tools for Linux
https://github.com/vmware/open-vm-tools

LICENSE
-------
This software is licensed under the GNU General Public License
version 2 or later. It may be licensed under different terms with
written permission of the author.

Contact WINNT35@outlook.com for alternative licensing inquiries.

SOURCE CODE
-----------
Built with MSVC 4 and the Windows NT 3.51 DDK.
Source code available at:
https://github.com/WINNT35/MSYNC35