

typedef struct {
  unsigned char type;
  unsigned char len;
  struct timeval tv;
} __attribute__ ((__packed__)) evt_head_t ;

#define EVT_INIT(typ,type_const,name) typ name; ((evt_head_t *)&name)->type=type_const; ((evt_head_t *)&name)->len=sizeof(typ);

void evt_init();
void distribute_evt(evt_head_t *evt);
