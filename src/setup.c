/*
 * setup.c - Installer for MSYNC35
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

#define CURRENT_VERSION "1.1"
#define AUTHOR          "WINNT35"
#define DESCRIPTION     "VMware Mouse Sync Driver for Windows NT 3.51"
#define SUPPORT_URL     "https://github.com/WINNT35/MSYNC35"

/* ---------------------------------------------------------------
 * CheckExistingInstall - checks if MSYNC35 is already installed
 * and prompts user to confirm replacement
 * returns 1 to proceed, 0 to cancel
 * --------------------------------------------------------------- */
int CheckExistingInstall(void)
{
    HKEY existingKey;
    char version[16];
    DWORD size;
    DWORD type;
    char response[4];

    if (RegOpenKey(HKEY_LOCAL_MACHINE, SERVICE_KEY, &existingKey)
        != ERROR_SUCCESS) {
        /* Not installed, proceed normally */
        return 1;
    }

    /* Already installed - check version */
    size = sizeof(version);
    if (RegQueryValueEx(existingKey, "Version", NULL, &type,
        (LPBYTE)version, &size) == ERROR_SUCCESS) {
        if (strcmp(version, CURRENT_VERSION) > 0) {
            printf("WARNING: Version %s is currently installed.\n", version);
            printf("You are about to downgrade to version %s.\n", CURRENT_VERSION);
            printf("Continue? (Y/N): ");
        } else if (strcmp(version, CURRENT_VERSION) == 0) {
            printf("Version %s is currently installed.\n", version);
            printf("Reinstall? (Y/N): ");
        } else {
            printf("Version %s is currently installed.\n", version);
            printf("Upgrade to version %s? (Y/N): ", CURRENT_VERSION);
        }
    } else {
        /* Version 1.0 is the only release without version tracking. */
        printf("Version 1.0 is currently installed.\n");
        printf("Upgrade to version %s? (Y/N): ", CURRENT_VERSION);
    }

    RegCloseKey(existingKey);

    fgets(response, sizeof(response), stdin);
    if (response[0] != 'Y' && response[0] != 'y') {
        printf("Installation cancelled.\n");
        return 0;
    }

    return 1;
}

/* ---------------------------------------------------------------
 * CopyDriver - copies MSYNC35.sys to the drivers folder
 * destPath receives the full path of the installed driver
 * --------------------------------------------------------------- */
int CopyDriver(char *destPath)
{
    char sysDir[MAX_PATH];
    char confirm[4];

    /* Get system directory e.g. C:\WINNT\System32 */
    GetSystemDirectory(sysDir, MAX_PATH);

    /* Build default destination path */
    sprintf(destPath, "%s\\drivers\\MSYNC35.sys", sysDir);

    printf("\nDriver will be copied to:\n");
    printf("  %s\n\n", destPath);
    printf("Is this correct? (Y/N): ");
    fgets(confirm, sizeof(confirm), stdin);

    if (confirm[0] == 'N' || confirm[0] == 'n') {
        printf("\nEnter full destination path:\n");
        printf("(e.g. C:\\WINNT\\System32\\drivers\\MSYNC35.sys)\n");
        printf("> ");
        fgets(destPath, MAX_PATH, stdin);

        /* Strip newline */
        destPath[strlen(destPath) - 1] = '\0';
    }

		printf("\nCopying driver...\n");

		/* Clear read-only attribute on existing file if present */
		SetFileAttributes(destPath, FILE_ATTRIBUTE_NORMAL);

		if (!CopyFile("MSYNC35.sys", destPath, FALSE)) {
			printf("ERROR: Failed to copy driver. (error %lu)\n",
				GetLastError());
			return 0;
		}

    printf("Driver copied successfully.\n");
    return 1;
}

/* ---------------------------------------------------------------
 * WriteRegistry - writes service registry entries
 * --------------------------------------------------------------- */
int WriteRegistry(const char *driverPath)
{
    HKEY hKey;
    LONG result;
    DWORD value;
    char imagePath[MAX_PATH];

    printf("\nWriting registry entries...\n");

    result = RegCreateKey(
        HKEY_LOCAL_MACHINE,
        SERVICE_KEY,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        printf("ERROR: Failed to create registry key. (error %ld)\n",
            result);
        return 0;
    }

    /* Type = SERVICE_KERNEL_DRIVER (1) */
    value = 1;
    RegSetValueEx(hKey, "Type", 0, REG_DWORD,
        (LPBYTE)&value, sizeof(DWORD));

    /* Start = SERVICE_SYSTEM_START (1) */
    value = 1;
    RegSetValueEx(hKey, "Start", 0, REG_DWORD,
        (LPBYTE)&value, sizeof(DWORD));

    /* ErrorControl = SERVICE_ERROR_NORMAL (1) */
    value = 1;
    RegSetValueEx(hKey, "ErrorControl", 0, REG_DWORD,
        (LPBYTE)&value, sizeof(DWORD));

    /* Group = Pointer Port */
    RegSetValueEx(hKey, "Group", 0, REG_SZ,
        (LPBYTE)"Pointer Port",
        strlen("Pointer Port") + 1);

    /* ImagePath - build \SystemRoot relative path */
    sprintf(imagePath, "\\SystemRoot\\System32\\drivers\\MSYNC35.sys");
    RegSetValueEx(hKey, "ImagePath", 0, REG_EXPAND_SZ,
        (LPBYTE)imagePath,
        strlen(imagePath) + 1);

    /* Identification entries */
    RegSetValueEx(hKey, "Version", 0, REG_SZ,
        (LPBYTE)CURRENT_VERSION,
        strlen(CURRENT_VERSION) + 1);

    RegSetValueEx(hKey, "Author", 0, REG_SZ,
        (LPBYTE)AUTHOR,
        strlen(AUTHOR) + 1);

    RegSetValueEx(hKey, "Description", 0, REG_SZ,
        (LPBYTE)DESCRIPTION,
        strlen(DESCRIPTION) + 1);

    RegSetValueEx(hKey, "SupportURL", 0, REG_SZ,
        (LPBYTE)SUPPORT_URL,
        strlen(SUPPORT_URL) + 1);

    RegCloseKey(hKey);

    printf("Registry updated successfully.\n");
    return 1;
}

/* ---------------------------------------------------------------
 * main
 * --------------------------------------------------------------- */
int main(void)
{
    char destPath[MAX_PATH];

    printf("MSYNC35 - VMware Mouse Driver for NT 3.51\n");
    printf("=========================================\n\n");
    printf("This installer will:\n");
    printf("  1. Copy MSYNC35.sys to your drivers folder\n");
    printf("  2. Register the driver in the registry\n\n");
    printf("Press Enter to continue or Ctrl+C to cancel...\n");
    getchar();

    if (!CheckExistingInstall()) {
        getchar();
        return 0;
    }

    if (!CopyDriver(destPath)) {
        printf("\nInstallation failed.\n");
        getchar();
        return 1;
    }

    if (!WriteRegistry(destPath)) {
        printf("\nInstallation failed.\n");
        getchar();
        return 1;
    }

    printf("\n==============================================\n");
    printf("Installation complete.\n");
    printf("Please reboot for changes to take effect.\n");
    printf("==============================================\n\n");
    printf("Press Enter to exit...\n");
    getchar();

    return 0;
}
