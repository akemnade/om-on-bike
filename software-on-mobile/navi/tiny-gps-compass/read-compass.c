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

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include "evt.h"
#include "tacho.h"
#include "compass.h"
int main(int argc, char **argv)
{
  union {
  heading_evt_t hd;
  magnetometer_evt_t magn;
  evt_head_t head;
  char buf[256];
  } evt;
  while(fread(&evt,1,sizeof(evt.head),stdin)) {
    fread(evt.buf+sizeof(evt.head),1,evt.head.len-sizeof(evt.head),stdin);
    switch(evt.head.type) {
      case EVENT_MAGNETOMETER:
         printf("X: %d Y: %d Z: %d\n",(int)evt.magn.x,(int)evt.magn.y,(int)evt.magn.z);
         break;
      case EVENT_HEADING:
         printf("heading: %.1f\n",evt.hd.heading10/10.0);
         break;
      default:
         printf("type: %d\n",(int)evt.head.type);
         break;
    }
  } 
  return 0;
}
