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
#include <delay.h>
#include <string.h>
#include <stdint.h>
#include "usb-drv.h"
#include "i2cif.h"
#include "sdcard.h"
#include "main.h"
#include "serirq.h"
#define ANZAHL_LEDS 11
#define FOSC 16000000
#define EP1SIZE 32
__sfr __at (0x408) EP1OSTAT;
__sfr __at (0x409) EP1OCNT;
__sfr __at (0x40a) EP1OADRL;
__sfr __at (0x40b) EP1OADRH;
__sfr __at (0x40c) EP1ISTAT;
__sfr __at (0x40d) EP1ICNT;
__sfr __at (0x40e) EP1IADRL;
__sfr __at (0x40f) EP1IADRH;
__sfr __at (0x414) EP2ISTAT;
__sfr __at (0x415) EP2ICNT;
__sfr __at (0x416) EP2IADRL;
__sfr __at (0x417) EP2IADRH;

__sfr __at (0x420) EP4OSTAT;
__sfr __at (0x421) EP4OCNT;
__sfr __at (0x422) EP4OADRL;
__sfr __at (0x423) EP4OADRH;
__sfr __at (0x424) EP4ISTAT;
__sfr __at (0x425) EP4ICNT;
__sfr __at (0x426) EP4IADRL;
__sfr __at (0x427) EP4IADRH;
#define EP1ADDR 0x580
#define EP1IADDR 0x5a0
#define EP3IADDR 0x5c0
#define EP3OADDR 0x600
/* for simplification in the code: EP2IADDR & 255 = 0 ! */
#define EP4IADDR 0x680
#define EP4OADDR 0x6c0
#define EP2IADDR 0x700
#define EP2ISIZE 0x40

static uint8_t  __at (EP1ADDR) ep1buf[EP1SIZE];
static uint8_t  __at (EP1IADDR) ep1ibuf[EP1SIZE];
static uint8_t  __at (EP2IADDR) ep2ibuf[256];
static uint8_t  __at (EP4IADDR) ep4ibuf[64];
static uint8_t __at (EP4OADDR) ep4obuf[64];
static uint8_t __at (SDBLOCKBUF) sdbuf[512];
static uint8_t __data *sertxbuf;
static uint8_t ep4tmpbuf[64];
uint8_t rxbufpos;
uint8_t sertxlen;
static struct {
   unsigned cycling: 1;
   unsigned input_power: 1;
   unsigned gps:1;
  unsigned gps_receiving:1;
   unsigned gps_has_data:1;
  unsigned gps_to_sd:1;
  unsigned gps_to_ep4:1;
  unsigned saving:1;
  unsigned sd_powered:1;
} powerstate;
uint8_t revs_without_data;
uint8_t sdcard_powerstate;
uint8_t vinh,vinl,vctll,vctlh;
uint32_t tmrdiff,tmrdiff2;
uint32_t tmrold;
uint8_t transfer_sdblock;
uint16_t sdblockpos;

uint8_t last_tmr2;
uint8_t tmr2_sd_turned_off;
uint8_t tmr2_last_idle;
uint8_t usbpulsecount;
uint8_t tmrportbxor;
uint8_t ser_in_progress;
static uint32_t pulsecounter;
static uint32_t saved_pulsecounter;
static uint32_t partstart;
int start_ser_tx(uint8_t __data *buf, uint8_t len);
int gps_standby();
static unsigned char save_to_sd();

static unsigned char restore_from_sd();
/* setup ep1 for receiving data from host */
void init_ep1_desc()
{
  EP1OCNT=EP1SIZE;
  EP1OADRL=EP1ADDR&255;
  EP1OADRH=EP1ADDR/256;
  EP1OSTAT=USTAT_USIE;
}

void init_ep4_desc()
{
  if (EP4OSTAT & 128)
    return;
  EP4OCNT=64;
  EP4OADRL=EP4OADDR&255;
  EP4OADRH=EP4OADDR/256;
  EP4OSTAT=USTAT_USIE;
}


/* setup endpoints after usb configuration,
  called from usb code */
