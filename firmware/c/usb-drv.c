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


#include <pic18fregs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "usb-drv.h"
#define EP0SIZE 0x20
/* usb descriptors */
#include "dev-descs.h"

#define WAIT_SETUP 0
#define CTRL_TRF_TX 1
#define CTRL_TRF_RX 2
__sfr __at (0x400) EP0OSTAT;
__sfr __at (0x401) EP0OCNT;
__sfr __at (0x402) EP0OADRL;
__sfr __at (0x403) EP0OADRH;
__sfr __at (0x404) EP0ISTAT;
__sfr __at (0x405) EP0ICNT;
__sfr __at (0x406) EP0IADRL;
__sfr __at (0x407) EP0IADRH;
#define EP0ADDR 0x500
#define EP0IADDR 0x540
#define EP00_OUT 0
#define EP00_IN 4

static uint8_t  __at (EP0ADDR) ep0buf[EP0SIZE];
static uint8_t  __at (EP0IADDR) ep0ibuf[EP0SIZE];



int usb_state;
int old_state;
static int ctrl_state=0;
static uint8_t wcount=0;
static uint8_t tmp_addr;
static __code uint8_t *ctrl_desc_pos;

/* has to be implemented by user of this driver */
extern void usb_init_other_eps();

/* has to be implemented by user of this driver */
extern void usb_other_eps();

/* sets ctrl_desc_pos to the requested descriptor in flash */
void usb_get_descriptor()
{
  if (ep0buf[0] != 0x80)
    return;
  switch(ep0buf[3]) {
    case DSC_DEV:
    wcount=sizeof(dev_desc);
    ctrl_desc_pos=dev_desc;    
    break;
    case DSC_CFG:
    ctrl_desc_pos=config_desc;
    wcount=sizeof(config_desc);
    break;
    case DSC_STR:
     ctrl_desc_pos=str_desc_table[ep0buf[2]&7];
     ctrl_desc_pos++;
     wcount=*ctrl_desc_pos;
     ctrl_desc_pos--;

     break;
    
  } 
}
/* handle chapter 9 stuff */
void usb_handle_std_requests()
{
  if (ep0buf[0] & (3<<5)) {
    return; 
  }
  switch(ep0buf[1]) {
    case 5:
      usb_state = ADR_PENDING_STATE;
      tmp_addr = ep0buf[DEVADR];
      break;
    case 6:
      usb_get_descriptor();
      break;
    case 9:
      if (usb_state >= ADDRESS_STATE) {
      usb_state = CONFIGURED_STATE;
      usb_init_other_eps();
      }
      break;
    case 8:
      /* get cfg */
      ep0ibuf[0]=0;
      wcount=1;
      break;
    case 0:
      /* get_status */
      wcount=2;
      ep0ibuf[0]=0;
      ep0ibuf[1]=0;
      break;
    case 1:
    case 2:
      /* feature_req */
      break;
    case 10:
      /* get intf */
      wcount=1;
      ep0ibuf[0]=ep0buf[4];
      break;
    case 11:
      break;
  }
}

void ctrl_tx()
{
  uint8_t cpcount;
  uint8_t i;
  if (wcount == 0) {
   // ctrl_state=WAIT_SETUP;
  }
  /* data fits into the endpoint buffer,
   * or no data (to complete control transfer
   */
  if (wcount <= EP0SIZE) {
    EP0ICNT=wcount;
    cpcount=wcount;
    if (wcount == 0) {
    ctrl_desc_pos = NULL;
      return;
    }
    wcount=0;
  } else {
    /* copy a portion of the data */
    cpcount=EP0SIZE;
    EP0ICNT=EP0SIZE;
    wcount-=EP0SIZE;
  }
  /* if data is to be copied from flash, copy it
     else it is expected to be already there */
  if (ctrl_desc_pos != NULL) {
#if 0
  for(i=0;i<cpcount;i++) {
    ep0ibuf[i]=*ctrl_desc_pos++;
  } 
#else

    memcpypgm2ram(ep0ibuf,ctrl_desc_pos,cpcount);
    ctrl_desc_pos+=cpcount;
#endif 
  }
}

/* would be used for additional data to received in the control packets,
   not used here */
void ctrl_rx()
{
}

/* prepare EP0 for next setup packet */
static void usb_next_setup()
{
  ctrl_state=WAIT_SETUP;
  EP0OCNT=EP0SIZE;
  EP0OADRL=EP0ADDR&0xff;
  EP0OADRH=EP0ADDR/256;
  EP0OSTAT=USTAT_USIE;
  EP0ISTAT=USTAT_UCPU; 
}

