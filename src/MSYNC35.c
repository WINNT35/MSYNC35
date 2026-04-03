/*
 * MSYNC35.c - VMware mouse sync driver for Windows NT 3.51
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

#include <ntddk.h>
#include <ntddmou.h> 
#include "kbdmou.h"

/* Safe IoSkipCurrentIrpStackLocation for NT 3.51 */
static __inline VOID IoSkipCurrentIrpStackLocation(PIRP Irp)
{
    Irp->CurrentLocation++;
    Irp->Tail.Overlay.CurrentStackLocation++;
}
/* ---------------------------------------------------------------
 * VMware backdoor definitions
 * --------------------------------------------------------------- */
#define VMWARE_MAGIC             0x564D5868  /* 'VMXh' */
#define VMWARE_PORT              0x5658      /* 'VX'   */
#define BDOOR_CMD_ABSPTR_DATA    39
#define BDOOR_CMD_ABSPTR_STATUS  40
#define BDOOR_CMD_ABSPTR_COMMAND 41
#define ABSPTR_ENABLE            0x45414552  /* Enable vmmouse */
#define ABSPTR_ABSOLUTE          0x53424152  /* Request absolute mode */
#define ABSPTR_RELATIVE          0x4c455252  /* Request relative mode */
#define VMWARE_ERROR             0xFFFF0000  /* Error indicator */

/* ---------------------------------------------------------------
 * Device extension - our per-device context
 * --------------------------------------------------------------- */
typedef struct _MOUSE_FILTER_EXTENSION {

    /* The device object below in the stack (i8042prt) */
    PDEVICE_OBJECT LowerDevice;

    /* The real mouclass callback and device object intercepted */
    CONNECT_DATA UpperConnectData;

    /* Button state tracking for transition detection */
    BOOLEAN LeftButtonDown;
    BOOLEAN RightButtonDown;

    /* Did the VMware backdoor initialize successfully? */
    BOOLEAN BackdoorActive;

} MOUSE_FILTER_EXTENSION, *PMOUSE_FILTER_EXTENSION;


/* ---------------------------------------------------------------
 * Forward declarations
 * --------------------------------------------------------------- */
VOID MouseFilterUnload(
    IN PDRIVER_OBJECT DriverObject
);

VOID MouseServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
);

NTSTATUS MouseFilterDispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS MouseFilterInternalIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID VmBackdoorEnable(VOID);
VOID VmBackdoorDisable(VOID);
BOOLEAN VmBackdoorGetPosition(
    OUT PULONG X,
    OUT PULONG Y,
    OUT PULONG Buttons
);


/* ---------------------------------------------------------------
 * DriverEntry - called by NT when driver is loaded
 * --------------------------------------------------------------- */
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
{
    ULONG i;
    NTSTATUS status;
    PDEVICE_OBJECT filterDevice;
    PDEVICE_OBJECT lowerDevice;
    UNICODE_STRING portName;
    PMOUSE_FILTER_EXTENSION ext;

    UNREFERENCED_PARAMETER(RegistryPath);

    /* Set all dispatch slots to passthrough first */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = MouseFilterDispatchPassThrough;
    }

    /* Override internal IOCTL handler - this is where we
     * intercept IOCTL_INTERNAL_MOUSE_CONNECT */
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        MouseFilterInternalIoctl;

    DriverObject->DriverUnload = MouseFilterUnload;

    /* Build the name of the i8042prt device we are attaching to */
    RtlInitUnicodeString(&portName,
        DD_POINTER_PORT_DEVICE_NAME_U L"0");

    /* Create our filter device object */
    status = IoCreateDevice(
        DriverObject,
        sizeof(MOUSE_FILTER_EXTENSION),
        NULL,
        FILE_DEVICE_MOUSE,
        0,
        FALSE,
        &filterDevice
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* Attach ourselves above i8042prt in the stack */
    status = IoAttachDevice(
        filterDevice,
        &portName,
        &lowerDevice
    );

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(filterDevice);
        return status;
    }

    /* Initialize our device extension */
    ext = (PMOUSE_FILTER_EXTENSION)filterDevice->DeviceExtension;
    RtlZeroMemory(ext, sizeof(MOUSE_FILTER_EXTENSION));
    ext->LowerDevice = lowerDevice;

    /* Copy I/O flags from lower device for buffer compatibility */
    filterDevice->Flags |= lowerDevice->Flags &
        (DO_BUFFERED_IO | DO_DIRECT_IO);
    filterDevice->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Switch VMware to absolute mouse mode */
    VmBackdoorEnable();
    ext->BackdoorActive = TRUE;

    return STATUS_SUCCESS;
}


