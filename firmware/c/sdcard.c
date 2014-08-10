#include <pic18fregs.h>
#include <stdint.h>
#include <delay.h>
#include "sdcard.h"
#include "main.h"
//#define DEBUG_VIA_USB
#ifndef DEBUG_VIA_USB
#define SD_DEBUG_OUT(x)
#else
#define SD_DEBUG_OUT(x) usb_ep4_put(x)
#endif

#define SDCARD_CLK PORTAbits.RA5 // RP2
#define SDCARD_CS PORTAbits.RA1  // RP1
#define SDCARD_MOSI PORTAbits.RA2 // ouch, no RP
#define SDCARD_MISO PORTBbits.RB0 // RP3
struct {
  unsigned HC :1;
  unsigned ready :1;
  unsigned writing :1;
  unsigned busy :1;
  unsigned wakeup :1;
} sdstatus;
static uint16_t remaining;
static uint32_t last_block;
static unsigned char busybuf[16];
static uint8_t busybufpos;
#define SPI_TRANSACT_BIT_SLOW(bit, val, valret) \
  do { \
  SDCARD_CLK = 0; \
  if (val & (1<<bit))   SDCARD_MOSI = 1; else SDCARD_MOSI = 0;	\
  delay10tcy(1); \
  SDCARD_CLK = 1; \
  delay10tcy(1); \
  if (SDCARD_MISO) \
    valret |= (1 << bit); \
  } while(0)

#define SPI_TRANSACT_BIT(bit, val, valret) \
  do { \
  SDCARD_CLK = 0; \
  if (val & (1<<bit))   SDCARD_MOSI = 1; else SDCARD_MOSI = 0;	\
  SDCARD_CLK = 1; \
  if (SDCARD_MISO) \
    valret |= (1 << bit); \
  } while(0)


static void sdcard_wakeup();

