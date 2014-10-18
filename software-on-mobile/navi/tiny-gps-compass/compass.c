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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "evt.h"
#include "compass.h"
#include "fxoss.h"


static int calib_matrix[3][3]={{4096,0,0},{0,4096,0},{0,0,4096}};
static int calib_xoff, calib_yoff;
static float calib_headingoff=0;
static int calib_flip=1;
static FILE *logfile=NULL;
static char *compass_dev;
static int compass_active;
static float last_heading;
static pthread_t compass_thread;
static pthread_mutex_t heading_mutex = PTHREAD_MUTEX_INITIALIZER;

float calculate_heading(magnetometer_evt_t *mag, int *xr, int *yr, int *zr)
{
  int x=mag->x;
  int y=mag->y;
  int z=mag->z;
  int xn,yn,zn;
  float heading;
  xn=calib_matrix[0][0]*x+calib_matrix[0][1]*y+calib_matrix[0][2]*z;
  yn=calib_matrix[1][0]*x+calib_matrix[1][1]*y+calib_matrix[1][2]*z;
  zn=calib_matrix[2][0]*x+calib_matrix[2][1]*y+calib_matrix[2][2]*z;
  x=xn/4096;
  y=yn/4096;
  z=zn/4096;
  if (xr)
    *xr=x;
  if (yr)
    *yr=y;
  if (zr)
    *zr=z;
  heading=atan2((float)calib_yoff+y,(float)calib_xoff+x);
  if (calib_flip==-1)
    heading=-heading;
  heading=heading*180/M_PI;
  heading+=calib_headingoff;
  if (heading<0)
    heading+=360;
  if (heading>360)
    heading-=360;
  return heading;
}

static void  calculate_heading_evt(magnetometer_evt_t *mag)
{
  float heading;
  int x,y,z;
  EVT_INIT(heading_evt_t,EVENT_HEADING,hd);
  heading=calculate_heading(mag,&x,&y,&z);
  pthread_mutex_lock(&heading_mutex);
  compass_active=1;
  last_heading=heading;
  pthread_mutex_unlock(&heading_mutex);
  hd.head.tv=mag->head.tv;
  hd.heading10=heading*10;
  distribute_evt(&hd.head);
  fprintf(stderr,"%d.%03d X: %d Y: %d Z: %d %.1f\n",(int)mag->head.tv.tv_sec,(int)mag->head.tv.tv_usec/1000,(int)x,(int)y,(int)z,heading);
  if (logfile)
    fprintf(logfile,"%d.%03d X: %d Y: %d Z: %d %.1f\n",(int)mag->head.tv.tv_sec,(int)mag->head.tv.tv_usec/1000,(int)x,(int)y,(int)z,heading);
}

void *compass_loop(void *data)
{
   unsigned char buf[20];
   int x,y,z;
   EVT_INIT(magnetometer_ext_evt_t,EVENT_MAGNETOMETER_EXT,mag);
   while(1) {
     if (logfile) {
       fprintf(logfile,"opening device\n");
       fflush(logfile);
     }
     int fd=open(compass_dev,O_RDWR);
     struct i2c_rdwr_ioctl_data iod;
     struct i2c_msg msg[2];
     iod.nmsgs=1;
     iod.msgs=msg;
     if (fd<0) {
       if (logfile)  {
	 fprintf(logfile,"opening device failed\n");
	 fflush(logfile);
       }
       compass_active=0;
       sleep(1);
       continue;
     }
     if (logfile) {
       fprintf(logfile,"opening device successful\n");
       fflush(logfile);
     }
     if (!init_compass_i2c(fd)) {
       if (logfile) {
	 fprintf(logfile,"init failed 1\n");
	 fflush(logfile);
       }
       close(fd);
       fd=-1;
       sleep(1);
       continue;
     } 
     while(1) {
       usleep(80000);
       compass_read_raw(fd,&mag); 
       gettimeofday(&mag.head.tv,NULL);
       distribute_evt(&mag.head);
       calculate_heading_evt(&mag);
     
     }
     if (logfile) {
       fprintf(logfile,"reading data failed\n");
       fflush(logfile);
     }
     close(fd);
     fd=-1;
     usleep(100000);
   } 
   return 0;
}

static void read_calib_matrix()
{
  int i;
  char buf[80];
  FILE *f=fopen("calib_matrix","r");
  if (!f)
    return;
  for(i=0;fgets(buf,sizeof(buf),f)&&(i<3);i++) {
    sscanf(buf,"%d %d %d",&calib_matrix[i][0],&calib_matrix[i][1],&calib_matrix[i][2]); 
  }
  fclose(f);
}

static void read_mag_calib()
{
  char buf[80];
  FILE *f=fopen("mag_calib","r");
  if (!f)
    return;
  fgets(buf,sizeof(buf),f);
  sscanf(buf,"%d %d %f %d",&calib_xoff,&calib_yoff,&calib_headingoff,&calib_flip);
  fclose(f);
}

void init_compass_calib()
{
  read_calib_matrix();
  read_mag_calib();
}

void init_compass(int argc, char **argv)
{
  if (argc>0) {
    compass_dev=argv[0];
    if (argc>1) {
      logfile=fopen(argv[1],"a");
    }
    init_compass_calib();
    pthread_create(&compass_thread,
                   NULL,
                   compass_loop,NULL);
  }
}

int compass_get_heading(float*heading)
{
   if (!compass_active) {
     return 0;
   }
   
   pthread_mutex_lock(&heading_mutex); 
   *heading=last_heading;
   pthread_mutex_unlock(&heading_mutex); 
   return 1;
}
