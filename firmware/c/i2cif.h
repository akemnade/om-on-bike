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

void i2c_usb_init();
void handle_i2c_usb_data_out();
void handle_i2c_usb_data_in();
void init_ep3_desc();

extern __sfr __at (0x418) EP3OSTAT;
extern __sfr __at (0x41c) EP3ISTAT;


#define EP3IADDR 0x5c0
#define EP3ADDR 0x600
uint32_t get_eeprom_i2c_32(uint8_t i2c_addr,uint16_t addr);
void put_eeprom_i2c_32(uint8_t i2c_addr,uint16_t addr,uint32_t data);
