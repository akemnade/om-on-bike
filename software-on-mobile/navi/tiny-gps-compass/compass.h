int compass_get_heading(float *heading);
void init_compass(int argc, char **argv);
void init_compass_calib();

#define EVENT_MAGNETOMETER 0x11
#define EVENT_MAGNETOMETER_EXT 0x13
#define EVENT_HEADING 0x12
typedef struct {
  evt_head_t head;
  int16_t x;
  int16_t y;
  int16_t z;
} __attribute__ ((__packed__)) magnetometer_evt_t;
typedef struct {
  evt_head_t head;
  int16_t x;
  int16_t y;
  int16_t z;
  int16_t ax;
  int16_t ay;
  int16_t az;
} __attribute__ ((__packed__)) magnetometer_ext_evt_t;
typedef struct {
  evt_head_t head;
  u_int16_t heading10;
} __attribute__ ((__packed__)) heading_evt_t;
float calculate_heading(magnetometer_evt_t *mag, int *xr, int *yr, int *zr);
