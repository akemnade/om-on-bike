#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "evt.h"
#include "compass.h"

int main(int argc, char **argv)
{
  magnetometer_evt_t mag;
  int x,y,z,xr,yr,zr;
  char buf[80];
  init_compass_calib();
  while(fgets(buf,sizeof(buf),stdin)) {
    if (3==sscanf(buf,"%d %d %d",&x,&y,&z)) {
      mag.x=x;
      mag.y=y;
      mag.z=z;
      calculate_heading(&mag,&xr,&yr,&zr); 
      printf("%d %d %d\n",xr,yr,zr);
    }
  }
}
