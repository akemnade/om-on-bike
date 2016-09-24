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
#define SDCARD_CLK LATAbits.LATA5 // RP2
#define SDCARD_CS LATAbits.LATA1  // RP1
#define SDCARD_MOSI LATAbits.LATA2 // ouch, no RP
#define SDCARD_MISO PORTBbits.RB0 // RP3
#define SDCARD_POWER_OFF  LATBbits.LATB3 = 1
#define SDCARD_POWER_ON LATBbits.LATB3 = 0
struct {
  unsigned HC :1;
  unsigned ready :1;
  unsigned writing :1;
  unsigned reading :1;
  unsigned busy :1;
  unsigned wakeup :1;
  unsigned err :1;
  unsigned powered :1;
} sdstatus;
static uint16_t remaining;
uint32_t sd_last_block;
static unsigned char busybuf[16];
static uint8_t busybufpos, resettries;
uint8_t asmtmp, asmtmp2;

extern void quickread();


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
static unsigned char  sdcard_do_reset();
static unsigned char spi_transact_byte(unsigned char transmit) __wparam
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
    delay10tcy(10);
  }
  return ret;
}
unsigned char sdcard_status()
{
  unsigned char __data* x = (unsigned char __data*)&sdstatus;
  
  return *x;
}

void sdcard_power_off()
{
  SDCARD_CS = 0;
  SDCARD_CLK = 0;
  SDCARD_MOSI = 0;
  SDCARD_POWER_OFF;
}

void sdcard_hw_read()
{
  unsigned char i = 128;
  uint8_t __data *data = (uint8_t __data *) SDBLOCKBUF;
  SSP2CON1bits.SSPEN = 1;
  
  while(i!= 0) {
    SSP2BUF = 0xff;
    while(!SSP2STATbits.BF);
    *data = SSP2BUF;
    data++;
    SSP2BUF = 0xFF;
    while(!SSP2STATbits.BF);
    *data = SSP2BUF;
    data++;
    SSP2BUF = 0xFF;
    while(!SSP2STATbits.BF);
    *data = SSP2BUF;
    data++;
    SSP2BUF = 0xFF;
    while(!SSP2STATbits.BF);
    *data = SSP2BUF;
    data++;
    i--;
  }
 
  SSP2CON1bits.SSPEN = 0;
}

void sdcard_io_init()
{
  ANCON0 |= (1 << 1) | (1 << 2) | (1 << 4);
  RPINR22 = 2; /* SCK2 in = RP2 */
  RPINR21 = 3; /* SDI2 in = RP3 */
  RPOR2 = 10; /* set RP2 output function to clock out */
  SSP2CON1 = 0;
  SSP2STATbits.CKE = 0;
  SSP2CON1bits.CKP = 1;
  SSP2STATbits.SMP = 1;
  SSP2CON1bits.SSPM =0;
  //RPOR2 = 10;
 
  //RPOR2 = 10;
  SDCARD_CS = 0;
  SDCARD_CLK = 0;
  SDCARD_MOSI = 0;

  sdstatus.ready = 0;
  sd_last_block=2;
  busybufpos = 0;
  resettries = 0;
  sdstatus.ready = 0;
  sdstatus.writing = 0;
  sdstatus.reading = 0;
  sdstatus.HC = 0;
  sdstatus.busy = 0;
  sdstatus.wakeup = 0;
  sdstatus.err = 0;
  sdstatus.powered = 0;
  SDCARD_POWER_OFF;
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
  for(i=0;i<16;i++)
    spi_transact_byte(0xff);
  SDCARD_CS=0;
  for(i=0;i<sizeof(cmd0);i++) {
    spi_transact_byte(cmd0[i]);
  }
  for(i=0;i<128;i++) {
    delay1ktcy(1);
    r1=spi_transact_byte(0xff);
    if (r1 == 0x01)
      break;
  }
  SD_DEBUG_OUT(r1);
  return (r1 == 0x1);

}

unsigned char sdcard_idle()
{
  if (!sdstatus.powered)
    return 0;
  if (resettries != 0)  {
    if (sdcard_do_reset()) {
      resettries = 0;
    } else {
      resettries--;
      if (resettries == 0)
	sdstatus.err = 1;
      return 0;
    }
  }
  if (sdstatus.busy) {
    if (spi_transact_byte(0x0) != 0) {
      unsigned char r;
      sdstatus.busy = 0;
      SDCARD_CS = 1;
      spi_transact_byte(0xff);
      SDCARD_CS = 0;
      spi_transact_byte(0x40+13);
      spi_transact_byte(0);
      spi_transact_byte(0);
      spi_transact_byte(0);
      spi_transact_byte(0);
      spi_transact_byte(0xff);
      r = get_r1();
      /* get second byte of r2 */
      r = spi_transact_byte(0xff);
      SDCARD_CS = 1;
      return 1;
    }
    return 0;
  } else if (sdstatus.wakeup) {
    sdcard_wakeup();
 
  }
  if (sdstatus.ready)
    return 1;
  else
    return 0;
}