/* ---------------------------------------------------------------
 * VmBackdoorEnable - switch VMware to absolute mouse mode
 * --------------------------------------------------------------- */
VOID VmBackdoorEnable(VOID)
{
    /* Step 1: Enable vmmouse */
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_COMMAND
        mov ebx, ABSPTR_ENABLE
        mov edx, VMWARE_PORT
        in  eax, dx
    }

    /* Step 2: Read status */
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_STATUS
        mov ebx, 0
        mov edx, VMWARE_PORT
        in  eax, dx
    }

    /* Step 3: Read 1 data item */
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_DATA
        mov ebx, 1
        mov edx, VMWARE_PORT
        in  eax, dx
    }

    /* Step 4: Switch to absolute mode */
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_COMMAND
        mov ebx, ABSPTR_ABSOLUTE
        mov edx, VMWARE_PORT
        in  eax, dx
    }
}


/* ---------------------------------------------------------------
 * VmBackdoorDisable - restore VMware to relative mouse mode
 * --------------------------------------------------------------- */
VOID VmBackdoorDisable(VOID)
{
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_COMMAND
        mov ebx, ABSPTR_RELATIVE
        mov edx, VMWARE_PORT
        in  eax, dx
    }
}


VOID MouseFilterUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject;
    PMOUSE_FILTER_EXTENSION ext;

    /* Get our device object and extension */
    deviceObject = DriverObject->DeviceObject;
    ext = (PMOUSE_FILTER_EXTENSION)deviceObject->DeviceExtension;

    /* Restore VMware to relative mouse mode */
    VmBackdoorDisable();

    /* Detach from lower device if attached */
    if (ext->LowerDevice) {
        IoDetachDevice(ext->LowerDevice); 
        ext->LowerDevice = NULL; /* Clear pointer to avoid double detach */
    }

    /* Delete our own device object */
    IoDeleteDevice(deviceObject);
}


/* ---------------------------------------------------------------
 * MouseFilterDispatchPassThrough - pass IRPs straight down
 * --------------------------------------------------------------- */
NTSTATUS MouseFilterDispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PMOUSE_FILTER_EXTENSION ext;

    ext = (PMOUSE_FILTER_EXTENSION)DeviceObject->DeviceExtension;

    if (ext->LowerDevice == NULL) {
        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(ext->LowerDevice, Irp);
}


/* ---------------------------------------------------------------
 * MouseFilterInternalIoctl - handle internal IOCTLs
 * --------------------------------------------------------------- */
NTSTATUS MouseFilterInternalIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PIO_STACK_LOCATION stack;
    PMOUSE_FILTER_EXTENSION ext;

    ext = (PMOUSE_FILTER_EXTENSION)DeviceObject->DeviceExtension;

    if (ext->LowerDevice == NULL) {
        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    stack = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_INTERNAL_MOUSE_CONNECT:
        {
            PCONNECT_DATA connectData;

            /* Get the CONNECT_DATA structure mouclass sent down */
            connectData = (PCONNECT_DATA)
                stack->Parameters.DeviceIoControl.Type3InputBuffer;

            /* Steal and store mouclass's device object and callback */
            ext->UpperConnectData = *connectData;

            /* Replace the callback with our own */
            connectData->ClassDeviceObject = DeviceObject;
            connectData->ClassService = MouseServiceCallback;

            break;
        }

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(ext->LowerDevice, Irp);
}


/* ---------------------------------------------------------------
 * VmBackdoorGetPosition - read absolute mouse position from
 * VMware backdoor port. Returns TRUE if data was available.
 * --------------------------------------------------------------- */