void usb_handle_ctrl_ep()
{
  /* last transaction was EP0 receive */
  if (USTAT==EP00_OUT) {
    if (((EP0OSTAT & 0x3c) == (SETUP_TOKEN *4))) {
      /* setup, new control transaction */
      ctrl_state=WAIT_SETUP;
      usb_handle_std_requests();
      /* additional requests would go here */
      UCONbits.PKTDIS=0;
      /* check whether the request asks for data,
         or will send additional (payload) data */
      if (ep0buf[0]&128) {
        wcount=ep0buf[6];
        ctrl_tx();
        ctrl_state=CTRL_TRF_TX;
        EP0OCNT=EP0SIZE;
        EP0OADRL=EP0ADDR&0xff;
        EP0OADRH=EP0ADDR/256;
        EP0OSTAT=USTAT_USIE;
        EP0IADRL=EP0IADDR&0xff;
        EP0IADRH=EP0IADDR/256;
        EP0ISTAT=USTAT_USIE | USTAT_DAT1 | USTAT_DTSEN;
      } else {
        ctrl_state=CTRL_TRF_RX;
        EP0ICNT=0;
        EP0ISTAT=USTAT_USIE | USTAT_DAT1 | USTAT_DTSEN;
        EP0OCNT=EP0SIZE;
        EP0OADRL=EP0ADDR&0xff;
        EP0OADRH=EP0ADDR/256;
        EP0OSTAT=USTAT_USIE;
      }
    } else {
      /* receiving data, but no setup packet,
       */
      if (ctrl_state != CTRL_TRF_RX) {
       /* probably just the final zero byte packet */
        usb_next_setup(); 
      } else {
      /* additional data for a control transfer,
        currently unused  */
        ctrl_rx();
        EP0OADRL=EP0ADDR&0xff;
        EP0OADRH=EP0ADDR/256;
        if (EP0OSTAT & (1<<USTAT_DTSBIT)) {
          EP0OSTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN;
        } else {
          EP0OSTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN;
        }
      }
    }
  } 
  if ( USTAT==EP00_IN)  {
    /* in */
    /* address set transaction is finally complete,
     * so the address can be set */
    if (usb_state == ADR_PENDING_STATE) {
      UADDR=tmp_addr;
      if (UADDR > 0) {
        usb_state = ADDRESS_STATE;
      } else {
        usb_state = USBDEFAULT_STATE; 
      }
    }
    if (ctrl_state != CTRL_TRF_TX) {
      usb_next_setup(); 
    } else {
      /* send additional payload data (or the final
         zero byte transaction) in response of
         a control request */
      ctrl_tx();
      EP0IADRL=EP0IADDR&0xff;
      EP0IADRH=EP0IADDR/256;
      if (EP0ISTAT & (1<<USTAT_DTSBIT)) {
        EP0ISTAT=USTAT_USIE | USTAT_DAT0 | USTAT_DTSEN; 
      } else {
        EP0ISTAT=USTAT_USIE | USTAT_DAT1 | USTAT_DTSEN; 
      }
    }
  }
}

void usb_init()
{
  UEP0=0;
  UCFG=0;
  UCFGbits.FSEN=1;
  UCFGbits.UPUEN=1;
  usb_state=ATTACHED_STATE;
  old_state=255; 
  wcount=0;

  UCON=0;
  UIE=0;
  /* enable wakeup from idle mode for usb */
  UCONbits.USBEN=1;
  PIE2bits.USBIE=1;
}

/* (re-)initialize usb registers */
static void usb_reset()
{
  UEIR=0;
  UIR=0;
/* device_dependant values */
#if defined(__SDCC_PIC18F25J50)
  UEIE=0x9f;
  UIE=0x3b;
#else
#error define correct things for the selected processor
#endif
  UADDR=0;
  UEP1=0;
  UEP2=0;
  UEP3=0;
  UEP4=0;
  UEP5=0;
  UEP6=0;
  UEP7=0;
  UEP8=0;
  UEP9=0;
  UEP10=0;
  UEP11=0;
  UEP12=0;
  UEP13=0;
  UEP14=0;
  UEP15=0;
/* device dependant settings */
#if defined(__SDCC_PIC18F25J50)
  UEP0=(1<<4)| (1<<1) | (1<<2);
#else
#error define correct things for the selected processor
#endif
  //  UEP0bits = {.EPHSHK=1, .EPOUTEN=1, . EPINEN=1 };
  while (UIRbits.TRNIF)
    UIRbits.TRNIF=0;
  UCONbits.PKTDIS=0; 
  usb_next_setup();
  usb_state=USBDEFAULT_STATE;
}

void usb_check_interrupts()
{
  PIR2bits.USBIF=0;
  if (usb_state == ATTACHED_STATE) {
	if (UCONbits.SE0) {
		UIR=0;
                UIE=0;
                UIEbits.URSTIE=1;
                UIEbits.IDLEIE=1;
                usb_state=POWERED_STATE;
        }
  }
  /* bus activity -> wakeup from usb suspend */
  if ((UIRbits.ACTVIF) && (UIEbits.ACTVIE)) {
    UCONbits.SUSPND=0;
    UIEbits.ACTVIE=0;
    UIRbits.ACTVIF=0;
  }
  /* usb reset */
  if ((UIRbits.URSTIF) && (UIEbits.URSTIE)) {
    usb_reset();
  }
  /* no bus actitiy -> suspend */
  if ((UIRbits.IDLEIF) && (UIEbits.IDLEIE)) {
    UIEbits.ACTVIE=1;
    UIRbits.IDLEIF=0;
    UCONbits.SUSPND=1;
  }
  if ((UIRbits.SOFIF) && (UIEbits.SOFIE)) {
    UIRbits.SOFIF=0;
  }
  if ((UIRbits.STALLIF) && (UIEbits.STALLIE)) {
    if (UEP0bits.EPSTALL) {
      usb_next_setup(); 
      UEP0bits.EPSTALL=0;
    }
    UIRbits.STALLIF=0;
  }
  if ((UIRbits.UERRIF) && (UIEbits.UERRIE)) {
    UEIR=0;
    UIRbits.UERRIF=0;
  }
  if (usb_state < USBDEFAULT_STATE) 
    return;
   /* check transfer results, if in the right state */
  if ((UIRbits.TRNIF)&&(UIEbits.TRNIE)) {
    usb_handle_ctrl_ep();
    if (usb_state >= CONFIGURED_STATE)
      usb_other_eps();
    UIRbits.TRNIF=0;
  }
}
