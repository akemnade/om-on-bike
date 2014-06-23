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
#include "usb-drv.h"
#include "i2cif.h"
#include "sdcard.h"
#include "main.h"
#define ANZAHL_LEDS 11
#define FOSC 16000000
#define EP1SIZE 16
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
#define EP1IADDR 0x590
#define EP3IADDR 0x5a0
#define EP3OADDR 0x5e0
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
static uint8_t ep4tmpbuf[64];
uint8_t rxbufpos;

uint8_t vinh,vinl,vctll,vctlh;
uint32_t tmrdiff,tmrdiff2;
uint32_t tmrb,tmrold;
uint8_t tmrb0,tmrb1,tmrb2;
uint8_t pulse_pos;
uint8_t usbpulsecount;
uint8_t tmrportbxor;
static uint32_t pulsecounter;
static uint32_t saved_pulsecounter;

/* setup ep1 for receiving data from host */
void init_ep1_desc()
{
  EP1OCNT=16;
  EP1OADRL=EP1ADDR&255;
  EP1OADRH=EP1ADDR/256;
  EP1OSTAT=USTAT_USIE;
}

void init_ep4_desc()
{
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
    //ADCON1.ACQT1=0;
    ADCON0=0xd;
  } else {
    vctlh=ADRESH;
    vctll=ADRESL;
    //ADCON1.4=1;
    ADCON0=1;
  }
}

void ser_data()
{
  uint8_t data;
  if (RCSTA & 3) {
    RCSTAbits.CREN = 0;
    RCSTAbits.CREN = 1;
    return;
  }
  data = RCREG;
  
  usb_ep4_put(data);
  if (data == 0xa)
    usb_ep4_flush(); 
  
  sdcard_put_byte(data);
 
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
}

void ser_init()
{
#define SPBRGVAL -1+FOSC/4/
  SPBRGH = (SPBRGVAL 9600) / 256;
  SPBRG = (SPBRGVAL 9600) & 255;
  TXSTA = 0;
  TXSTAbits.BRGH = 1;
  RCSTA = (1 << 7) | (1 << 4) ;
  BAUDCON = ( 1 << 3 ); // no auto baud, BRG16
  PIE1bits.RC1IE = 1;
  rxbufpos = 0;
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
        ep1ibuf[5]=0;
        ep1ibuf[6]=(tmrdiff >> 16) & 255;
        ep1ibuf[7]=(tmrdiff >> 8) & 255;
        ep1ibuf[8]=tmrdiff & 255;
        /* total distance */
        ep1ibuf[9]=(pulsecounter >> 24) & 255;
        ep1ibuf[10]=(pulsecounter >> 16) & 255;
        ep1ibuf[11]=(pulsecounter >> 8) & 255;
        ep1ibuf[12]=pulsecounter & 255;
        /* setup EP1 to send the data to host */
        EP1IADRH=EP1IADDR / 256;
        EP1IADRL=EP1IADDR & 255;
        EP1ICNT=1+4+4+4;
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
    } 
  }
}
/* called from usb code when there is something to do
 * about the endpoints 
 */
uint32_t sd_block = 2;
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
      if (ep4obuf[0] == 'X') {
	sdcard_init();
      } else if (ep4obuf[0] == 'Z') {
	uint16_t i;
	sdcard_start_write(sd_block);
	for(i = 512 ; i != 0 ; i--) {
	  sdcard_put_byte(i & 255);
	}
	sd_block++;
      }
      usb_ep4_flush();
    }
    init_ep4_desc();
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
  tmrb=0;
  tmrold=0;
  tmrdiff=0;
  pulsecounter=0;
  TMR1L=0;
  TMR1H=0;
  T1CON=0x27;
  RPINR1=8;
  INTCON2bits.INTEDG0=0;
  INTCON2bits.INTEDG1=0;
  INTCON3bits.INT1IE=1;
  INTCONbits.INT0IE=1;
  PIE1bits.TMR1IE=1; 
  INTCONbits.PEIE=1;
  INTCONbits.INT0IF=0;
  INTCON3bits.INT1IF=0;
  T0CON=0x47;
  T0CONbits.TMR0ON=1;
}

