/*
 * uninst.c - Uninstaller for MSYNC35 (Mouse Sync for Windows NT 3.51, VMWare.)
 * Author: WINNT35 (Contact: WINNT35@outlook.com)
 * Copyright (C) 2026 WINNT35
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Alternative licensing terms may be available from the author upon request.
 */

#include <windows.h>
#include <stdio.h>

#define SERVICE_KEY \
    "SYSTEM\\CurrentControlSet\\Services\\MSYNC35"

/* ---------------------------------------------------------------
 * RemoveRegistry - deletes the service registry key
 * --------------------------------------------------------------- */
int RemoveRegistry(void)
{
    LONG result;

    printf("Removing registry entries...\n");

    result = RegDeleteKey(
        HKEY_LOCAL_MACHINE,
        SERVICE_KEY
    );

    if (result != ERROR_SUCCESS) {
        printf("ERROR: Failed to remove registry key. (error %ld)\n",
            result);
        printf("No MSYNC35 installation was detected.\n");
        return 0;
    }

    printf("Registry entries removed successfully.\n");
    return 1;
}

/* ---------------------------------------------------------------
 * RemoveDriver - deletes MSYNC35.sys from the drivers folder
 * --------------------------------------------------------------- */
int RemoveDriver(void)
{
    char sysDir[MAX_PATH];
    char driverPath[MAX_PATH];

    GetSystemDirectory(sysDir, MAX_PATH);
    sprintf(driverPath, "%s\\drivers\\MSYNC35.sys", sysDir);

    printf("Removing driver file...\n");
    printf("  %s\n", driverPath);

    /* Clear read-only attribute before attempting deletion */
    SetFileAttributes(driverPath, FILE_ATTRIBUTE_NORMAL);

    if (!DeleteFile(driverPath)) {
        printf("WARNING: Failed to delete driver file. (error %lu)\n",
            GetLastError());
        printf("You may delete it manually after rebooting:\n");
        printf("  %s\n", driverPath);
        return 0;
    }

    printf("Driver file removed successfully.\n");
    return 1;
}

/* ---------------------------------------------------------------
 * main
 * --------------------------------------------------------------- */
int main(void)
{
    printf("MSYNC35 Uninstaller - VMware Mouse Driver for NT 3.51\n");
    printf("=====================================================\n\n");
    printf("This will remove MSYNC35 from your system.\n\n");
    printf("Press Enter to continue or Ctrl+C to cancel...\n");
    getchar();

    if (!RemoveRegistry()) {
        printf("\nUninstallation failed.\n");
        printf("Press Enter to exit...\n");
        getchar();
        return 1;
    }

    RemoveDriver();

    printf("\n=====================================================\n");
    printf("Uninstallation complete.\n");
    printf("Please reboot for changes to take effect.\n");
    printf("=====================================================\n\n");
    printf("Press Enter to exit...\n");
    getchar();

    return 0;
}