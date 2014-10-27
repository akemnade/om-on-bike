void sdcard_io_init();
void sdcard_init();
unsigned char sdcard_start_write(uint32_t block);
unsigned char sdcard_put_byte(uint8_t data);
unsigned char sdcard_idle();
unsigned char sdcard_read_block(uint32_t block);
unsigned char sdcard_write_block(uint32_t block);
unsigned char sdcard_start_read(uint32_t block);
unsigned char sdcard_status();
extern uint32_t sd_last_block;

void sdcard_flush_write();
