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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "evt.h"
#include "compass.h"

int main(int argc, char **argv)
{
  int x,y,z;
  int minx,miny,maxy,maxx;
  union {
    heading_evt_t hd;
    magnetometer_evt_t magn;
    evt_head_t head;
    char buf[256];
  } evt;
  init_compass_calib(); 
  minx=miny=32767;
  maxx=maxy=-32768;
  while(fread(&evt,1,sizeof(evt.head),stdin)) {
    fread(evt.buf+sizeof(evt.head),1,evt.head.len-sizeof(evt.head),stdin);
    switch(evt.head.type) {
      case EVENT_MAGNETOMETER:
      case EVENT_MAGNETOMETER_EXT:
         calculate_heading(&evt.magn,&x,&y,&z);    
         printf("x: %d y: %d z: %d\n",x,y,z);
         if (x<minx)
           minx=x;
         if (x>maxx)
           maxx=x;
         if (y<miny)
           miny=y;
         if (y>maxy)
           maxy=y;
         break;
    } 
  }
  fprintf(stderr,"MinX: %d MaxX: %d, pp: %d\n",minx,maxx,maxx-minx);
  fprintf(stderr,"MinY: %d MaxY: %d, pp: %d\n",miny,maxy,maxy-miny);
  printf("%d %d 0\n",-((maxx-minx)/2+minx),-((maxy-miny)/2+miny)); 
  return 0;
}