void usb_init_other_eps()
{
/*  __UEP1bits_t tmpuep;
  tmpuep.EPCONDIS=1;
  tmpuep.EPHSHK=1;
  tmpuep.EPOUTEN=1;
  tmpuep.EPINEN=1; */
  UEP1=(1<<4) | (1<< 3) | (1<<2) | (1<<1); //(1<<EPCONDIS) | (1 << EPHSHK) | (1<<EPOUTEN) | (1<<EPINEN);
  EP1ISTAT=USTAT_UCPU | USTAT_DAT1;
  UEP2=(1<<4) | (1<<1) | (1<<3); //(1<<EPCONDIS) | (1 << EPHSHK) | (1<<EPINEN) ;
  EP2ISTAT=USTAT_UCPU | USTAT_DAT1;
  UEP3=(1<<4) |(1<<3)| (1<<2) | (1<<1); //(1<<EPCONDIS) | (1 << EPHSHK) | (1<<EPOUTEN) | (1<<EPINEN);
  EP3ISTAT=USTAT_UCPU | USTAT_DAT1;
  UEP4=(1<<4) | (1<<3) | (1<<2) | (1<<1);
  EP4ISTAT=USTAT_UCPU | USTAT_DAT1;
  T0CON=0x47;
  T0CONbits.TMR0ON=1;
  init_ep1_desc();
  init_ep3_desc();
  init_ep4_desc();
}

/* regularly check ad state 
 * 4 different states
 * if bit 0 is set, start conversion
 * if bit 0 is clear, read result and switch ad input
 *  setting
 */
void adstatecheck()
{
  static uint8_t adstate=0;
  adstate++;
  if (adstate&1) {
    ADCON0bits.GO_NOT_DONE=1;
    return;
  }
  if (adstate&2) {
    vinh=ADRESH;
    vinl=ADRESL;
    if (vinh > 0x29) {
      powerstate.input_power = 1;
    } else if (vinh < 0x10) {
      powerstate.input_power = 0;
    }
    //ADCON1.ACQT1=0;
    ADCON0=0xd;
  } else {
    vctlh=ADRESH;
    vctll=ADRESL;
    //ADCON1.4=1;
    ADCON0=1;
  }
}

static char cmd_gps_off[]="$PMTK161,0*28\r\n\r\n";

int gps_standby()
{
  return start_ser_tx(cmd_gps_off,sizeof(cmd_gps_off)-3);
}

int gps_on()
{
  return start_ser_tx(cmd_gps_off+sizeof(cmd_gps_off)-5,4);
}

static uint8_t gps_ggastate = 0;
static uint8_t gps_fieldnum = 0;
static uint8_t gps_sat = 0;
static uint8_t gps_satt = 0;
static uint8_t serfifosdpos = 0;
void ser_to_sd()
{
  while(serfifowritepos != serfifosdpos) {
    uint8_t x;
    x = serfifobuf[serfifosdpos];
   
    if ((powerstate.gps) && (powerstate.gps_to_sd)) {
      if ((sdcard_powerstate == 2) && (!powerstate.saving)) {
	if (sdcard_put_byte(x) == SDCARD_EAGAIN) {
	  return;
	}
      }
    }
    serfifosdpos++;
    serfifosdpos &= 63;
  }
}

void ser_data()
{
  uint8_t x,cmp;
  while(serfiforeadpos != serfifowritepos) {
    powerstate.gps_receiving = 1;
    revs_without_data = 0;
    x = serfifobuf[serfiforeadpos];
    serfiforeadpos++;
    serfiforeadpos &= 63;
    if (powerstate.gps_to_ep4) { 
      usb_ep4_put(x);
      if (x == 0xa)
	usb_ep4_flush(); 
    }
  
    if (x=='$') {
      gps_fieldnum = 0;
      gps_ggastate = 1;
      gps_sat = gps_satt;
      continue;
    }
    if (gps_ggastate == 0)
      continue;
    if (gps_ggastate >= 6) {
      if (x==',') {
	gps_fieldnum++;   
      } else if (gps_fieldnum == 7) {
	gps_satt *= 10;
	gps_satt += (x-'0');
      } else if (gps_fieldnum == 1) {
	gps_satt = 0;
      }
      continue;
    }
    switch(gps_ggastate) {
    case 1: cmp='G'; break;
    case 2: cmp='P'; break;
    case 3: cmp='G'; break;
    case 4: cmp='G'; break;
    case 5: cmp='A'; break;
    }
    if (x==cmp) {
      gps_ggastate++;
    } else {
      gps_ggastate=0;
    }
  } 
}

