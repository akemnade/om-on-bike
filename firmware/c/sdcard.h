void sdcard_io_init();
unsigned char sdcard_init();
unsigned char sdcard_start_write(uint32_t block);
unsigned char sdcard_put_byte(uint8_t data);
void sdcard_idle();
