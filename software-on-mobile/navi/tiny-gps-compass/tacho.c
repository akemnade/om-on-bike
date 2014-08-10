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

#include <sys/ioctl.h>
#include <sys/time.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include "evt.h"
#include "tacho.h"

#define DYNPERTACHO 28

static pthread_t tacho_thread;
static pthread_mutex_t tacho_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *logfile;
static char *tacho_dev;
static double tottachodist;
static double totdyndist;
static double rotlen=2.125;
static unsigned int tacholimit=250000;
static int oldtachotimer;
static int olddyntimer;
static int dyncounter;
static int dynpertacho;
static int lastdisplaytime;
static double lastspeed;
static struct timeval lastspeedtime;
static int tacho_active;

static void read_tacho_timings(unsigned char *buf)
{
  unsigned int timer;
  //double lenperiter;
  double speed=0;
  timer=((buf[1]*256)+buf[2])*256+buf[3];
  if ((buf[0]&0x40)||(buf[0]&0x10)) {
    EVT_INIT(evt_tacho_t, EVENT_DYNAMO,evt);
    pthread_mutex_lock(&tacho_mutex);
    gettimeofday(&lastspeedtime,NULL);
    pthread_mutex_unlock(&tacho_mutex);
    evt.head.tv=lastspeedtime;
    evt.counter=timer;
    distribute_evt(&evt.head);
    totdyndist+=(rotlen/DYNPERTACHO);
    dyncounter++;
    if ((((timer-lastdisplaytime)&0xffffff)>(1000000/2))) {
      fprintf(stderr,"dyn %d  %9d %9d us %.1f\n",dyncounter,timer,((timer-olddyntimer)&0xffffff)*2,
              totdyndist);
      lastdisplaytime=timer;
    } 
    if (logfile) {
      fprintf(logfile,"%d.%03d dyn %d  %9d %9d us %.1f\n",
	      (int)lastspeedtime.tv_sec,(int)lastspeedtime.tv_usec/1000,
	      dyncounter,timer,((timer-olddyntimer)&0xffffff)*2,
              totdyndist);

    }
    olddyntimer=timer;
    dynpertacho++;
  } 
  if ((buf[0]&0x80)||(buf[0]&0x20)) {
    unsigned int timediff=((timer-oldtachotimer)&0xffffff)*2;
    if ((timediff>tacholimit)||(dynpertacho>10)) {
      EVT_INIT(evt_tacho_t,EVENT_TACHO,evt);
      EVT_INIT(evt_speed_t,EVENT_SPEED,evts);
      if ((tacholimit==250000) && (timediff<350000)) {
        tacholimit=100000;
      } else if ((tacholimit==100000) && (timediff>350000)) {
        tacholimit=250000;
      }
      speed=(int)timediff;

      speed/=1000000.0;
      speed=rotlen/speed;
      pthread_mutex_lock(&tacho_mutex);
      lastspeed=speed;
      gettimeofday(&lastspeedtime,NULL);
      pthread_mutex_unlock(&tacho_mutex);
      evt.head.tv=lastspeedtime;
      evt.counter=timer;
      distribute_evt(&evt.head);
      evts.head.tv=lastspeedtime;
      evts.speed=speed*100.0;
      distribute_evt(&evts.head);
      tottachodist+=rotlen;

      printf("tacho %9d %9d us %.1f km/h %d %.1f m\n",timer,timediff,
             speed*3.6,dynpertacho,tottachodist);
      if (logfile) {
	fprintf(logfile,"%d.%03d tacho %9d %9d us %.1f km/h %d %.1f m\n",
	       (int)lastspeedtime.tv_sec,(int)lastspeedtime.tv_usec/1000,
	       timer,timediff,
	       speed*3.6,dynpertacho,tottachodist);
	fflush(logfile);
      }
      oldtachotimer=timer;
      dynpertacho=0;
    }
  }
  
}


void *tacho_loop(void *data)
{
  unsigned char buf[256];
  int l,i;
  int pos;
  while(1) {
    int fd;
    if (logfile) {
      fprintf(logfile,"opening device\n");
      fflush(logfile);
    }
    fd=open(tacho_dev,O_RDONLY);
    if (fd<0) {
      if (logfile)  {
	fprintf(logfile,"opening device failed\n");
	fflush(logfile);
      }
      sleep(1);
      continue;
    }
    tacho_active=1;
    pos=0;
    while((l=read(fd,buf+pos,sizeof(buf)-pos))>0) {
      if (buf[0]!=0xfa) {
	pos=0;
	printf("got crap from %s\n",tacho_dev);
      } else {
	pos+=l;
	for(i=0;i<=(pos-6);i+=6) {
	  if (buf[i]==0xfa) {
	    read_tacho_timings(buf+i+1);
	  }
	}
        
	if (pos!=i) {
	  memmove(buf,buf+i,pos-i);
	  pos=pos-i;
	} else {
	  pos=0;
	}
      }
    }
    close(fd);
    tacho_active=0;
    if (logfile) {
      fprintf(logfile,"closing device");
    }
    sleep(2);
  }
}

void init_tacho(int argc, char **argv)
{
  if (argc>0) {
    tacho_dev=argv[0];
    if (argc>1)
      logfile=fopen(argv[1],"a");
    pthread_create(&tacho_thread,NULL,
		   tacho_loop,NULL);
  }
}

int tacho_get_speed(float *speed)
{
  struct timeval tv;
  struct timeval tvs;
  if (!tacho_active)
    return 0;
  pthread_mutex_lock(&tacho_mutex);
  tvs=lastspeedtime;
  *speed=(float)lastspeed;
  pthread_mutex_unlock(&tacho_mutex);
  (*speed)*=(3.6/1.852);
  gettimeofday(&tv,NULL);
  tv.tv_sec-=tvs.tv_sec;
  tv.tv_usec-=tvs.tv_usec;
  if (tv.tv_usec < 0) {
    tv.tv_sec--;
    tv.tv_usec+=1000000;
  }
  if (tv.tv_sec!=0)
    *speed=0;
  return 1;
}
