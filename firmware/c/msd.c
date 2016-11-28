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


#include <pic18fregs.h>
#include <stdint.h>
#include <delay.h>
#include <string.h>
#include "usb-drv.h"
#include "msd.h"
#include "main.h"
/*
#include "i2cif.h"
#include "sdcard.h"
#include "main.h"
*/
#define MSD_HEADER_LEN 15
#define INQUIRY 0x12
#define MODE_SENSE 0x1a
#define START_STOP_UNIT 0x1b
#define READ_CAPACITY 0x25
#define READ_6 0x08
#define READ_10 0x28
#define REQUEST_SENSE 0x3
#define TEST_UNIT_READY 0x00
//#define READ_12

#define MSD_IDLE 0
#define MSD_DATA_READ 1
#define MSD_CSW 2
#define MSD_ERROR 3

#define SCSI_SENSE_KEY_NOT_READY 0x2
#define SCSI_SENSE_KEY_MEDIUM_ERROR 0x3
#define SCSI_SENSE_KEY_ILLEGAL_REQUEST 0x5
#define SCSI_SENSE_KEY_UNIT_ATTENTION 0x6
#define SCSI_SENSE_KEY_DATA_PROTECT 0x7

#define SCSI_ASC_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE 0x21
#define SCSI_ASC_INVALID_COMMAND_OPERATION_CODE 0x20
#define SCSI_ASC_INVALID_FIELD_IN_COMMAND_PACKET 0x24
#define SCSI_ASC_LOGICAL_UNIT_NOT_SUPPORTED 0x25


#define SCSI_ASC_PERIPHERAL_DEVICE_WRITE_FAULT 0x03
#define SCSI_ASC_UNRECOVERED_READ_ERROR 0x11

#define SCSI_ASC_WRITE_ERROR 0x0c
#define SCSI_ASC_MEDIUM_NOT_PRESENT 0x3a

#define SCSI_ASC_WRITE_PROTECTED 0x27

uint8_t __at (MSD_EPOUTB)msd_epoutb[64];
static uint32_t cbw_length;

static uint8_t msd_state = MSD_IDLE;
static uint8_t __at (MSD_EPINB) msd_epinb[64];
static uint8_t __at (MSD_CSWB) msd_csw[13]; /* = {'U','S','B','S',
					   0,0,0,0,
					   0,0,0,0,
					   0}; */
static uint8_t sense_key;
static uint8_t additional_sense_code;
static uint32_t read_block_nr;
static uint16_t blocks_to_read;
static uint16_t block_pos;
void msd_ep_init()
{
  msd_csw[0] = 'U';
  msd_csw[1] = 'S';
  msd_csw[2] = 'B';
  msd_csw[3] = 'S';
  msd_state = MSD_IDLE;
}

static void start_read_block();

const uint8_t inquiry_desc[]={
  0, /* no cdrom */
  0x80, /*removable */
  2,2,31,0,0,0,
  'A','K',0,0,0,0,0,0,
  'B','i','k','e',' ','S','D',0, 0,0,0,0,0,0,0,0,
  '0','.','1',0
};

static void prepare_csw(uint32_t residue, uint8_t status)
{
  msd_state = MSD_CSW;
  msd_csw[8] = residue >> 0;
  msd_csw[9] = residue >> 8;
  msd_csw[10] = residue >> 16;
  msd_csw[11] = residue >> 24;
  msd_csw[12] = status;
}

static void return_inquiry(uint8_t len)
{
  //while(1);
  memcpypgm2ram(msd_epinb,inquiry_desc,sizeof(inquiry_desc));
  msd_send_epin_cb(msd_epinb,sizeof(inquiry_desc)); 
  prepare_csw(0,0);
  //start_send_from_rom(inquiry_desc, sizeof(inquiry_desc));
}  


static void send_csw()
{
  msd_send_epin_cb(msd_csw, sizeof(msd_csw)); 
}

void msd_ep_handle_in()
{
 
  if (msd_state == MSD_DATA_READ) {
    block_pos += 64;
    if (block_pos == 512) {
      read_block_nr++;
      blocks_to_read--;
      if (blocks_to_read == 0)
	prepare_csw(0,0);
      else
	start_read_block();
    } else {
      msd_send_epin_cb(msd_readblockbuf + block_pos, 64);
    }
  }
  if (msd_state == MSD_CSW) {
    send_csw();
    msd_state = MSD_IDLE;
  }
}


static void read_capacity()
{
  uint32_t blocks = msd_get_num_blocks_cb();
  /* want maximum block number */
  blocks--;
  msd_epinb[0] = blocks >> 24;
  msd_epinb[1] = blocks >> 16;
  msd_epinb[2] = blocks >> 8;
  msd_epinb[3] = blocks >> 0;
  msd_epinb[4] = MSD_BLOCK_SIZE >> 24;
  msd_epinb[5] = MSD_BLOCK_SIZE >> 16;
  msd_epinb[6] = MSD_BLOCK_SIZE >> 8;
  msd_epinb[7] = (MSD_BLOCK_SIZE >> 0) & 0xff;
  msd_send_epin_cb(msd_epinb, 8);
  prepare_csw(0,0);
}