void usb_ep4_put(unsigned char data)
{
  if (rxbufpos < sizeof(ep4tmpbuf)) {
    ep4tmpbuf[rxbufpos] = data;
    rxbufpos++;
  }
  if (rxbufpos == 64) {
    usb_ep4_flush();
  }
}

void usb_ep4_flush()
{
  if (rxbufpos == 0)
    return;
  if (usb_state >= CONFIGURED_STATE) {
    if (!(EP4ISTAT & 128)) {
      memcpyram2ram(ep4ibuf, ep4tmpbuf, rxbufpos);
      EP4IADRH = EP4IADDR / 256;
      EP4IADRL = EP4IADDR & 255;
      EP4ICNT = rxbufpos;
      if (EP4ISTAT & (1<<USTAT_DTSBIT)) {
	EP4ISTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN;
      } else {
	EP4ISTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN;
      }
      rxbufpos = 0;
    }
  }
  if (rxbufpos == 64)
    rxbufpos = 0;
}

void ser_init()
{
#define SPBRGVAL -1+FOSC/4/
  SPBRGH = (SPBRGVAL 9600) / 256;
  SPBRG = (SPBRGVAL 9600) & 255;
  TXSTA = 0;
  TXSTAbits.BRGH = 1;
  BAUDCON = ( 1 << 3 ); // no auto baud, BRG16
  TXSTAbits.TXEN = 1;
  RCSTA = 0;
  RCSTAbits.SPEN = 1;
  RCSTAbits.CREN = 1;
  serfifosdpos = 0;
  serfiforeadpos = 0;
  serfifowritepos = 0;
  PIE1bits.RC1IE = 1; 
  rxbufpos = 0;
  sertxlen = 0;
  ser_in_progress = 0;
}

/* initialize ad */
void adinit() 
{
  TRISA|=(1<<0) | (1<< 3);
  ANCON1=0x1d;
  ADCON1=0xb;
  ANCON0=(1<<3) | (1<<0);
  ADCON0=0;
  ADCON0bits.ADON=1;
}

/* check if vin has reached a certain level
 * adc is assumed to be setup for vin check 
 */
int8_t vincheck()
{
  ADCON0bits.GO_NOT_DONE=1;
  delay1ktcy(1);
  return (ADRESH > 29);
}

