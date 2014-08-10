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
  evt_tacho_t tacho;
  evt_speed_t speed;
  evt_head_t head;
  char buf[256];
  } evt;
  while(fread(&evt,1,sizeof(evt.head),stdin)) {
    fread(evt.buf+sizeof(evt.head),1,evt.head.len-sizeof(evt.head),stdin);
    switch(evt.head.type) {
      case EVENT_TACHO:
         printf("timer: %d us\n",(int)evt.tacho.counter);
         break;
      case EVENT_SPEED:
         printf("speed: %.1f\n",3.6*0.01*(float)evt.speed.speed);
         break;
      default:
         printf("type: %d\n",(int)evt.head.type);
         break;
    }
  } 
  return 0;
}