static void request_sense()
{
  uint8_t answer_len = cbw_length < 18 ? cbw_length : 18;
  memset(msd_epinb,0,18);
  msd_epinb[0] = 0x70;
  msd_epinb[2] = sense_key;
  msd_epinb[7] = 0xa;
  msd_epinb[12] = additional_sense_code; 
  msd_send_epin_cb(msd_epinb,answer_len);
  prepare_csw(0,0);
}

static void start_read_block()
{
  uint8_t status = msd_read_cb(read_block_nr);
  switch(status) {
  case MSD_READY:
    break;
  case MSD_MEDIUM_NOT_PRESENT:
    /* not ready */
    sense_key = SCSI_SENSE_KEY_NOT_READY;
    /* medium not present */
    additional_sense_code = SCSI_ASC_MEDIUM_NOT_PRESENT;
    prepare_csw((uint32_t)blocks_to_read * MSD_BLOCK_SIZE, 1);
    break;
  case MSD_READ_ERROR:
    sense_key = SCSI_SENSE_KEY_MEDIUM_ERROR;
    additional_sense_code = SCSI_ASC_UNRECOVERED_READ_ERROR;
    prepare_csw((uint32_t)blocks_to_read * MSD_BLOCK_SIZE, 1);
    return;
  }
  block_pos = 0;
  msd_send_epin_cb(msd_readblockbuf, 64);
}

void msd_ep_handle_out(uint8_t len)
{
  // TODO: check_valid
  //  return_inquiry(len);
  //return;
  // send_epout(msd_epoutb+MSD_HEADER_LEN,1);
  // return;
  uint8_t status;
  msd_csw[4] = msd_epoutb[4];
  msd_csw[5] = msd_epoutb[5];
  msd_csw[6] = msd_epoutb[6];
  msd_csw[7] = msd_epoutb[7];
  cbw_length = msd_epoutb[8];
  cbw_length |= ((uint32_t) msd_epoutb[9] << 8);
  cbw_length |= ((uint32_t) msd_epoutb[10] << 16);
  cbw_length |= ((uint32_t) msd_epoutb[11] << 24);
  switch(msd_epoutb[MSD_HEADER_LEN]) {
  case INQUIRY:
    return_inquiry(len);
    break;
  case MODE_SENSE:
    msd_epinb[0] = 3;
    msd_epinb[1] = 0; /* SBC */
#ifdef MSD_ALLOW_WRITE
    msd_epinb[2] = 0; 
#else
    msd_epinb[2] = 0x80;
#endif
    msd_epinb[3] = 0;
    msd_send_epin_cb(msd_epinb, 4);
    prepare_csw(0,0);
    break;
  case READ_6:
    read_block_nr = msd_epoutb[18] ;
    read_block_nr |= (uint32_t)msd_epoutb[17] << 8;
    read_block_nr |= (uint32_t)msd_epoutb[16] << 16;
    blocks_to_read = msd_epoutb[19];
    msd_state = MSD_DATA_READ;
    start_read_block();
    break;
  case READ_10:
    read_block_nr = msd_epoutb[20];
    read_block_nr |= (uint32_t)msd_epoutb[19] << 8;
    read_block_nr |= (uint32_t)msd_epoutb[18] << 16;
    read_block_nr |= (uint32_t)msd_epoutb[17] << 24;
    blocks_to_read = msd_epoutb[23];
    blocks_to_read |= (uint16_t)msd_epoutb[22] << 8;
    msd_state = MSD_DATA_READ;
    start_read_block();
    // case READ_10:
    //case READ_12:
    break;
  case READ_CAPACITY:
    read_capacity();
    //  case READ_HEADER:
    break;
  case REQUEST_SENSE:
    request_sense();
    break;
  case TEST_UNIT_READY:
    status = msd_unit_ready_cb();
    if (status != MSD_READY) {
      prepare_csw(0, 1);
      /* not ready */
      sense_key = SCSI_SENSE_KEY_NOT_READY;
      /* medium not present */
      additional_sense_code = SCSI_ASC_MEDIUM_NOT_PRESENT;
    } else {
      prepare_csw(0, 0);
    }
    break;
  default:
    sense_key = SCSI_SENSE_KEY_ILLEGAL_REQUEST;
    additional_sense_code = SCSI_ASC_INVALID_COMMAND_OPERATION_CODE;
    /* direction in, no data */
    if ((cbw_length == 0) || (msd_epoutb[12] & 0x80)) {
      msd_ep_stall_in_cb();
    } else {
      msd_ep_stall_out_cb();
    }
    prepare_csw(cbw_length, 1);
    break;
  }
}