/* handle EP1 input and (status polling iface) */
void handle_ep1()
{
  int i;
  for(i=0;i<EP1OCNT;i++) {
    switch(ep1buf[i]) {
        /* request state */
      case 0x30:
        if (!(EP1ISTAT&128)) {
        ep1ibuf[0]=0x31;
        ep1ibuf[1]=vinh;
        ep1ibuf[2]=vinl;
        ep1ibuf[3]=vctlh;
        ep1ibuf[4]=vctll;
        /* revolution period => speed */
	if (powerstate.cycling) {
	  ep1ibuf[5]=0;
	  ep1ibuf[6]=(tmrdiff >> 16) & 255;
	  ep1ibuf[7]=(tmrdiff >> 8) & 255;
	  ep1ibuf[8]=tmrdiff & 255;
	  
	} else {
	  ep1ibuf[5]=0;
	  ep1ibuf[6]=0;
	  ep1ibuf[7]=0;
	  ep1ibuf[8]=0;
	}
	  /* total distance */
        ep1ibuf[9]=(pulsecounter >> 24) & 255;
        ep1ibuf[10]=(pulsecounter >> 16) & 255;
        ep1ibuf[11]=(pulsecounter >> 8) & 255;
        ep1ibuf[12]=pulsecounter & 255;
	ep1ibuf[13]=(sd_last_block >> 24) & 255;
	ep1ibuf[14]=(sd_last_block >> 16) & 255;
	ep1ibuf[15]=(sd_last_block >>  8) & 255;
	ep1ibuf[16]=(sd_last_block) & 255;
	ep1ibuf[17]=sdcard_powerstate;
	ep1ibuf[18]=sdcard_status();
        /* setup EP1 to send the data to host */
        EP1IADRH=EP1IADDR / 256;
        EP1IADRL=EP1IADDR & 255;
        EP1ICNT=1+4+4+4+4+1+1;
        if (EP1ISTAT & (1<<USTAT_DTSBIT)) {
          EP1ISTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN;
        } else {
          EP1ISTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN;
        }
        }
        break;
        /* reset */
      case 0x65:
	UCON = 0;
	delay1mtcy(4*5);
        __asm
          reset
        __endasm;
       break; 
       /* power transistors output */
      case 0x42:
       LATBbits.LATB2=1;
       break;
      case 0x43:
       LATBbits.LATB2=0;
       break;
      case 0x44:
       LATBbits.LATB1=1;
       break;
      case 0x45:
       LATBbits.LATB1=0;
       break;
      case 0x46:
       LATCbits.LATC0=1;
       break;
      case 0x47:
       LATCbits.LATC0=0;
       break;
      case 0x48:
       LATCbits.LATC2=1;
       break;
      case 0x49:
       LATCbits.LATC2=0; 
       break;
    case 0x4a:
	if (powerstate.gps_to_sd == 1) {
	  powerstate.gps_to_sd = 0;
	  if (sdcard_powerstate == 2)
	    sdcard_flush_write();
	}
	break;
    case 0x4b:
      powerstate.gps_to_sd = 1;
      break;
    case 0x4c:
      powerstate.gps_to_ep4 = 0;
      break;
    case 0x4d:
      powerstate.gps_to_ep4 = 1;
      break;
    } 
  }
}
/* called from usb code when there is something to do
 * about the endpoints 
 */
uint32_t sd_block = 2;

#define EP4STARTWRITE(x) do { EP4IADRL = (x) & 255;	\
  EP4IADRH = (x) / 256; \
  EP4ICNT = 64; \
   if (EP4ISTAT & (1<<USTAT_DTSBIT)) { \
    EP4ISTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN; \
  } else { \
    EP4ISTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN; \
   }} while(0)

void sdbuf2usbep4()
{
  transfer_sdblock = 512/64;
  sdblockpos = SDBLOCKBUF;
}

