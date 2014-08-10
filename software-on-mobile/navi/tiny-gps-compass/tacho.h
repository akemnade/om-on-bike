typedef struct {
  evt_head_t head;
  u_int32_t counter;
} __attribute__ ((__packed__)) evt_tacho_t;

typedef struct {
  evt_head_t head;
  int16_t speed;
} __attribute__ ((__packed__)) evt_speed_t;

#define EVENT_DYNAMO 0x19
#define EVENT_TACHO 0x1A
#define EVENT_SPEED 0x1B

int tacho_get_speed(float *speed);
void init_tacho(int argc, char **argv);