/* checks if there is any non-usb interrupt flag set */
void myintr()
{
  tmrportbxor=0;
  if (PIR1bits.RC1IF) {
    ser_data();
  }
  if (PIR1bits.TMR1IF) {
    PIR1bits.TMR1IF=0;
    tmrb2++;    
  }
  if (INTCONbits.INT0IF) {
    INTCONbits.INT0IF=0;
    tmrportbxor|=0x11;
  } 
  if (INTCON3bits.INT1IF) {
    INTCON3bits.INT1IF=0;
    tmrportbxor|=0x22;
  }
  if (!tmrportbxor) {
    return; 
  }
  do {
    tmrb0=TMR1L;
    tmrb1=TMR1H;
    if (PIR1bits.TMR1IF) {
      PIR1bits.TMR1IF=0;
      tmrb2++;    
    } else {
      break;
    }
  } while(1);
  if (usb_state >=CONFIGURED_STATE) {
   /* this clearly looks very ugly with the generated
      assembler code, so do it by hand */
#if 0
  ep2ibuf[pulse_pos]=0xfa;
  ep2ibuf[pulse_pos+1]=tmrportbxor;
  ep2ibuf[pulse_pos+2]=tmrb2;
  ep2ibuf[pulse_pos+3]=tmrb1;
  ep2ibuf[pulse_pos+4]=tmrb0;
  ep2ibuf[pulse_pos+5]=0xfb;
  pulse_pos+=6; 
#else
/* assumption EP2IADDR&255 = 0 ! */
  __asm
  movlw EP2IADDR/256
  movwf _FSR0H,0
  movff _pulse_pos,FSR0L
  movlw 0xfa
  movwf _POSTINC0,0
  movff _tmrportbxor,_POSTINC0
  movff _tmrb2,_POSTINC0
  movff _tmrb1,_POSTINC0
  movff _tmrb0,_POSTINC0
  movlw 0xfb
  movwf _POSTINC0,0 
  __endasm;
  pulse_pos+=6;
#endif
  }
  if (tmrportbxor&2) {
#if 0
    tmrb=((uint32_t)tmrb2<<16)+((uint32_t)tmrb1<<8)+(tmrb0);
#else
__asm
    movff  _tmrb2,_tmrb+2
    movff  _tmrb1,_tmrb+1
    movff  _tmrb0,_tmrb+0
__endasm;
#endif
    if (tmrold == 0) {
      tmrold=tmrb;
      return;
    }

    tmrdiff2=tmrb-tmrold;
    if (tmrdiff2 & 0xffff0000) {
      tmrdiff=tmrdiff2;
      tmrold=tmrb;
      pulsecounter++;
    }
  }
}

/* checks whether some data of the streaming iface
   can be send (urb returned back to CPU)
*/ 
void check_pulse_send()
{
  if (EP2ISTAT&128) {
    return; 
  }
  if (((256-EP2ISIZE) & pulse_pos) == usbpulsecount) {
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

void main()
{
  int old_st=0;
/* sleep should not turn off peripherals */
  OSCCONbits.IDLEN=1;
  TRISC=0xb0;
  TRISA=0x9;
  TRISB=0xb1;
  adinit();
  delay100ktcy(10);
  timer_init();
  /* setup bits for being waked up on interrupts */
  INTCONbits.PEIE=1;
  INTCONbits.TMR0IE=1;
  sdcard_io_init();
  i2c_usb_init(); 
  /* get saved pulse counter, so total distance does not
     get lost during power losses */ 
    
  saved_pulsecounter=get_eeprom_i2c_32(0x50,0xa830); 
  pulsecounter=saved_pulsecounter;
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
     bcf _PORTB,1,0
     sleep
     bsf _PORTB,1,0
    __endasm;
    myintr();
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
  sdcard_init();
  usb_init(); 
  while(1) {
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
   sdcard_idle();
   usb_check_interrupts();
   if (usb_state == CONFIGURED_STATE) {
     check_pulse_send();
   }
   if (UCONbits.SUSPND == 1) {
     /* save, if vctl is low and some distance was cycled */
     if ((vctlh < 0x88) && ((pulsecounter-saved_pulsecounter) >= 16)) {
       saved_pulsecounter=pulsecounter;
       put_eeprom_i2c_32(0x50,0xa830,pulsecounter);
     }
   }
  }
  
}
    