void usb_other_eps()
{
  if (!(EP1OSTAT&128)) {
    handle_ep1();
    init_ep1_desc();
  }
  if (!(EP3ISTAT&128)) {
    handle_i2c_usb_data_out();
  }
  if (!(EP3OSTAT&128)) {
    handle_i2c_usb_data_in();
    init_ep3_desc();
  }
  if (!(EP3ISTAT&128)) {
    handle_i2c_usb_data_out();
  }
  
  if (!(EP4OSTAT&128)) {
    if (EP4OCNT > 0) {
      // powerstate.gps = 1;
      if (powerstate.gps_to_ep4) {
	if (!ser_in_progress) {
	  ser_in_progress = 1;
	  start_ser_tx(ep4obuf, EP4OCNT);
	}
      } else {
	usb_ep4_put('X');
	usb_ep4_put(ep4obuf[0]);
	switch(ep4obuf[0]) {
	case 0x81:
	  if (sdcard_read_block(1))
	    sdbuf2usbep4();
	  break;
	case 0x82:
	  sdcard_read_block(2);
	  break;
	case 0x83:
	  sdcard_start_write(2);
	  break;
	case 0x84:
	  sdcard_put_byte(ep4obuf[1]);
	  break;
	case 0x85:
	  sdcard_flush_write();
	  break;
	case 0x86:
	  usb_ep4_put(sdcard_status());
	  break;
	case 0x87:
	  sdcard_power_on();
	  sdcard_powerstate = 0;
	  break;
	case 0x88:
	  sdcard_read_block(partstart+1);
	  break;
	case 0x89:
	  restore_from_sd();
	  break;
	case 0x8a:
	  usb_ep4_put(saved_pulsecounter >> 24);
	  usb_ep4_put(saved_pulsecounter >> 16);
	  usb_ep4_put(saved_pulsecounter >> 8);
	  usb_ep4_put(saved_pulsecounter);
	  usb_ep4_put(pulsecounter >> 24);
	  usb_ep4_put(pulsecounter >> 16);
	  usb_ep4_put(pulsecounter >>  8);
	  usb_ep4_put(pulsecounter);
	  usb_ep4_put(partstart >> 24);
	  usb_ep4_put(partstart >> 16);
	  usb_ep4_put(partstart >>  8);
	  usb_ep4_put(partstart >>  9);
	  usb_ep4_put(sd_last_block >> 24);
	  usb_ep4_put(sd_last_block >> 16);
	  usb_ep4_put(sd_last_block >>  8);
	  usb_ep4_put(sd_last_block);
	  break;
	case 0x8b: {
	  uint32_t blk;
	  blk = (uint32_t) ep4obuf[1] << 24;
	  blk |= (uint32_t) ep4obuf[2] << 16;
	  blk |= (uint32_t) ep4obuf[3] << 8;
	  blk |= ep4obuf[4];
	  if (sdcard_read_block(blk+partstart))
	    sdbuf2usbep4();
	}
	  break;
	case 0x8c:
	  if (sdcard_idle())
	    save_to_sd();
	  break;
	case 0x8d: {
	  uint32_t blk;
	  blk = (uint32_t) ep4obuf[1] << 24;
	  blk |= (uint32_t) ep4obuf[2] << 16;
	  blk |= (uint32_t) ep4obuf[3] << 8;
	  blk |= ep4obuf[4];
	  if (sdcard_read_block(blk))
	    sdbuf2usbep4();
	}
	  break;
	}
	usb_ep4_flush();
	init_ep4_desc();
      }
    } else { 
      init_ep4_desc();
    }
  }
  if (!(EP4ISTAT & 128)) {
    if (transfer_sdblock) {
      EP4IADRL = sdblockpos & 255;
      EP4IADRH = sdblockpos >> 8;
      EP4ICNT = 64;
      if (EP4ISTAT & (1<<USTAT_DTSBIT)) {    
	EP4ISTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN;	
      } else {						
	EP4ISTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN;    
      }
      transfer_sdblock --;
      sdblockpos += 64;
    }
  } 
}

/* 
 * initialize timers
 * timer 0: ad state check
 * timer 1: tacho timings/dynamo frequency measurement
 */
void timer_init()
{
  usbpulsecount=0;
  pulse_pos=0;
  tmrb2=0;
  last_tmr2=0;
  tmr=0;
  tfound=0;
  tmrold=0;
  tmrdiff=0;
  pulsecounter=0;
  TMR1L=0;
  TMR1H=0;
  T1CON=0x37;
  RPINR1=8;
  INTCON2bits.INTEDG0=0;
  INTCON2bits.INTEDG1=0;
  INTCONbits.INT0IF=0;
  INTCON3bits.INT1IF=0;
  INTCON3bits.INT1IE=1;
  INTCONbits.INT0IE=1;  
  PIE1bits.TMR1IE=1; 
  /* INTCONbits.PEIE=1; */
  T0CON=0x47;
  T0CONbits.TMR0ON=1;
}

static void ser_tx_finish()
{
  if (ser_in_progress) {
    ser_in_progress = 0;
    init_ep4_desc();
  }
}

static void ser_tx_data()
{
  if (sertxlen != 0) {
    TXREG=*sertxbuf++;
    sertxlen--;
    if (sertxlen == 0) {
      ser_tx_finish();
    }
  }
  if (sertxlen == 0) {
    PIE1bits.TX1IE = 0;
  }
}

int start_ser_tx(uint8_t __data *buf, uint8_t len)
{
  if (PIE1bits.TX1IE)
    return 0;
  sertxbuf = buf;
  sertxlen = len;
  PIE1bits.TX1IE = 1;
  if (PIR1bits.TX1IF)
    ser_tx_data();
  return 1;
}

