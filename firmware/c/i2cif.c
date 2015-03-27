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

#define FOSC 8000000
#include <pic18fregs.h>
#include <delay.h>
#include <stdint.h>
#include <string.h>

#include "i2c-pic.h"
#include "i2c-algo.h"
#include "usb-drv.h"
#include "i2cif.h"

static uint8_t picprogstate;
static uint8_t picprogcmd;
static uint8_t picinbufpos;
static uint8_t databuf[64];

#define EP3SIZE 64
__sfr __at (0x418) EP3OSTAT;
__sfr __at (0x419) EP3OCNT;
__sfr __at (0x41a) EP3OADRL;
__sfr __at (0x41b) EP3OADRH;
__sfr __at (0x41c) EP3ISTAT;
__sfr __at (0x41d) EP3ICNT;
__sfr __at (0x41e) EP3IADRL;
__sfr __at (0x41f) EP3IADRH;

static uint8_t  __at (EP3ADDR) ep3buf[EP3SIZE];
static uint8_t  __at (EP3IADDR) ep3ibuf[EP3SIZE];

void init_ep3_desc()
{
  EP3OCNT=64;
  EP3OADRL=EP3ADDR&255;
  EP3OADRH=EP3ADDR/256;
  EP3OSTAT=USTAT_USIE;
}

void i2c_usb_init()
{
  picprogstate=0;
  myi2c_init();
  picinbufpos=0;
}

uint32_t get_eeprom_i2c_32(uint8_t i2c_addr,uint16_t addr)
{
  uint32_t res;
  myi2c_stop();
  myi2c_start();
  res=0;
  if (write_i2c_byte(i2c_addr+i2c_addr)!=0)
    goto eep_fail;
  if (write_i2c_byte(addr >> 8)!=0)
    goto eep_fail;
  if (write_i2c_byte(addr & 255)!=0)
    goto eep_fail;
  myi2c_stop();
  myi2c_start();
  if (write_i2c_byte(i2c_addr+i2c_addr+1)!=0)
    goto eep_fail;
  res=read_i2c_byte(0);
  res<<=8;
  res+=read_i2c_byte(0); 
  res<<=8;
  res+=read_i2c_byte(0); 
  res<<=8;
  res+=read_i2c_byte(0); 
eep_fail:
  myi2c_stop();
  return res;
}

void put_eeprom_i2c_32(uint8_t i2c_addr,uint16_t addr,uint32_t data)
{
  myi2c_stop();
  myi2c_start();
  if (write_i2c_byte(i2c_addr+i2c_addr)!=0) goto eep_fail;
  if (write_i2c_byte(addr>>8)!=0) goto eep_fail;
  if (write_i2c_byte(addr&255)!=0) goto eep_fail;
  if (write_i2c_byte(data >>24)!=0) goto eep_fail;
  if (write_i2c_byte((data >>16)&255)!=0) goto eep_fail;
  if (write_i2c_byte((data >>8)&255)!=0) goto eep_fail;
  if (write_i2c_byte((data    )&255)!=0) goto eep_fail;
eep_fail:
  myi2c_stop();
}
static void wr_bits(uint8_t data, uint8_t len)
{ 
  uint8_t i,j;
  /* if (len==8) {
    write_i2c_byte(data);
  } else */ {
    for(i=0;i!=len;i++) {
      write_i2c_bit((data&128)?1:0);
      data=data<<1;
    }
  }
  picprogstate=0;
}


static void rd_bits(uint8_t len)
{
  uint8_t res;
  uint8_t i;
 /* if (len==8) {
    res=read_i2c_byte();
  } else */ {
    uint8_t tmp;
    tmp=1;
    res=0;
    for(i=0;i<len;i++) {
      res=res+res;
      if (get_i2c_bit()) {
        res|=tmp;
      }
     
    }
    
  }
  databuf[picinbufpos]=res;
  picinbufpos++;
}


void handle_i2c_usb_data_out()
{
  if (picinbufpos == 0)
    return; 
  memcpyram2ram(ep3ibuf,databuf,picinbufpos);
  EP3IADRH=EP3IADDR/256;
  EP3IADRL=EP3IADDR&255;
  EP3ICNT=picinbufpos;
  picinbufpos=0;
  if (EP3ISTAT & (1<<USTAT_DTSBIT)) {
    EP3ISTAT=USTAT_USIE|USTAT_DAT0|USTAT_DTSEN;
  } else {
    EP3ISTAT=USTAT_USIE|USTAT_DAT1|USTAT_DTSEN;
  }
}

extern void ser_data();
void handle_i2c_usb_data_in()
{
  unsigned int i;
  for(i=0;i<EP3OCNT;i++) {

    if (picprogstate) {
      wr_bits(ep3buf[i],picprogcmd&15);
      /* writeavail */
    } else {
      picprogcmd=ep3buf[i];
      if (!(picprogcmd&128)) {
        delay100tcy(5);
      } else {
        if (picprogcmd&16) {
          if (picprogcmd&32) {
            rd_bits(picprogcmd&15);
          } else {
            uint8_t b=picprogcmd&15;
            switch(b) {
              case 0xf: myi2c_start(); break;
              case 0xe: myi2c_stop(); break;
              default:
                picprogstate=1;
            }
          }
        } else {
          if (picprogcmd&32) {
            databuf[picinbufpos]=MYI2CDATAIN?1:0;
            picinbufpos++;
          } else {
            if (picprogcmd&1) {
              MYI2CDATAT=1;
            } else {
              MYI2CDATAT=0;
              MYI2CDATA=0;
            }
          }
          if (picprogcmd&2) {
            MYI2CCLK=1;
          } else {
            MYI2CCLK=0;
          }
        }
      }
    }
  }
 
}
