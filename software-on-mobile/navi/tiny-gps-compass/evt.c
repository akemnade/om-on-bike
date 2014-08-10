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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "evt.h"

static FILE *evtf;
static pthread_mutex_t evt_mutex = PTHREAD_MUTEX_INITIALIZER;
void evt_init()
{
  if (!evtf)
    evtf=popen("multistream 2948","w");
}

void distribute_evt(evt_head_t *evt)
{
  pthread_mutex_lock(&evt_mutex);
  if (evtf) {
    fwrite(evt,1,evt->len,evtf);
  }
  pthread_mutex_unlock(&evt_mutex);
}