static void tmrcheck()
{
  if (!(tfound & 2)) {
    return; 
  }
  powerstate.cycling = 1;
  last_tmr2 = tmrb2;
  tfound &= ~2;
  

  if (tmrold == 0) {
    tmrold=tmr;
    return;
  }
  
  tmrdiff2=tmr-tmrold;
  if (tmrdiff2 & 0xffff0000) {
    tmrdiff=tmrdiff2;
    tmrold=tmr;
    pulsecounter++;
    revs_without_data++;
    if (revs_without_data & 0xf0) {
      revs_without_data = 15;
      powerstate.gps_receiving = 0;
    }
  }
}

/* checks if there is any non-usb interrupt flag set */
void myintr()
{
  ser_data();
  ser_to_sd();
 
  if (PIR1bits.TX1IF) {
    ser_tx_data();
  }
  tmrcheck(); 
}

/* checks whether some data of the streaming iface
   can be send (urb returned back to CPU)
*/ 
void check_pulse_send()
{
  if (EP2ISTAT&128) {
    return; 
  }
  if (((256-EP2ISIZE) & pulse_pos) != usbpulsecount) {
    EP2IADRL=usbpulsecount;
    EP2ICNT=EP2ISIZE;
    usbpulsecount+=EP2ISIZE; 
    EP2IADRH=EP2IADDR/256;
    if (EP2ISTAT & (1<<USTAT_DTSBIT)) {
      EP2ISTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN;
    } else {
      EP2ISTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN;
    }
  }
}

static unsigned char save_to_sd()
{
   sdbuf[0]=0;
   sd_last_block -= partstart;
#if 0
   sdbuf[15] = pulsecounter & 255;
   sdbuf[14] = (pulsecounter >> 8) & 255;
   sdbuf[13] = (pulsecounter >> 16) & 255;
   sdbuf[12] = (pulsecounter >> 24);
#else
   __asm       
     movff (_pulsecounter+0),(_sdbuf+15)
     movff (_pulsecounter+1),(_sdbuf+14)
     movff (_pulsecounter+2),(_sdbuf+13)
     movff (_pulsecounter+3),(_sdbuf+12)
     movff (_sd_last_block + 3),(_sdbuf+16)
     movff (_sd_last_block + 2),(_sdbuf+17)
     movff (_sd_last_block + 1),(_sdbuf+18)
     movff (_sd_last_block + 0),(_sdbuf+19)
     __endasm;
#endif
   sd_last_block += partstart;
   if (sdcard_write_block(partstart + 1)) {
     sdcard_idle();
     return 1;
   }
   return 0;

}

static unsigned char restore_from_sd()
{
  uint8_t  __data * ptbl;
  uint8_t i;
  if (!sdcard_read_block(0))
    return 0;
 
  ptbl = sdbuf + 0x1BE;
  
  for(i = 0; i < 4; i++) {
    if (ptbl[4]==0xb2) {
      partstart = ptbl[8 + 3];
      partstart <<= 8;
      partstart |= ptbl[8 + 2];
      partstart <<= 8;
      partstart |= ptbl[8 + 1];
      partstart <<= 8;
      partstart |= ptbl[8 + 0];
      break;
    }
    ptbl+=16;
  }
  if (i == 4)
    return 0;
  if (!sdcard_read_block(partstart + 1))
    return 0;
  if (!sdcard_read_block(partstart + 1))
    return 0;
  
#if 1
  __asm
    movff (_sdbuf+12), (_saved_pulsecounter+3)
    movff (_sdbuf+13), (_saved_pulsecounter+2)  
    movff (_sdbuf+14), (_saved_pulsecounter+1)
    movff (_sdbuf+15), (_saved_pulsecounter+0)
    movff (_sdbuf+16), (_sd_last_block+3)
    movff (_sdbuf+17), (_sd_last_block+2)
    movff (_sdbuf+18), (_sd_last_block+1)
    movff (_sdbuf+19), (_sd_last_block+0)
    __endasm;
#else
  /* get saved pulse counter, so total distance does not
     get lost during power losses */ 
  saved_pulsecounter=sdbuf[12];
  saved_pulsecounter <<= 8;
  saved_pulsecounter|=sdbuf[13];
  saved_pulsecounter <<= 8;
  saved_pulsecounter|=sdbuf[14];
  saved_pulsecounter <<= 8;
  saved_pulsecounter|=sdbuf[15];
  sd_last_block = sdbuf[16];
  sd_last_block <<= 8;
  sd_last_block |= sdbuf[17];
  sd_last_block <<= 8;
  sd_last_block |= sdbuf[18];
  sd_last_block <<= 8;
  sd_last_block |= sdbuf[19];
  /* saved_pulsecounter=get_eeprom_i2c_32(0x50,0xa830); */
#endif
  sd_last_block += partstart;
  return 1;
}

