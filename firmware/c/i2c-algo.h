/*
 * generic i2c interface bit banging - Copyright (C) 2012 - Andreas Kemnade
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

static void myi2c_init()
{
    MYI2CCLK=1;
#ifndef __AVR
    MYI2CCLKT=0;
    MYI2CDATAT=1;
#else
#warning AVR enabled!
    MYI2CCLKT=1;
    MYI2CDATAT=0;
#endif 
    MYI2CDATA=0;
#ifndef __AVR
    delay1mtcy(1);
//    TRISAbits.TRISA2=0;
//   PORTAbits.RA2=0;
#endif
}


static void myi2c_start()
{
    SETI2CDATAT(1);
    MYI2CCLK=1;
    MYI2CDELAY;
    SETI2CDATAT(0);
    MYI2CDATA=0;
    MYI2CDELAY;
    MYI2CCLK=0;
    MYI2CDELAY;
}

static void myi2c_stop()
{
    MYI2CDATA=0;
    SETI2CDATAT(0);
    MYI2CCLK=1;
    MYI2CDELAY;
    SETI2CDATAT(1);
    MYI2CDELAY;
}

#define WRITE_I2C_BIT(b) do {\
      SETI2CDATAT((b)?1:0);\
      MYI2CDATA=0;\
      MYI2CDELAY;\
      MYI2CCLK=1;\
      MYI2CDELAY;\
      MYI2CCLK=0;\
      MYI2CDELAY; } while (0)

static void write_i2c_bit(unsigned char b)
{
      SETI2CDATAT(b?1:0);
      MYI2CDATA=0;
      MYI2CDELAY;
    //  PORTA |= (1<<5);
      MYI2CCLK=1;
      MYI2CDELAY;
      MYI2CCLK=0;
     // PORTA&=~(1<<5);
      MYI2CDELAY;
}

static unsigned char get_i2c_bit()
{
    unsigned char ack;
    SETI2CDATAT(1);
    MYI2CDELAY;
    MYI2CCLK=1;
    MYI2CDELAY;
    ack=MYI2CDATAIN;
    MYI2CCLK=0;
    MYI2CDELAY;
    return ack;
}


static unsigned char write_i2c_byte(unsigned char b)
{
    unsigned char i;
  /* for(i=128;i!=0;i=i>>1) {
      write_i2c_bit((b&i)?1:0);
    } */
    if (b&(1<<7)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<6)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<5)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<4)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<3)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<2)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<1)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    if (b&(1<<0)) WRITE_I2C_BIT(1); else WRITE_I2C_BIT(0);
    return get_i2c_bit();
}

static unsigned char read_i2c_byte(unsigned char ack)
{
    unsigned char b;
    unsigned char ret=0;
    for(b=128;b!=0;b=b>>1) {
      if (get_i2c_bit())
        ret|=b;
    }
    write_i2c_bit(ack);
    return ret;
}

