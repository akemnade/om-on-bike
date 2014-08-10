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
#include "tacho.h"
#include "compass.h"
static double lon,lat;
/* should not be hard-coded but configurable */
static double rotlen=2.230;

/*
 * adds a combination of distance and heading to the current position
 * math is quite simple, could be problematic in case of adding longer
 *  distances
 */

static void update_position(double dist, double heading)
{
  lat+=cos(heading/180.0*M_PI)*dist/(6371221.0*M_PI)*180;
  lon+=sin(heading/180.0*M_PI)*dist/(6371221.0*M_PI*cos(lat*M_PI/180.0))*180.0; 
}


/*
 * send a line in nmea-compatible format
 */
static void out_nmealine(double t, double lon,
                         double lat, double heading, double speed)
{
  char timestr[20];
  char datestr[20];
  char linebuf[100];
  struct timeval tv;
  struct tm tm;
  int chksum;
  int l,i;
  int lattdeg,longdeg;
  double lattmin, longmin;
  tv.tv_sec=(int)t;
  tv.tv_usec=(t-(double)tv.tv_sec)*1000000.0;
  if (tv.tv_usec > 1000000) {
    tv.tv_sec++;
    tv.tv_usec-=1000000;
  }
  tm=*gmtime(&tv.tv_sec);
  strftime(datestr, sizeof(datestr),"%d%m%y",&tm);
  tv.tv_sec=tv.tv_sec%86400;
  snprintf(timestr,sizeof(timestr),"%02d%02d%02d.%03d",
           (int)(tv.tv_sec/3600),(int)(tv.tv_sec%3600/60),(int)(tv.tv_sec%60),
           (int)(tv.tv_usec/1000));
  lattdeg=(int)lat;
  longdeg=(int)lon;
  lattmin=lat*60.0-lattdeg*60.0;
  longmin=lon*60.0-longdeg*60.0;
  snprintf(linebuf,sizeof(linebuf),"GPRMC,%s,A,%02d%07.4f,%c,%03d%07.4f,%c,%1f,%.1f,%s,,,E",
           timestr,lattdeg,lattmin,lattdeg>0?'N':'S',longdeg,longmin,longdeg>0?'E':'W',speed/1.852,heading<0?(heading+360):heading,
     datestr);
  l=strlen(linebuf);
  for(i=0,chksum=0;i<l;i++) {
    chksum^=linebuf[i];
  }
  printf("$%s*%02X\r\n",linebuf,chksum&0xff);
  fflush(stdout);
}

/*
 * helper functions for converting nmea coordinates
 */
double to_seconds(char *str)
{
  int pointindex = strcspn(str,".");
  double mins, degs;
  if (pointindex <2)
    return 0;
  mins = atof(str+pointindex-2);
  str[pointindex-2]=0;
  degs = atof(str);
  return degs * 3600.0+mins*60.0;
  
}


/*
 * helper functions for converting nmea coordinates
 */
static double parse_lonlat(char *llstr)
{
  int commaind=strcspn(llstr,",");
  char *side=llstr+commaind;
  int do_neg=0;
  double ret;
  if (side[0]==',') {
    side[0]=0;
    if ((side[1]=='S')||(side[1]=='W')) {
      do_neg=1;
    }
  }
  ret=to_seconds(llstr)/3600.0;
  return do_neg?-ret:ret;
}


int main(int argc, char **argv)
{
  char buf[256];
  int heading_found=0;
  int heading=0;
  int spd=0;
  if (argc != 3) {
    fprintf(stderr,"Usage: %s lattitude-in-nmea-format(e.g. 5055.76433,N) longitude-in-nmea-format (e.g. 00710.92521,E) >nmea-output <dead-reckoning-input\n",argv[0]);
  }
  lat=parse_lonlat(argv[1]);
  lon=parse_lonlat(argv[2]);
  union {
    heading_evt_t hd;
    magnetometer_evt_t magn;
    evt_head_t head;
    evt_speed_t speed;
    char buf[256];
  } evt;

  while(fread(&evt,1,sizeof(evt.head),stdin)) {
    fread(evt.buf+sizeof(evt.head),1,evt.head.len-sizeof(evt.head),stdin);
    switch(evt.head.type) {
      case EVENT_MAGNETOMETER:
         break;
      case EVENT_HEADING:
         printf("heading: %.1f\n",evt.hd.heading10/10.0);
         heading=evt.hd.heading10;
         heading_found=1;
         break;
      case EVENT_TACHO:
         if (heading_found) {
         update_position(rotlen,heading/10.0);
         out_nmealine(((double)(evt.head.tv.tv_sec))+(double)(evt.head.tv.tv_usec)/1000000.0,lon,lat,heading/10.0,spd/100.0*3.6);
         }
         break;
      case EVENT_SPEED:
         spd=evt.speed.speed;
         break;
      default:
         printf("type: %d\n",(int)evt.head.type);
         break;
    } 
  }
  return 0;
}
