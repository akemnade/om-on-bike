/*
 * om-on-bike (c firmware) - Copyright (C) 2013 - Andreas Kemnade
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

void usb_check_interrupts();
void usb_init();

#define DETACHED_STATE 0
#define ATTACHED_STATE 1
#define POWERED_STATE 2
#define USBDEFAULT_STATE 3
#define ADR_PENDING_STATE 4
#define ADDRESS_STATE 5
#define CONFIGURED_STATE 6

#define DEVADR 2
/* some masks somehow missing in the includes */
#define USTAT_BSTALL     0x04
#define USTAT_DTSEN      0x08
#define USTAT_DTSBIT        6
#define USTAT_INCDIS     0x10
#define USTAT_KEN        0x20
#define USTAT_DAT0       0x00
#define USTAT_DAT1       0x40
#define USTAT_DTSMASK    0x40
#define USTAT_USIE       0x80
#define USTAT_UCPU       0x00
#define DSC_DEV     0x01
#define DSC_CFG     0x02
#define DSC_STR     0x03
#define DSC_INTF    0x04
#define DSC_EP      0x05
#define SETUP_TOKEN 0x0d

extern int usb_state;


