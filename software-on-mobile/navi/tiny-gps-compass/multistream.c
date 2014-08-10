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

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>     
#include <termios.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h> 
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "evt.h"
#include "compass.h"
#include "tacho.h"

#ifndef STANDALONE
static int adjlast;

static int my_split(char *bigstr, char **needle, char *delim, int maxsplit)
{
  int i;
  for(i=0;i<maxsplit;i++) {
    int n;
    needle[i]=bigstr;
    n=strcspn(bigstr,delim);
    if (bigstr[n]==0) {
      i++;
      break;
    }
    bigstr[n]=0;
    bigstr+=n;
    bigstr++;
  }
  return i;
}

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



void set_time(char *timestr, char *datestr, double longg, double latt,
	      void *data)
{
  struct tm tm;
  struct tm tm_bak;
  struct tm gmtm;
  struct timeval tv;
  struct timeval tvnew;
  struct timeval delta;
  time_t t,t2;
 
  char buf[10];
  buf[2]=0;
  gettimeofday(&tv,NULL);
  if (strlen(datestr)!=6) {
    return;
  }
  if (strlen(timestr)<6)
    return;
  buf[0]=datestr[0];
  buf[1]=datestr[1];
  tm.tm_mday=strtoul(buf,NULL,10);
  buf[0]=datestr[2];
  buf[1]=datestr[3];
  tm.tm_mon=strtoul(buf,NULL,10)-1;
  buf[0]=datestr[4];
  buf[1]=datestr[5];
  tm.tm_year=strtoul(buf,NULL,10);
  if (tm.tm_year < 1990)
    tm.tm_year+=100;
  buf[0]=timestr[0];
  buf[1]=timestr[1];
  tm.tm_hour=strtoul(buf,NULL,10);
  buf[0]=timestr[2];
  buf[1]=timestr[3];
  tm.tm_min=strtoul(buf,NULL,10);
  buf[0]=timestr[4];
  buf[1]=timestr[5];
  tm.tm_sec=strtoul(buf,NULL,10);
  tm.tm_isdst=0;
#ifdef DEBUG_NMEA_TIME
  fprintf(stderr,"got time: %s %s\n",datestr, timestr);
#endif
  tm_bak=tm;
  t=mktime(&tm_bak);
  gmtm=*gmtime(&t);
  t2=(gmtm.tm_hour*60+gmtm.tm_min)-(tm.tm_hour*60+tm.tm_min);
  if (t2 < -12*60)
    t2+=24*60;
  if (t2 >12*60)
    t2-=24*60;
  t2*=60;
#ifdef DEBUG_NMEA_TIME
  fprintf(stderr,"t: %d t2: %d, t2-t: %d\n",(int)t,(int)t2,(int)(t2-t));
#endif
  t=t-t2;
  tvnew.tv_sec=t;
  tvnew.tv_usec=0;
#ifdef DEBUG_NMEA_TIME
  fprintf(stderr,"ctime: %d %s\n",(int)t,asctime(localtime(&t)));
#endif
  if (strlen(timestr)>7) {
    int lstr;
    const int factor[]={0,100000,10000,1000,100,10,1};
    timestr+=7;
    lstr=strlen(timestr);
    if (lstr <sizeof(factor))
      tvnew.tv_usec=factor[lstr]*atoi(timestr);
  }
#ifdef DEBUG_NMEA_TIME
  fprintf(stderr,"sec: %d, usec: %d\n",(int)tvnew.tv_sec,
	  (int)tvnew.tv_usec);
#endif
  delta.tv_usec = tvnew.tv_usec-tv.tv_usec;
  delta.tv_sec= tvnew.tv_sec-tv.tv_sec;
  if ((delta.tv_usec <0) && (delta.tv_sec>0)) {
    delta.tv_usec+=1000000;
    delta.tv_sec--;
  }
  if ((delta.tv_sec < 0)&&(delta.tv_usec > 0)) {
    delta.tv_usec-=1000000;
    delta.tv_sec++;
  }
#ifdef DEBUG_NMEA_TIME
  fprintf(stderr,"delta: sec: %d usec: %d\n",(int)delta.tv_sec,
	  (int)delta.tv_usec);
#endif
  if ((delta.tv_sec >2) || (delta.tv_sec < -5)) {
#ifdef DEBUG_NMEA_TIME
    fprintf(stderr,"setting clock\n");
#endif
    settimeofday(&tvnew,NULL);
  } else {
    adjlast++;
    if (adjlast&1) {
#ifdef DEBUG_NMEA_TIME
    fprintf(stderr,"slowly adjusting clock\n");
#endif
    if (adjtime(&delta,NULL))
        perror("adjtime");
    }
  }
}

