#define SDBLOCKBUF 0x800
#define MSD_EPINB 0xa80
#define MSD_EPOUTB 0xa40
#define MSD_CSWB 0xac0
void usb_ep4_put(unsigned char s);
void usb_ep4_flush();