static unsigned char spi_transact_byte(unsigned char transmit)
{
  unsigned char ret = 0;
  if (sdstatus.ready) {
    SPI_TRANSACT_BIT(7, transmit, ret);
    SPI_TRANSACT_BIT(6, transmit, ret);
    SPI_TRANSACT_BIT(5, transmit, ret);
    SPI_TRANSACT_BIT(4, transmit, ret);
    SPI_TRANSACT_BIT(3, transmit, ret);
    SPI_TRANSACT_BIT(2, transmit, ret);
    SPI_TRANSACT_BIT(1, transmit, ret);
    SPI_TRANSACT_BIT(0, transmit, ret);
  } else {
    SPI_TRANSACT_BIT_SLOW(7, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(6, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(5, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(4, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(3, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(2, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(1, transmit, ret);
    SPI_TRANSACT_BIT_SLOW(0, transmit, ret);
    delay10tcy(1);
  }
  return ret;
}

void sdcard_io_init()
{
  ANCON0 |= (1 << 1) | (1 << 2) | (1 << 4);
  SDCARD_CS = 1;
  SDCARD_CLK = 0;
  SDCARD_MOSI = 0;
  sdstatus.ready = 0;
  last_block=2;
  busybufpos = 0;
}
static const uint8_t cmd0[]={0x40,0x0,0x0,0x0,0x00,0x95};
static const uint8_t cmd8[]={0x48,0x0,0x0,0x1,0xaa,0x87};
static const uint8_t cmd16[]={0x50,0x0,0x0,0x2,0x00,0xFF};
static const uint8_t cmd55[]={0x40+55,0x0,0x0,0x0,0x0,0x65};
static const uint8_t acmd41[]={0x40+41,0x40 /* bit 6 = sdhc/xc support */,0x00,0x00,0x00,0x77};
static const uint8_t cmd58[]={0x40+58,0x0,0x0,0x0,0x0,0xff};

static uint8_t get_r1()
{
  uint8_t ret;
  uint8_t i;
  for(i=0;i<64;i++) {
    ret=spi_transact_byte(0xff);
    if (ret != 0xff)
      return ret;
  }
  return ret;
}

unsigned char sdcard_reset()
{
  unsigned char i;
  unsigned char r1;
  SDCARD_CS=1;
  for(i=0;i<10;i++)
    spi_transact_byte(0xff);
  SDCARD_CS=0;
  for(i=0;i<sizeof(cmd0);i++) {
    spi_transact_byte(cmd0[i]);
  }
  for(i=0;i<64;i++) {
    delay1ktcy(1);
    r1=spi_transact_byte(0xff);
    if (r1 == 0x01)
      break;
  }
  SD_DEBUG_OUT(r1);
  return (r1 == 0x1);

}

void sdcard_idle()
{
  if (sdstatus.busy) {
    if (spi_transact_byte(0x0) != 0) {
      sdstatus.busy = 0;
      SDCARD_CS = 1;
    }
  } else if (sdstatus.wakeup) {
    sdcard_wakeup();
  }
}

unsigned char sdcard_put_byte(uint8_t data)
{
  unsigned char ret = 0;
  uint8_t i;
  if (!sdstatus.ready)
    return 0;
  if (sdstatus.busy) {
    if (busybufpos < sizeof(busybuf)) {
      busybuf[busybufpos] = data;
      busybufpos++;
      return 1;
    }
    return 0;
  }
  if (!sdstatus.writing) {
    sdcard_start_write(last_block);
    if (sdstatus.writing) {
      last_block++;
      if (busybufpos > 0) {
	for(i = 0; i< busybufpos;i++) {
	  spi_transact_byte(busybuf[i]);
	}
	remaining-=busybufpos;
	busybufpos = 0;
      }
    } else {
      if (busybufpos < sizeof(busybuf)) {
	busybuf[busybufpos] = data;
	busybufpos++;
	return 1;
      }
    }
  }
  if (!sdstatus.writing)
    return 0;
  spi_transact_byte(data);
  remaining--;
  if (remaining == 0) {
    unsigned char r,i;
    /* dummy CRC */
    spi_transact_byte(0xff);
    spi_transact_byte(0xff);
    r = spi_transact_byte(0xff);
    r &= 31;
    SD_DEBUG_OUT(r);
    sdstatus.busy = 1;
    if (r == 5) {
      SD_DEBUG_OUT(r);
      ret = 1;
    }
    //SDCARD_CS = 1;
    sdstatus.writing = 0;
  }
  return ret;
}

unsigned char sdcard_start_write(uint32_t block)
{
  unsigned char r1;
  if (sdstatus.ready == 0)
    return 0;
  if (sdstatus.busy)
    return 0;
  spi_transact_byte(0xff);
  SDCARD_CS = 0;
  spi_transact_byte(0x40+24);
  spi_transact_byte((block >> 24) & 255);
  spi_transact_byte((block >> 16) & 255);
  spi_transact_byte((block >>  8) & 255);
  spi_transact_byte(block & 255);
  spi_transact_byte(0xff);
  r1 = get_r1();
  SD_DEBUG_OUT(r1);
  if (r1 != 0)
    return 0;
  /* start block token */
  spi_transact_byte(0xfe);
  remaining = 512;
  sdstatus.writing = 1;
  usb_ep4_flush();
  return 1;
}

unsigned char sdcard_init()
{
  unsigned char i, r1;
  sdstatus.ready = 0;
  sdstatus.writing = 0;
  sdstatus.HC = 0;
  sdstatus.busy = 0;
  sdstatus.wakeup = 0;
  busybufpos = 0;
  if (!sdcard_reset()) {
    usb_ep4_flush();
    return 0;
  }
  SDCARD_CS = 1;
  spi_transact_byte(0xff);
  //delay1ktcy(1);
  SDCARD_CS = 0;
  for(i=0;i<sizeof(cmd8);i++) {
    spi_transact_byte(cmd8[i]);
  }
  r1 = get_r1();
  SD_DEBUG_OUT(r1);
  for(i=1;i<5;i++) {
    r1=spi_transact_byte(0xff);
    SD_DEBUG_OUT(r1);
  }
  SDCARD_CS=1;
  sdstatus.wakeup = 1;
  return 1;
}

static void sdcard_wakeup()
{
  unsigned char i, r1;
  SDCARD_CS=1;
  spi_transact_byte(0xff);
  delay1ktcy(1);
  SDCARD_CS=0;
  for(i=0;i<sizeof(cmd55);i++) {
    spi_transact_byte(cmd55[i]);
  }
  r1=get_r1();
  SD_DEBUG_OUT(r1);
  SDCARD_CS=1;
  spi_transact_byte(0xff);
  delay1ktcy(1);
  SDCARD_CS=0;
  for(i=0;i<sizeof(acmd41);i++) {
    spi_transact_byte(acmd41[i]);
  }
  delay10ktcy(1);
  r1=get_r1();
  SD_DEBUG_OUT(r1);
  SDCARD_CS=1;
  if (r1!=0)
    return;
  sdstatus.wakeup = 0;
  SDCARD_CS=1;
  spi_transact_byte(0xff);
  delay1ktcy(1);
  SDCARD_CS=0;
  for(i=0;i<sizeof(cmd58);i++) {
    spi_transact_byte(cmd58[i]);
  }
  r1 = get_r1();
  if (r1 != 0) {
    SDCARD_CS = 1;
    return;
  }
  r1=spi_transact_byte(0xff);
  SD_DEBUG_OUT(r1);
  if (r1 & 0x40)
    sdstatus.HC=1;
  else
    sdstatus.HC=0;
  for(i=0;i<3;i++) {
    r1=spi_transact_byte(0xff);
    SD_DEBUG_OUT(r1);
  }
  SDCARD_CS = 1;
  if (sdstatus.HC == 0) {
    spi_transact_byte(0xff);
    SDCARD_CS = 0;
    for(i=0;i<sizeof(cmd16);i++) {
      spi_transact_byte(cmd16[i]);
    }
    r1=get_r1();
    SDCARD_CS = 1;
    if (r1 != 0) {
      return;
    }
  }
  sdstatus.ready = 1;
}