int proc_gps_input(int l, int *bufpos, int buflen, char *buf,
                   void (*gpsproc)(char *ptime,char *date,double longg, double latt,void *)
, void *data, char *writeptr)
{
  char *endp;
  int wlen=0;
  
  if (l<=0) {
    perror("proc_gps_input: ");
    return l;
  }

  /* fprintf(stderr,"read %d\n",l); */
  (*bufpos)+=l;
  while ((endp=memchr(buf,'\n',*bufpos))) {
    int readlen;
    memcpy(writeptr,buf,endp-buf+1);
    *endp=0;
    if (strncmp(buf,"$GPRMC",6)==0) {
      char linebuf[256];
      char *fields[13];
      char headingbuf[10];
      char speedbuf[10];
      strcpy(linebuf,buf);
      int numfields=my_split(linebuf,fields,",",13);
#ifdef DEBUG_NMEA_TIME
      fprintf(stderr,"$GPRMC found\n");
#endif
      if ((numfields == 12) || (numfields == 13))  {
        double lattsec;
        double longsec;
        float heading;
	float speed;
        if (compass_get_heading(&heading)) {
	  snprintf(headingbuf,sizeof(headingbuf),"%.1f",heading); 
        } else {
	  headingbuf[0]=0; 
        } 
	if (tacho_get_speed(&speed)) {
	  snprintf(speedbuf,sizeof(speedbuf),"%.1f",speed);
	} else {
	  speedbuf[0]=0;
	}
	if (1){ 
           int i;
           int chksum;
           char *chkptr;
           if ((chkptr=strchr(fields[numfields-1],'*'))) {
             if (strlen(chkptr)>2) {
               char chkbuf[3];  
               chksum=strtoul(chkptr+1,NULL,16);
               for(i=0;i<strlen(fields[8]);i++) {
                 chksum^=(fields[8])[i];
               } 
               for(i=0;i<strlen(headingbuf);i++) {
                 chksum^=headingbuf[i];
               }
	       if (speedbuf[0] != 0) {
		 for(i=0;i<strlen(fields[7]);i++) {
		   chksum^=(fields[7])[i];
		 } 
		 for(i=0;i<strlen(speedbuf);i++) {
		   chksum^=speedbuf[i];
		 }
	       }
	       snprintf(chkbuf,sizeof(chkbuf),"%02X",0xff&(int)chksum);
               chkptr[1]=chkbuf[0];
               chkptr[2]=chkbuf[1];
             } 
           }
           fields[8]=headingbuf;
	   if (speedbuf[0])
	     fields[7]=speedbuf;
           for(i=0;i<numfields;i++) {
             if (i) {
               writeptr[0]=',';
               wlen++;
               writeptr++;
             }
             strcpy(writeptr,fields[i]);
             wlen+=strlen(fields[i]);
             writeptr+=strlen(fields[i]);
           } 
           *writeptr='\n';
           writeptr++;
           wlen++; 
        } else {
          wlen+=(endp-buf)+1;
          writeptr+=(endp-buf)+1;
        } 
        longsec=to_seconds(fields[5]);
        lattsec=to_seconds(fields[3]);
        if ((fields[4])[0] == 'S')
          lattsec=-lattsec;
        if ((fields[6])[0] == 'W')
          longsec=-longsec;
        
        gpsproc(fields[1],fields[9],longsec,lattsec,data);
      }  else {
        wlen+=(endp-buf)+1;
        writeptr+=(endp-buf)+1;
      }
    } else {
      wlen+=(endp-buf)+1;
      writeptr+=(endp-buf)+1;
    }
    *endp='\n';
    endp++;
    readlen=endp-buf;
    
    if (readlen != *bufpos)
      memmove(buf,endp,(*bufpos)-readlen);
    (*bufpos)-=readlen;
  }
  return wlen;
}