void init_interrupts()
{
  IPR1=0;
  IPR2=0;
  IPR3=0;
  INTCON2bits.TMR0IP=0;
  INTCON2bits.INT3IP=0;
  INTCON2bits.RBIP=0;
  INTCON3bits.INT2IP=0;
  INTCON3bits.INT1IP=1;
  

  RCONbits.IPEN=1;

  INTCONbits.TMR0IE=1;
  IPR1bits.RC1IP=1;
  IPR1bits.TMR1IP = 1;
  INTCONbits.GIEL = 0;
  INTCONbits.GIEH = 1;
}

/* sdcard timeout and power cycling logic */
static void sdcard_status_check()
{
  /* if sdcard is powered off, power it on, when it is powered off at least
   * for some seconds and no low supply voltage irq occured during the last
   * few seconds
   */
  if (!powerstate.sd_powered) {
    uint8_t td = tmrb2-tmr2_sd_turned_off;
    if (td > 50) {
      if (PIR2bits.LVDIF) {
	PIR2bits.LVDIF = 0;
	tmr2_sd_turned_off = tmrb2;
      } else {
	powerstate.sd_powered = 1;
	sdcard_power_on();
	tmr2_last_idle = tmrb2;
      }
    }
    /* if sdcard is powered on and it was never idle for several seconds,
     * assume it needs to be power cycled. Power the card off here.
     * This happens if supply voltage
     * is getting low.
     */
  } else {
    uint8_t td = tmrb2-tmr2_last_idle;
    if ((td > 50) && (powerstate.gps_to_sd)) {
      sdcard_power_off();
      tmr2_sd_turned_off = tmrb2;
      powerstate.sd_powered = 0;
    }
  }
  /* let sdcard do its work and 
   * check whether sdcard is idle 
   */
  if (sdcard_idle()) {
    tmr2_last_idle = tmrb2;
    /* if sdcard was never powered on,
     * restore saved pulsecounter once
     */
    if (sdcard_powerstate == 0) {
      if (restore_from_sd()) {
	sdcard_powerstate = 2;
	pulsecounter = saved_pulsecounter;
      } else
	/* restoring stuff failed,
	 * might be no sd, unprepared sd,
	 * check what needs to be done here
	 */
	sdcard_powerstate = 1;
    }
  }
}

/*
 * power gps on, try it again if no data is received
 * if gps is powered on >0.4s after standby, it might not power on
 */
static void gps_check_on()
{
  if (!powerstate.gps) {
    gps_on();
    powerstate.gps = 1;
  } else {
    if (!powerstate.gps_receiving) {
      if (revs_without_data > 7) {
	gps_on();
	revs_without_data = 1;
      }
    }
  }
}

/*
 * put gps to standby if there was a fix with a lot
 * of satellites
 * todo: ensure, it is really powered off
 */
static void gps_check_off()
{
  if ((powerstate.gps) && (powerstate.gps_has_data)) {
    if (gps_standby())
      powerstate.gps = 0;
  }
}

