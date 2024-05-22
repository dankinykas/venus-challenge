// TU/e Engineering challenge for venus
// This file contains function headers for using the color sensor provided as part of the course

#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <pulsecounter.h>
#include <stdint.h>

#define PULSECOUNTER_FREQUENCY 100000000
#define COLOR_SENSOR_OUT IO_AR4
#define COLOR_SENSOR_S2 IO_AR5
#define COLOR_SENSOR_S3 IO_AR6

struct pulsecount {
  uint32_t count;
  uint32_t time;
};

double get_color_frequency();