#endif

int main(int argc, char **argv)
{
  fd_set rfds;
  fd_set wfds_bak;
  fd_set wfds;
  char buf[256];
  int bufpos=0;
  int max_wfds=0;
  int do_duplex=0;
  int x;
  int sock=socket(AF_INET,SOCK_STREAM,0);
  struct sockaddr_in addr;
  if ((argc<2)||(!strcmp(argv[1],"--help"))) {
    printf("Usage: %s port\n",argv[0]);
    return 1;
  }
  do_duplex=1;
  signal(SIGPIPE,SIG_IGN);
#ifndef STANDALONE
  evt_init();
  init_compass(argc-2,argv+2);
  init_tacho(argc-4,argv+4);
#endif
  x=1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &x, sizeof (x));

  addr.sin_port=atoi(argv[1]);
  addr.sin_port=htons(addr.sin_port);
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=INADDR_ANY;
  if (bind(sock,(struct sockaddr *)&addr,sizeof(addr))) {
    fprintf(stderr,"cannot bind port %s\n",argv[1]);
    return 1;
  }
  listen(sock,1);
  FD_ZERO(&wfds_bak);
  while(1) {
    int maxfd;
    if (do_duplex) {
      rfds=wfds_bak;
    } else {
      FD_ZERO(&rfds);
    }
    FD_SET(sock,&rfds);
    FD_SET(0,&rfds);
    wfds=wfds_bak;
    maxfd=sock;
    if (max_wfds > sock) {
      maxfd=max_wfds;
    }
    if (0<select(maxfd+1,&rfds,NULL,NULL,NULL)) {
      if (FD_ISSET(sock,&rfds)) {
	int newsock=accept(sock,NULL,0);
	fprintf(stderr,"accepting new connection\n");
	if (newsock>0)
	  FD_SET(newsock,&wfds_bak);
	if (newsock>max_wfds)
	  max_wfds=newsock;
      }
      if (FD_ISSET(0,&rfds)) {
	struct timeval tv={0,0};
	int i,l;
        char writebuf[512];
        char *writeptr=writebuf;
	l=read(0,buf+bufpos,sizeof(buf)-bufpos);
        if (l<0) {
          if (errno != EINTR) { 
            perror("stdin read: ");
            return 0;
          }
        } else if (l==0) {
          fprintf(stderr,"EOF on stdin\n");
          return 0;
        } else {
#ifndef STANDALONE
	l=proc_gps_input(l,&bufpos,sizeof(buf),buf,set_time,NULL,writebuf);
        if (sizeof(buf)==bufpos) {
          fprintf(stderr,"throwing away %d bytes without newline\n",
                  bufpos);
          bufpos=0;
          l=sizeof(buf);
        }
#else
        writeptr=buf;
#endif
        if ((l>0)&&(0<select(maxfd+1,NULL,&wfds,NULL,&tv))) {
	  for(i=1;i<=max_wfds;i++) {
	    if (FD_ISSET(i,&wfds)) {
	      int lwrite=write(i,writeptr,l);
#ifdef DEBUG_FLOW
	      fprintf(stderr,"written %d: %d\n",i,lwrite);
#endif
	      if (lwrite<=0) {
		perror("write: ");
		fprintf(stderr,"closing %d\n",i);
		close(i);
		FD_CLR(i,&wfds_bak);
		if (i==max_wfds)
		  max_wfds--;
	      } 
	    }
	  }
	}
        }
      }
      if (do_duplex) {
          int i;
        for(i=sock+1;i<=max_wfds;i++) {
          if ((FD_ISSET(i,&wfds_bak))&&(FD_ISSET(i,&rfds))) {
            char rbuf[256];
            int rlen=read(i,&rbuf,sizeof(rbuf));
            if (rlen<=0) {
              perror("read");
              fprintf(stderr,"closing %d\n",i);
              close(i);
              FD_CLR(i,&wfds_bak);
            } else {

              int wlen=write(1,rbuf,rlen);
              tcdrain(1);
#ifdef DEBUG_FLOW
              fprintf(stderr,"written back %d/%d\n",wlen,rlen);
#endif
            }
          }
        }
      }
    } else {
      perror("select: ");
    }
    
  }
}