unsigned char sdcard_write_block(uint32_t block)
{
  uint8_t __data *data = (uint8_t __data *) SDBLOCKBUF;
  uint8_t r;
  uint8_t ret = 0;
  uint16_t i;
  if (!sdstatus.ready)
    return 0;
  sdcard_start_write(block);
  if (!sdstatus.writing)
    return 0;
  for(i=0;i!=512;i++) {
    spi_transact_byte(*data);
    data++;
  }
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
  return ret;
}

unsigned char sdcard_start_read(uint32_t block)
{
  unsigned char r1;
  if (sdstatus.ready == 0)
    return 0;
  if (sdstatus.busy)
    return 0;
  spi_transact_byte(0xff);
  SDCARD_CS = 0;
  spi_transact_byte(0x40+17);
  spi_transact_byte((block >> 24) & 255);
  spi_transact_byte((block >> 16) & 255);
  spi_transact_byte((block >>  8) & 255);
  spi_transact_byte(block & 255);
  spi_transact_byte(0xff);
  r1 = get_r1();
  SD_DEBUG_OUT(r1);
  if (r1 != 0)
    return 0;
  remaining = 512;
  sdstatus.reading = 1;
  // usb_ep4_flush();
  return 1;
}

unsigned char sdcard_read_block(uint32_t block)
{
  uint8_t __data *data = (uint8_t __data *)SDBLOCKBUF;
  uint16_t i;
  unsigned char ret = 0;
  if (!sdstatus.ready)
    return 0;
  sdcard_start_read(block);
  if (sdstatus.reading) {
    ret = 0xff;
    for(i= 128 ; i != 0 ; i--) {
      ret = spi_transact_byte(0xff);
      if (ret != 0xff)
	break;
    }
    if (ret == 0xff) {
      for(i = 128; i != 0; i--) {
	ret = spi_transact_byte(0xff);
	if (ret != 0xff)
	  break;
	delay1ktcy(1);
      }
    }
    if (ret != 0xfe) {
      sdstatus.reading = 0;
      SD_DEBUG_OUT(ret);
      SDCARD_CS = 1;
      spi_transact_byte(0xff);
      return 0;
    }
    if (block != 0) {
      /* if (block < 10) */
	sdcard_hw_read();
	/*
      else 
	quickread();
	*/
      
    } else {
      for(i=0;!(i&0x200);i++) {
	*data = spi_transact_byte(0xff);
	SD_DEBUG_OUT(*data);
	data++;
      }
    }
    spi_transact_byte(0xff);
    spi_transact_byte(0xff);
    sdstatus.reading = 0;
    ret = 1;
  } 
  SDCARD_CS = 1;
  return ret;
}

void sdcard_flush_write()
{
  uint8_t r;
  if (!sdstatus.writing)
    return;
  while(remaining) {
    spi_transact_byte(0);
    remaining--;
  }
  spi_transact_byte(0xff);
  spi_transact_byte(0xff);
  r = spi_transact_byte(0xff);
  sdstatus.busy = 1;
  sdstatus.writing = 0;
}

unsigned char sdcard_put_byte(uint8_t data)
{
  unsigned char ret = 0;
  if (!sdstatus.ready)
    return 0;
  if (sdstatus.busy) {
    return SDCARD_EAGAIN;
  }
  if (!sdstatus.writing) {
    sdcard_start_write(sd_last_block);
    if (sdstatus.writing) {
      sd_last_block++;
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
    unsigned char r;
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
  if (r1 != 0) {
    sdstatus.err = 1;
    return 0;
  }
  /* start block token */
  spi_transact_byte(0xfe);
  remaining = 512;
  sdstatus.writing = 1;
  usb_ep4_flush();
  return 1;
}

void sdcard_power_on()
{
  sdstatus.ready = 0;
  sdstatus.writing = 0;
  sdstatus.reading = 0;
  sdstatus.HC = 0;
  sdstatus.busy = 0;
  sdstatus.wakeup = 0;
  sdstatus.err = 0;
  busybufpos = 0;
  SDCARD_POWER_ON;
  sdstatus.powered = 1;
  SDCARD_CS = 1;
  resettries = 16;
}

static unsigned char sdcard_do_reset()
{
  unsigned char i, r1;
  if (!sdcard_reset()) {
    usb_ep4_flush();
    return 0;
  }
  SDCARD_CS = 1;
  spi_transact_byte(0xff);
  //spi_transact_byte(0xff);
  //spi_transact_byte(0xff);
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
  /* echo back failure */
  if (r1 != 0xaa) 
    return 0;
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

