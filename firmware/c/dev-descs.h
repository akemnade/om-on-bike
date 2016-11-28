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

const uint8_t dev_desc[]={
         sizeof(dev_desc),1, /* size, dev desc */
         0x00,0x02, /*  USB Spec Release Number */
         0,0,     /*  class,subclass */
         0,EP0SIZE,    /*  protocol, max packet size */
         0xD0,0x16, /* Vendor ID */
         0xd9,0x04, /* Product ID */
         0x03,0x00, /* Device release */
         0x01,0x02, /* manf string, product string */
         0x00,0x01}; /* serial number string, num of configurations */

const uint8_t config_desc[]={

 9,2,
 sizeof(config_desc),0, /* (config_end-config_desc),(config_end-config_desc)/256 */
 0x05,0x01, /*  num interfaces, configuration value */
 0x00,0x80, /*  configuration string, attributes */
 50, 9, /*  attributes, power consumption */
 0x04,0x00, /*  interface number */
 0x00,0x02, /*  alternate setting, num_endpoints */
 0xff,0xc3, /*  interface class, subclass */
 0x00,3, /*  interface protocol,interface string */
 7,0x05,
 0x01,0x02, /*  ep address, bulk transfer */
 0x20,0x0, /*  max_packet_size */
 0x0,7, /*  interval */
 0x05,0x81,
 0x02,0x20,
 0x0,0x0,

 9,4,
 1,0,
 1,0xff,
 0xc2,0,
 4,

 7, 5,0x82,
 0x02,0x40,
 0,0,

 9,4,
 2,0,
 2,0xff,
 0xca,0,
 5,

 7, 5,0x03,
 0x2,0x40,
 0,0,

 7,5,
 0x83,0x2,
 0x40,0,
 0,

 9, 4,3,
 0,2 ,
 0xff,0xc0,
 0,7,

 7,5,
 0x4,2,
 64,0,
 0,

 7, 5,0x84,
 2,64,
 0,0,

 9,4,
 0x04, /*  interface number */
 0x00,0x02, /*  alternate setting, num_endpoints */
 0x8,0x6, /*  interface class, subclass */
 0x50,3, /*  interface protocol,interface string */
 7,0x05,
 0x05,0x02, /*  ep address, bulk transfer */
 0x40,0x0, /*  max_packet_size */
 0x0,
 7, /*  interval */
 0x05,0x85,
 0x02,0x40,
 0x0,0x0,

 

};


static const uint8_t str0_desc[]={ 4,0x3, 0x09,0x04 };
static const uint8_t str1_desc[]={sizeof(str1_desc), 0x3, 'A',0,'K',0,' ',0,'H',0,'o',0,'b',0,'b',0,'y',0,' ',0,'p',0,'r',0,'o',0,'d',0,'u',0,'c',0,'t',0,'s',0};
static const uint8_t str2_desc[]={sizeof(str2_desc),0x3, 'B',0,'i',0,'k',0,'e',0,' ',0,'s',0,'e',0,'n',0,'s',0,'o',0,'r',0,' ',0,'s',0,'y',0,'s',0,'t',0,'e',0,'m',0};
static const uint8_t str3_desc[]={sizeof(str3_desc),0x3,'B',0,'i',0,'k',0,'e',0,' ',0,'c',0,'o',0,'n',0,'t',0,'r',0,'o',0,'l',0};
static const uint8_t str4_desc[]={sizeof(str4_desc),0x3,'P',0,'u',0,'l',0,'s',0,'e',0,' ',0,'m',0,'e',0,'a',0,'s',0,'u',0,'r',0,'e',0,'m',0,'e',0,'n',0,'t',0,' ',0,'t',0,'i',0,'m',0,'e',0,'r',0};
static const uint8_t str5_desc[]={sizeof(str5_desc),0x3,'I',0,'2',0,'C',0,'-',0,'M',0,'a',0,'s',0,'t',0,'e',0,'r',0};
static const uint8_t str6_desc[]={sizeof(str6_desc),0x3,'K',0,'e',0,'y',0,'b',0,'o',0,'a',0,'r',0,'d',0};
static const uint8_t str7_desc[]={sizeof(str7_desc),0x3,'R',0,'S',0,'2',0,'3',0,'2',0};
static const uint8_t __code *str_desc_table[]={str0_desc,str1_desc,str2_desc,str3_desc,str4_desc,str5_desc,str6_desc,str7_desc};
