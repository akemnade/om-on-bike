void msd_ep_init();

void msd_ep_handle_out(uint8_t len);
void msd_ep_handle_in();
uint32_t msd_get_num_blocks_cb();
uint8_t msd_unit_ready_cb();
uint8_t msd_read_cb(uint32_t blocknum);
void msd_send_epin_cb(__data uint8_t *epoutb,uint8_t len);
void msd_ep_stall_in_cb();
void msd_ep_stall_out_cb();

extern __data uint8_t *msd_readblockbuf;
#define MSD_READY 0
#define MSD_SUCCESS 0
#define MSD_MEDIUM_NOT_PRESENT 1
#define MSD_READ_ERROR 2
#define MSD_BLOCK_SIZE 512