BOOLEAN VmBackdoorGetPosition(
    OUT PULONG X,
    OUT PULONG Y,
    OUT PULONG Buttons
)
{
    ULONG status;
    ULONG x, y, buttons;

    /* Read status */
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_STATUS
        mov ebx, 0
        mov edx, VMWARE_PORT
        in  eax, dx
        mov status, eax
    }

    /* Check for error */
    if ((status & VMWARE_ERROR) == VMWARE_ERROR) {
        return FALSE;
    }

    /* Check if at least 4 items are available */
    if ((status & 0xFFFF) < 4) {
        return FALSE;
    }

    /* Read 4 data items */
    __asm {
        mov eax, VMWARE_MAGIC
        mov ecx, BDOOR_CMD_ABSPTR_DATA
        mov ebx, 4
        mov edx, VMWARE_PORT
        in  eax, dx
        mov buttons, eax
        mov x, ebx
        mov y, ecx
    }

    *X       = x & 0xFFFF;
    *Y       = y & 0xFFFF;
    *Buttons = buttons & 0xFFFF;

    return TRUE;
}


/* ---------------------------------------------------------------
 * MouseServiceCallback - called by i8042prt when PS/2 data arrives
 * --------------------------------------------------------------- */
VOID MouseServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
)
{
    PMOUSE_FILTER_EXTENSION ext;
	MOUSE_INPUT_DATA mouseData;
	MOUSE_INPUT_DATA passData;
	ULONG x, y, buttons;

    ext = (PMOUSE_FILTER_EXTENSION)DeviceObject->DeviceExtension;

    /* Try to get absolute position from VMware backdoor */
    if (ext->BackdoorActive &&
        VmBackdoorGetPosition(&x, &y, &buttons)) {

        /* Initialize our synthetic packet */
        mouseData.UnitId = 0;
        mouseData.Flags = MOUSE_MOVE_ABSOLUTE;
        mouseData.Buttons = 0;
        mouseData.RawButtons = 0;
        mouseData.LastX = (LONG)x;
        mouseData.LastY = (LONG)y;
        mouseData.ExtraInformation = 0;

        /* Handle left button transitions */
        if (buttons & 0x20) {
            if (!ext->LeftButtonDown) {
                mouseData.Buttons |= MOUSE_LEFT_BUTTON_DOWN;
                ext->LeftButtonDown = TRUE;
            }
        } else {
            if (ext->LeftButtonDown) {
                mouseData.Buttons |= MOUSE_LEFT_BUTTON_UP;
                ext->LeftButtonDown = FALSE;
            }
        }

        /* Handle right button transitions */
        if (buttons & 0x10) {
            if (!ext->RightButtonDown) {
                mouseData.Buttons |= MOUSE_RIGHT_BUTTON_DOWN;
                ext->RightButtonDown = TRUE;
            }
        } else {
            if (ext->RightButtonDown) {
                mouseData.Buttons |= MOUSE_RIGHT_BUTTON_UP;
                ext->RightButtonDown = FALSE;
            }
        }

        /* Call real mouclass callback with our synthetic packet */
        (*(PSERVICE_CALLBACK_ROUTINE)ext->UpperConnectData.ClassService)(
            ext->UpperConnectData.ClassDeviceObject,
            &mouseData,
            &mouseData + 1,
            InputDataConsumed
        );

		} else {
				/* Backdoor not active or no data.
				 * Strip position data from PS/2 packet to avoid
				 * conflicting with absolute coordinates on next
				 * backdoor read. Only pass button state. */
				passData.UnitId           = InputDataStart->UnitId;
				passData.Flags            = MOUSE_MOVE_RELATIVE;
				passData.Buttons          = InputDataStart->Buttons;
				passData.RawButtons       = InputDataStart->RawButtons;
				passData.LastX            = 0;
				passData.LastY            = 0;
				passData.ExtraInformation = InputDataStart->ExtraInformation;

				(*(PSERVICE_CALLBACK_ROUTINE)ext->UpperConnectData.ClassService)(
					ext->UpperConnectData.ClassDeviceObject,
					&passData,
					&passData + 1,
					InputDataConsumed
				);
			}
}