void main()
{
  int old_st=0;
/* sleep should not turn off peripherals */
  OSCCONbits.IDLEN=1;
  TRISC=0xb0;
  TRISA=0x9;
  TRISB=0xb1;
  adinit();
  sdblockpos = 0;
  transfer_sdblock = 0;
  powerstate.gps_to_ep4 = 1;
  powerstate.gps_to_sd = 1;
  powerstate.gps = 0;
  powerstate.saving = 0;
  powerstate.cycling = 0;
  powerstate.gps_has_data = 0;
  powerstate.gps_receiving = 0;
  powerstate.input_power = 0;
  partstart = 0;
  sdcard_powerstate = 0;
  timer_init();
  /* power off sdcard when power supply is not stable */
  sdcard_io_init();
  powerstate.sd_powered = 0;
  tmr2_sd_turned_off = tmrb2;
  sdcard_power_off();
  /* low voltage trip point at typ 3.0 (min 2.85, max. 3.15) */
  HLVDCON = 0x1c;
  /* sdcard_init(); */
  /* sdcard_idle(); */
  delay100ktcy(10);
  PIR2bits.LVDIF = 0;
  init_interrupts();
  /* setup bits for being waked up on interrupts */

  i2c_usb_init(); 
 
  /* continue of stable power supply is to be expected
   * two possibilities:
   * 1. vin reaches a certain level (so
   *   - only triggered when power consumption by light or
   *     usb power consumer (charger, smartphone) is low
   *
   * 2. a certain speed measurement by tacho is reached
   *   - works also when light is on or smartphone is heavily
   *     charging
   */
#ifndef NOVINCHECK
  while(1) {
     __asm
     bcf _LATB,1,0
     sleep
     bsf _LATB,1,0
    __endasm;
    tmrcheck();
    if (INTCONbits.TMR0IF) {
      INTCONbits.TMR0IF=0;
      if (vincheck())
        break;
    }
    if ((tmrdiff != 0) && (tmrdiff < 700000)) 
      break;
  }
#endif
  ser_init();
  gps_on();
  powerstate.gps = 1;
  sdcard_idle();
  delay100ktcy(1);
  saved_pulsecounter = 0;
  pulse_pos = 0;
  
  pulsecounter = saved_pulsecounter;
  usb_init();
  while(1) {
    uint8_t tmr2d;
    /* debug pin for measuring duty cycle of the cpu */
#ifndef NOMAINSLEEP 
    __asm
     bcf _PORTB,1,0
     sleep
     bsf _PORTB,1,0
    __endasm;
#endif
   if (INTCONbits.TMR0IF) {
     INTCONbits.TMR0IF=0;
     adstatecheck();
   } 
   myintr();
   sdcard_status_check();

   usb_check_interrupts();
   if (usb_state == CONFIGURED_STATE) {
     check_pulse_send();
   }
   /* check whether still cycling */
   tmr2d = (uint8_t)(tmrb2 - last_tmr2);
   if (tmr2d > 50) {
     powerstate.cycling = 0;
   }
   
   if (powerstate.cycling) {
     /* ensure that gps is powered on */
     gps_check_on();
   } else {
     gps_check_off();
   }
   /* a fix with at least seven satellites?
    * assume gps has collected enough data to have a quick fix after
    * standby/power on
    */
   if (gps_sat >= 7) {
     powerstate.gps_has_data = 1;
   }
   /* last block flushed?
    * update stats in first block
    */
   if (powerstate.saving) {
     if (sdcard_idle()) {
       save_to_sd();
       powerstate.saving = 0;
       saved_pulsecounter=pulsecounter;
     }
   }
   /* if ((UCONbits.SUSPND) || (usb_state != CONFIGURED_STATE)) */ {
     /* save, if vctl is low or gps is off and some distance was cycled */
     if (((vctlh < 0x88) || (!powerstate.gps)) && ((pulsecounter-saved_pulsecounter) >= 16)) {
       if (sdcard_powerstate == 2) {
	 if (! powerstate.saving) {
	   sdcard_put_byte(0);
	   sdcard_put_byte(pulsecounter >> 24);
	   sdcard_put_byte(pulsecounter >> 16);
	   sdcard_put_byte(pulsecounter >>  8);
	   sdcard_put_byte(pulsecounter);
	   sdcard_flush_write();
	   powerstate.saving = 1;
	 }
	 sdcard_idle();
       }
      
       /* put_eeprom_i2c_32(0x50,0xa830,pulsecounter); */
     }
   }
  }
  
}
    
